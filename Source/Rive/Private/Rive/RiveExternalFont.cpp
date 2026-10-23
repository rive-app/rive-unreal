// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveExternalFont.h"

#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "RiveCommandBuilder.h"
#include "RiveFontUtils.h"
#include "RiveRenderer.h"
#include "Stats/RiveStats.h"

DECLARE_DWORD_ACCUMULATOR_STAT(
    TEXT("External Fonts"),
    STAT_RIVE_EXTERNALFONTS,
    STATGROUP_Rive);
// The copy of the font bytes Rive holds for each live external font.
DECLARE_MEMORY_STAT(
    TEXT("External Font Bytes"),
    STAT_RIVE_EXTERNALFONTBYTES,
    STATGROUP_Rive);
DECLARE_DWORD_ACCUMULATOR_STAT(
    TEXT("External Font Decode Failures"),
    STAT_RIVE_EXTERNALFONTDECODEFAILURES,
    STATGROUP_Rive);
// From queuing a decode to the game thread hearing it succeeded. The decode
// runs inside the runtime on the command server, out of reach of these stats,
// so this is the only view of it; it includes the wait for the next frame's
// message processing.
DECLARE_FLOAT_ACCUMULATOR_STAT(
    TEXT("Last External Font Decode (ms)"),
    STAT_RIVE_EXTERNALFONTDECODEMS,
    STATGROUP_Rive);

namespace
{
FRiveRenderer* GetRiveRenderer()
{
  return IRiveRendererModule::IsAvailable()
             ? IRiveRendererModule::Get().GetRenderer()
             : nullptr;
}
} // namespace

void FRiveExternalFontListener::onFontDecoded(
    const rive::FontHandle Handle,
    uint64_t RequestId)
{
  check(IsInGameThread());
  if (URiveExternalFont* Font = Owner.Get())
  {
    Font->OnDecoded();
  }
}

void FRiveExternalFontListener::onFontError(
    const rive::FontHandle Handle,
    uint64_t RequestId,
    std::string Error)
{
  check(IsInGameThread());
  const FString Message = UTF8_TO_TCHAR(Error.c_str());
  if (URiveExternalFont* Font = Owner.Get())
  {
    Font->OnDecodeFailed(Message);
  }
  else
  {
    UE_LOG(
        LogRive,
        Error,
        TEXT("Font handle %p RequestId: %llu Error: %s"),
        Handle,
        RequestId,
        *Message);
  }
}

void URiveExternalFont::BeginDestroy()
{
  Release();
  Super::BeginDestroy();
}

URiveExternalFont* URiveExternalFont::MakeExternalFont(UFontFace* Face)
{
  SCOPED_NAMED_EVENT_TEXT(
      TEXT("URiveExternalFont::MakeExternalFont"),
      FColor::White);
  DECLARE_SCOPE_CYCLE_COUNTER(
      TEXT("URiveExternalFont::MakeExternalFont"),
      STAT_RIVE_EXTERNALFONT_MAKE,
      STATGROUP_Rive);

  if (Face == nullptr)
  {
    return nullptr;
  }

  FRiveRenderer* Renderer = GetRiveRenderer();
  if (Renderer == nullptr)
  {
    UE_LOG(
        LogRive,
        Error,
        TEXT("MakeExternalFont: no Rive renderer to decode '%s'"),
        *Face->GetName());
    return nullptr;
  }

  URiveExternalFont* Font = NewObject<URiveExternalFont>();
  if (!Font->Initialize(Renderer->GetCommandBuilder(), Face))
  {
    return nullptr;
  }
  return Font;
}

bool URiveExternalFont::Initialize(
    FRiveCommandBuilder& Builder,
    UFontFace* Face)
{
  check(IsInGameThread());
  check(Face != nullptr && NativeFontHandle == RIVE_NULL_HANDLE);

  std::vector<uint8_t> Bytes;
  if (!RiveFontUtils::ExtractFontBytes(FFontData(Face), Bytes))
  {
    UE_LOG(
        LogRive,
        Error,
        TEXT("MakeExternalFont: could not extract font bytes from "
             "'%s'"),
        *Face->GetName());
    return false;
  }

  FontFace = Face;
  FontByteCount = static_cast<int64>(Bytes.size());
  Listener = MakeUnique<FRiveExternalFontListener>(this);
  NativeFontHandle = Builder.CreateFont(MoveTemp(Bytes), Listener.Get());
  State = ERiveExternalFontState::Pending;
  DecodeStartCycles = FPlatformTime::Cycles64();

  INC_DWORD_STAT(STAT_RIVE_EXTERNALFONTS);
  INC_MEMORY_STAT_BY(STAT_RIVE_EXTERNALFONTBYTES, FontByteCount);
  return true;
}

void URiveExternalFont::Release()
{
  if (NativeFontHandle == RIVE_NULL_HANDLE)
  {
    return; // Never decoded, or already released.
  }
  check(IsInGameThread());
  SCOPED_NAMED_EVENT_TEXT(TEXT("URiveExternalFont::Release"), FColor::White);
  DECLARE_SCOPE_CYCLE_COUNTER(
      TEXT("URiveExternalFont::Release"),
      STAT_RIVE_EXTERNALFONT_RELEASE,
      STATGROUP_Rive);

  // During shutdown the renderer, and the server holding the font with it,
  // can already be gone.
  if (FRiveRenderer* Renderer = GetRiveRenderer())
  {
    Renderer->GetCommandBuilder().DestroyFont(NativeFontHandle);
  }
  NativeFontHandle = RIVE_NULL_HANDLE;
  State = ERiveExternalFontState::Released;

  DEC_DWORD_STAT(STAT_RIVE_EXTERNALFONTS);
  DEC_MEMORY_STAT_BY(STAT_RIVE_EXTERNALFONTBYTES, FontByteCount);
  FontByteCount = 0;

  // Unregisters the handle, so the delete confirmation and any decode
  // result still in flight find no listener. On the game thread: Release
  // runs from Blueprint, or from BeginDestroy, and UObject destruction runs
  // there (IsDestructionThreadSafe is false).
  Listener.Reset();
}

void URiveExternalFont::OnDecoded()
{
  if (State == ERiveExternalFontState::Pending)
  {
    State = ERiveExternalFontState::Decoded;
    SET_FLOAT_STAT(
        STAT_RIVE_EXTERNALFONTDECODEMS,
        FPlatformTime::ToMilliseconds64(
            FPlatformTime::Cycles64() - DecodeStartCycles));
  }
}

void URiveExternalFont::OnDecodeFailed(const FString& Error)
{
  State = ERiveExternalFontState::Failed;

  // The server keeps nothing for a failed decode, so its bytes no longer
  // count towards what Rive holds.
  INC_DWORD_STAT(STAT_RIVE_EXTERNALFONTDECODEFAILURES);
  DEC_MEMORY_STAT_BY(STAT_RIVE_EXTERNALFONTBYTES, FontByteCount);
  FontByteCount = 0;

  // Log only, matching how the other binding types report errors. A view
  // model delegate could surface this to Blueprints once bindings are
  // reworked.
  UE_LOG(
      LogRive,
      Error,
      TEXT("Font '%s' failed to decode (%s); properties bound to it show "
           "no font. Make a new font from the face to retry."),
      *GetNameSafe(FontFace),
      *Error);
}
