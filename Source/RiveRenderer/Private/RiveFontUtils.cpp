// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveFontUtils.h"

#include "Fonts/CompositeFont.h"
#include "HAL/FileManager.h"
#include "Logs/RiveRendererLog.h"
#include "RiveStats.h"
#include "Misc/FileHelper.h"
#include "Serialization/MemoryReader.h"

namespace
{
// Big-endian sfnt/WOFF tag from the first four bytes, or 0 if too short.
// Bytes are widened to uint32 before shifting: a promoted int shifted left by
// 24 is undefined for bytes >= 0x80.
uint32 ReadFontTag(TConstArrayView<uint8> Bytes)
{
  if (Bytes.Num() < 4)
  {
    return 0;
  }
  return (uint32(Bytes[0]) << 24) | (uint32(Bytes[1]) << 16) |
         (uint32(Bytes[2]) << 8) | uint32(Bytes[3]);
}

// Mirrors FCompositeFontCache::GetFontFace's byte sniffing (SlateCore), minus
// WOFF/WOFF2, which HarfBuzz can't decode. Pure check: a false result is how
// the cooked .ufont path is chosen, so it must not log.
bool IsRawFontData(TConstArrayView<uint8> Bytes)
{
  const uint32 Tag = ReadFontTag(Bytes);
  return Tag == 0x00010000     // TrueType
         || Tag == 0x4F54544F  // 'OTTO' CFF OpenType
         || Tag == 0x74746366; // 'ttcf' collection
}

bool IsWoffData(TConstArrayView<uint8> Bytes)
{
  const uint32 Tag = ReadFontTag(Bytes);
  return Tag == 0x774F4646     // 'wOFF'
         || Tag == 0x774F4632; // 'wOF2'
}

void AssignBytes(const TArray<uint8>& Bytes, std::vector<uint8_t>& OutBytes)
{
  OutBytes.assign(Bytes.GetData(), Bytes.GetData() + Bytes.Num());
}
} // namespace

bool RiveFontUtils::ExtractFontBytes(
    const FFontData& FontData,
    std::vector<uint8_t>& OutBytes)
{
  SCOPED_NAMED_EVENT_TEXT(
      TEXT("RiveFontUtils::ExtractFontBytes"),
      FColor::White);
  DECLARE_SCOPE_CYCLE_COUNTER(
      TEXT("RiveFontUtils::ExtractFontBytes"),
      STAT_RIVEFONTUTILS_EXTRACTFONTBYTES,
      STATGROUP_Rive);

  const FString& Filename = FontData.GetFontFilename();

  // 1. In memory: the editor, PIE, or Inline in a cooked build. This holds
  // the source file's bytes, so it can still be a format Rive can't read.
  if (FFontFaceDataConstPtr FaceData = FontData.GetFontFaceData();
      FaceData && FaceData->HasData())
  {
    if (!IsRawFontData(FaceData->GetData()))
    {
      UE_LOG(
          LogRiveRenderer,
          Error,
          TEXT("ExtractFontBytes: '%s' is not a TrueType/OpenType "
               "font (WOFF/WOFF2 are not supported)"),
          *Filename);
      return false;
    }
    AssignBytes(FaceData->GetData(), OutBytes);
    return true;
  }

  // 2. On disk: cooked LazyLoad/Stream (.ufont), simulated standalone, or a
  // legacy filename font. Check the size before reading anything.
  const int64 FileSize = IFileManager::Get().FileSize(*Filename);
  if (FileSize < 0)
  {
    UE_LOG(
        LogRiveRenderer,
        Error,
        TEXT("ExtractFontBytes: could not read font file '%s'"),
        *Filename);
    return false;
  }
  if (FileSize > RiveFontUtils::MaxFontFileBytes)
  {
    UE_LOG(
        LogRiveRenderer,
        Error,
        TEXT("ExtractFontBytes: '%s' is %lld bytes, over the %lld byte "
             "limit for a font"),
        *Filename,
        FileSize,
        RiveFontUtils::MaxFontFileBytes);
    return false;
  }

  TArray<uint8> FileBytes;
  {
    // Synchronous disk IO on the calling thread, which is the game thread
    // for view model bindings: the likeliest hitch for LazyLoad/Stream fonts
    // in a cooked build, so it gets its own scope.
    SCOPED_NAMED_EVENT_TEXT(
        TEXT("RiveFontUtils::ExtractFontBytes ReadFile"),
        FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(
        TEXT("RiveFontUtils::ExtractFontBytes ReadFile"),
        STAT_RIVEFONTUTILS_EXTRACTFONTBYTES_READFILE,
        STATGROUP_Rive);
    if (!FFileHelper::LoadFileToArray(FileBytes, *Filename))
    {
      UE_LOG(
          LogRiveRenderer,
          Error,
          TEXT("ExtractFontBytes: could not read font file '%s'"),
          *Filename);
      return false;
    }
  }

  if (IsRawFontData(FileBytes))
  {
    AssignBytes(FileBytes, OutBytes);
    return true;
  }

  if (IsWoffData(FileBytes))
  {
    UE_LOG(
        LogRiveRenderer,
        Error,
        TEXT("ExtractFontBytes: '%s' is WOFF/WOFF2, which Rive can't "
             "decode"),
        *Filename);
    return false;
  }

  // 3. Cooked .ufont: a serialized FFontFaceData whose first member is the
  // font as a TArray<uint8> (an int32 count, then the bytes). Read only that
  // array, checking its count against the bytes actually present first.
  // FFontFaceData::Serialize can't be used here: it trusts the counts in the
  // file and allocates for them before reading, both for this array and for
  // the preprocessed geometry array after it.
  FMemoryReader Ar(FileBytes, /*bIsPersistent*/ true);
  int32 PayloadSize = 0;
  Ar << PayloadSize;
  const int64 Available = FileBytes.Num() - Ar.Tell();
  if (Ar.IsError() || PayloadSize <= 0 || PayloadSize > Available)
  {
    UE_LOG(
        LogRiveRenderer,
        Error,
        TEXT("ExtractFontBytes: '%s' is not a TrueType/OpenType font "
             "or a valid cooked .ufont"),
        *Filename);
    return false;
  }
  const TConstArrayView<uint8> Payload(
      FileBytes.GetData() + Ar.Tell(),
      PayloadSize);
  if (!IsRawFontData(Payload))
  {
    UE_LOG(
        LogRiveRenderer,
        Error,
        TEXT("ExtractFontBytes: '%s' is a cooked .ufont that does not "
             "hold a TrueType/OpenType font"),
        *Filename);
    return false;
  }
  OutBytes.assign(Payload.GetData(), Payload.GetData() + Payload.Num());
  return true;
}
