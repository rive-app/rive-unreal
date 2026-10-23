// Copyright 2024-2026 Rive, Inc. All rights reserved.

// TODO(memory): Reference the font bytes instead of copying them.
//
// Today URiveExternalFont copies a face's bytes out of Unreal
// (RiveFontUtils::ExtractFontBytes) and hands the copy to
// CommandQueue::decodeFont, which keeps it for the font's lifetime. For an
// in-memory face (the editor, or Inline in a cooked build) that is a second
// resident copy of every bound font, tens of MB for CJK faces.
//
// A reference strategy would let Rive read Unreal's buffer in place:
//  - Core: add an HBFont::Decode overload that borrows caller-owned bytes
//    (HB_MEMORY_MODE_READONLY) and calls a release callback when the font is
//    destroyed, alongside the existing Decode(std::vector&&) that owns them.
//  - Plugin: have RiveFontUtils return a shared owner of the bytes (the face's
//    FFontFaceDataConstRef, or the TArray read from a .ttf/.ufont with a view
//    of the font payload), decode with the borrowing overload, and register
//    the font with CommandQueue::addExternalFont instead of decodeFont.
// That makes in-memory faces free and disk-backed ones cost only the read.
//
// It is safe because engine code only calls FFontFaceData::SetData while
// creating or loading a face, and a reload swaps in a new FFontFaceData rather
// than overwriting it; Slate's FFreeTypeFace relies on the same contract. The
// shared owner keeps the bytes alive as long as any Rive font uses them.
// Trade-offs: decoding moves to the game thread (addExternalFont takes a
// decoded font), and decode failures become synchronous. Deferred to a wider
// optimization pass because it needs the core runtime change.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtrTemplates.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/command_queue.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveExternalFont.generated.h"

class UFontFace;
class URiveExternalFont;
struct FRiveCommandBuilder;

UENUM(BlueprintType)
enum class ERiveExternalFontState : uint8
{
  // The decode is queued on the command server.
  Pending,
  // The server decoded the font; it can be bound.
  Decoded,
  // The server rejected the bytes; the handle never names a font.
  Failed,
  // Release() freed the font; it can no longer be bound.
  Released,
};

// Receives the messages for one URiveExternalFont's handle. Owned by that
// font and freed when it is released, so a message arriving afterwards finds
// no listener (ListenerBase's destructor unregisters the handle), and nothing
// leaks if the renderer shuts down before the server confirms the delete.
// Artboard and view model listeners instead delete themselves when the delete
// is confirmed.
class FRiveExternalFontListener final : public rive::CommandQueue::FontListener
{
public:
  explicit FRiveExternalFontListener(URiveExternalFont* InOwner)
      : Owner(InOwner)
  {}

  virtual void onFontDecoded(const rive::FontHandle Handle, uint64_t RequestId)
      override;
  virtual void onFontError(
      const rive::FontHandle Handle,
      uint64_t RequestId,
      std::string Error) override;

private:
  TWeakObjectPtr<URiveExternalFont> Owner;
};

/**
 * A UFontFace decoded for Rive, to bind to view model font properties. Like
 * URiveBlobAsset, the caller owns it: it stays decoded while anything
 * references it (a variable, or a view model property it is bound to), and is
 * freed by Release() or once garbage collected. Each MakeExternalFont call
 * decodes anew, so keep and reuse one font for a face bound in several places
 * or switched back to often.
 */
UCLASS(BlueprintType)
class RIVE_API URiveExternalFont : public UObject
{
  GENERATED_BODY()

public:
  virtual void BeginDestroy() override;

  /**
   * Decodes Face for binding to view model font properties.
   * @return The font, or nullptr (logged) if Face is null, has no bytes
   * Rive can read, or the renderer is unavailable.
   */
  UFUNCTION(BlueprintCallable, Category = Rive)
  static URiveExternalFont* MakeExternalFont(UFontFace* Face);

  /**
   * Frees the decoded font now rather than at garbage collection. Properties
   * already showing it keep their copy; binding it again is an error. Safe
   * to call more than once.
   */
  UFUNCTION(BlueprintCallable, Category = Rive)
  void Release();

  UFUNCTION(BlueprintPure, Category = Rive)
  UFontFace* GetFontFace() const { return FontFace; }

  UFUNCTION(BlueprintPure, Category = Rive)
  ERiveExternalFontState GetState() const { return State; }

  // RIVE_NULL_HANDLE once released.
  rive::FontHandle GetNativeHandle() const { return NativeFontHandle; }

  // Extracts Face's bytes and queues their decode. False (logged) if the
  // face has no bytes Rive can read.
  bool Initialize(FRiveCommandBuilder& Builder, UFontFace* Face);

private:
  friend class FRiveExternalFontListener;

  void OnDecoded();
  void OnDecodeFailed(const FString& Error);

  // Strong: the face stays loaded while this font is alive.
  UPROPERTY()
  TObjectPtr<UFontFace> FontFace;

  rive::FontHandle NativeFontHandle = RIVE_NULL_HANDLE;
  // For stats: the bytes handed to Rive (0 once it no longer holds them), and
  // when the decode was queued.
  int64 FontByteCount = 0;
  uint64 DecodeStartCycles = 0;
  ERiveExternalFontState State = ERiveExternalFontState::Pending;

  TUniquePtr<FRiveExternalFontListener> Listener;
};
