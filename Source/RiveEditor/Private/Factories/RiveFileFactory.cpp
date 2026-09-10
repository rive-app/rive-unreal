// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveFileFactory.h"

#include "RiveBlueprintGeneration.h"

#include "Editor/EditorEngine.h"
#include "IRiveRendererModule.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "RiveWidgetFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/EnumEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logs/RiveEditorLog.h"
#include "Subsystems/ImportSubsystem.h"
#include "Misc/FileHelper.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveBlobAsset.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveUtils.h"
#include "Rive/RiveViewModel.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Types/AttributeStorage.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealTypePrivate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "FileHelpers.h"
#include "Misc/EngineVersionComparison.h"

extern UNREALED_API class UEditorEngine* GEditor;

URiveFileFactory::URiveFileFactory(const FObjectInitializer& ObjectInitializer)
{
    SupportedClass = URiveFile::StaticClass();

    Formats.Add(TEXT("riv;Rive Animation File"));

    bEditorImport = true;
    bEditAfterNew = true;
}

bool URiveFileFactory::FactoryCanImport(const FString& Filename)
{
    return FPaths::GetExtension(Filename).Equals(TEXT("riv"));
}

UObject* URiveFileFactory::FactoryCreateFile(UClass* InClass,
                                             UObject* InParent,
                                             FName InName,
                                             EObjectFlags InFlags,
                                             const FString& InFilename,
                                             const TCHAR* Params,
                                             FFeedbackContext* Warn,
                                             bool& bOutOperationCanceled)
{
    const FString FileExtension = FPaths::GetExtension(InFilename);
    const TCHAR* Type = *FileExtension;
    GEditor->GetEditorSubsystem<UImportSubsystem>()
        ->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

    if (!IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(
            LogRiveEditor,
            Error,
            TEXT("Unable to import the Rive file '%s': the Renderer is null"),
            *InFilename);
        GEditor->GetEditorSubsystem<UImportSubsystem>()
            ->BroadcastAssetPostImport(this, nullptr);
        return nullptr;
    }

    if (!FPaths::FileExists(InFilename))
    {
        UE_LOG(
            LogRiveEditor,
            Error,
            TEXT(
                "Unable to import the Rive file '%s': the file does not exist"),
            *InFilename);
        GEditor->GetEditorSubsystem<UImportSubsystem>()
            ->BroadcastAssetPostImport(this, nullptr);
        return nullptr;
    }

    TArray<uint8> FileBuffer;
    if (!FFileHelper::LoadFileToArray(
            FileBuffer,
            *InFilename)) // load entire DNA file into the array
    {
        UE_LOG(
            LogRiveEditor,
            Error,
            TEXT(
                "Unable to import the Rive file '%s': Could not read the file"),
            *InFilename);
        GEditor->GetEditorSubsystem<UImportSubsystem>()
            ->BroadcastAssetPostImport(this, nullptr);
        return nullptr;
    }

    URiveFile* RiveFile =
        NewObject<URiveFile>(InParent, InClass, InName, InFlags | RF_Public);
    check(RiveFile);

    if (!RiveFile->EditorImport(InFilename, FileBuffer))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Failed to import the Rive file '%s': Could not import the "
                    "riv file"),
               *InFilename);
        RiveFile->ConditionalBeginDestroy();
        GEditor->GetEditorSubsystem<UImportSubsystem>()
            ->BroadcastAssetPostImport(this, nullptr);
        return nullptr;
    }

    TWeakObjectPtr<URiveFileFactory> WeakThis(this);
    auto Lambda = [WeakThis](URiveFile* RiveFile) {
        if (auto StrongThis = WeakThis.Pin(); StrongThis.IsValid())
        {
            auto Handle = StrongThis->OnDataReadyHandleMap.Find({RiveFile});
            if (Handle && Handle->IsValid())
            {
                RiveFile->OnDataReady.Remove(*Handle);
                StrongThis->OnDataReadyHandleMap.Remove({RiveFile});
            }
        }
        FRiveBlueprintGeneration::GenerateForFile(
            RiveFile,
            RiveUtils::GetPackagePathForFile(RiveFile),
            true);
    };

    if (RiveFile->GetHasData())
    {
        Lambda(RiveFile);
    }
    else
    {
        auto Handle = OnDataReadyHandleMap.Find({RiveFile});
        if (!Handle || !Handle->IsValid())
        {
            auto NewHandle = RiveFile->OnDataReady.AddLambda(Lambda);
            if (Handle)
            {
                *Handle = NewHandle;
            }
            else
            {
                OnDataReadyHandleMap.Add({RiveFile}, NewHandle);
            }
        }
    }

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(
        this,
        RiveFile);

    return RiveFile;
}

bool URiveFileFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    if (const URiveFile* RiveFile = Cast<URiveFile>(Obj))
    {
        if (IsValid(RiveFile))
        {
            OutFilenames.Add(RiveFile->AssetImportData->GetFirstFilename());
            return true;
        }
    }
    return false;
}

void URiveFileFactory::SetReimportPaths(UObject* Obj,
                                        const TArray<FString>& NewReimportPaths)
{
    if (URiveFile* RiveFile = Cast<URiveFile>(Obj))
    {
        if (IsValid(RiveFile) && !NewReimportPaths.IsEmpty() &&
            FPaths::FileExists(NewReimportPaths[0]))
        {
            RiveFile->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
        }
    }
}

EReimportResult::Type URiveFileFactory::Reimport(UObject* Obj)
{
    URiveFile* RiveFile = Cast<URiveFile>(Obj);

    if (!IsValid(RiveFile) || !ensure(GEditor))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Unable to Reimport the RiveFile: it is invalid"));
        return EReimportResult::Failed;
    }

    const FString SourceFilename =
        RiveFile->AssetImportData->GetFirstFilename();

    if (!IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Unable to Reimport the Rive file '%s' with file '%s': the "
                    "RiveRenderer is null"),
               *GetFullNameSafe(RiveFile),
               *SourceFilename);
        return EReimportResult::Failed;
        ;
    }

    if (!FPaths::FileExists(SourceFilename))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Failed to Reimport the Rive file '%s' with file '%s': the "
                    "SourceFilename is null"),
               *GetFullNameSafe(RiveFile),
               *SourceFilename);
        return EReimportResult::Failed;
    }

    TArray<uint8> FileBuffer;
    if (!FFileHelper::LoadFileToArray(
            FileBuffer,
            *SourceFilename)) // load entire DNA file into the array
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Failed to Reimport the Rive file '%s' with file '%s': "
                    "the file could not be read"),
               *GetFullNameSafe(RiveFile),
               *SourceFilename);
        return EReimportResult::Failed;
    }

    UE_LOG(LogRiveEditor,
           Display,
           TEXT("Reimporting RiveFile '%s' with file '%s'"),
           *GetFullNameSafe(RiveFile),
           *SourceFilename);
    // We don't actually recreate the whole UObject via reimport (which would
    // lose a lot of data), we just change the file and initialize.
    if (!RiveFile->EditorImport(SourceFilename, FileBuffer, true))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Reimported the RiveFile '%s' with file '%s' but the "
                    "initialization was "
                    "unsuccessful"),
               *GetFullNameSafe(RiveFile),
               *SourceFilename);
        return EReimportResult::Failed;
    }

    TWeakObjectPtr<URiveFileFactory> WeakThis(this);
    auto Lambda = [WeakThis](URiveFile* RiveFile) {
        if (auto StrongThis = WeakThis.Pin(); StrongThis.IsValid())
        {
            auto Handle = StrongThis->OnDataReadyHandleMap.Find({RiveFile});
            if (Handle && Handle->IsValid())
            {
                RiveFile->OnDataReady.Remove(*Handle);
                StrongThis->OnDataReadyHandleMap.Remove({RiveFile});
            }
        }
        FRiveBlueprintGeneration::GenerateForFile(
            RiveFile,
            RiveUtils::GetPackagePathForFile(RiveFile),
            true);
    };

    if (RiveFile->GetHasData())
    {
        Lambda(RiveFile);
    }
    else
    {
        auto Handle = OnDataReadyHandleMap.Find({RiveFile});
        if (!Handle || !Handle->IsValid())
        {
            auto NewHandle = RiveFile->OnDataReady.AddLambda(Lambda);
            if (Handle)
            {
                *Handle = NewHandle;
            }
            else
            {
                OnDataReadyHandleMap.Add({RiveFile}, NewHandle);
            }
        }
    }

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetReimport(
        RiveFile);

    return EReimportResult::Succeeded;
}

int32 URiveFileFactory::GetPriority() const
{
    return DefaultImportPriority + 1;
}
