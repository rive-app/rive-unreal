// Copyright Rive, Inc. All rights reserved.

#include "RiveFileFactory.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "RiveWidgetFactory.h"
#include "Blueprint/UserWidget.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"

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

UObject* URiveFileFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, const FString& InFilename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    if (!UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(LogRiveEditor, Error, TEXT("RiveRenderer is null, unable to import the Rive file '%s'"), *InFilename);
        return nullptr;
    }
    
    const FString FileExtension = FPaths::GetExtension(InFilename);
    const TCHAR* Type = *FileExtension;

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

    URiveFile* RiveFile = NewObject<URiveFile>(InParent, InClass, InName, InFlags | RF_Public);
    check(RiveFile);

    if (!FPaths::FileExists(InFilename))
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Rive file %s does not exist!"), *InFilename);
        return nullptr;
    }

    TArray<uint8> FileBuffer;
    if (!FFileHelper::LoadFileToArray(FileBuffer, *InFilename)) // load entire DNA file into the array
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Could not read DNA file %s!"), *InFilename);
        return nullptr;
    }
    
    if (!RiveFile->EditorImport(InFilename, FileBuffer))
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Could not import riv file"));
        return nullptr;
    }
    
    if (!FRiveWidgetFactory(RiveFile).Create())
    {
        return nullptr;
    }

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, RiveFile);
    
    return RiveFile;
}

bool URiveFileFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    if (const URiveFile* RiveFile = Cast<URiveFile>(Obj))
    {
        if (IsValid(RiveFile))
        {
            OutFilenames.Add(RiveFile->RiveFilePath);
            return true;
        }
    }
    return false;
}

void URiveFileFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    if (URiveFile* RiveFile = Cast<URiveFile>(Obj))
    {
        if (IsValid(RiveFile) && !NewReimportPaths.IsEmpty() && FPaths::FileExists(NewReimportPaths[0]))
        {
            RiveFile->RiveFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*NewReimportPaths[0]);
        }
    }
}

EReimportResult::Type URiveFileFactory::Reimport(UObject* Obj)
{
    URiveFile* RiveFile = Cast<URiveFile>(Obj);

    if (!IsValid(RiveFile) && !ensure(GEditor))
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Unable to Reimport the RiveFile as it is invalid"));
        return EReimportResult::Failed;
    }

    if (RiveFile->ParentRiveFile)
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Unable to Reimport the RiveFile '%s' as it is an instance, reimport the Parent directly."), *GetFullNameSafe(RiveFile));
        return EReimportResult::Failed;
    }
    
    const FString SourceFilename = RiveFile->RiveFilePath;
    
    if (!UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Unable to Reimport the RiveFile '%s' with file '%s' as the RiveRenderer is null"), *GetFullNameSafe(RiveFile), *SourceFilename);
        return EReimportResult::Failed;;
    }
    
    if (!FPaths::FileExists(SourceFilename))
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Failed to Reimport the RiveFile '%s' with file '%s' as the SourceFilename is null"), *GetFullNameSafe(RiveFile), *SourceFilename);
        return EReimportResult::Failed;
    }

    TArray<uint8> FileBuffer;
    if (!FFileHelper::LoadFileToArray(FileBuffer, *SourceFilename)) // load entire DNA file into the array
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Failed to Reimport the RiveFile '%s' with file '%s' as the we could not read DNA file"), *GetFullNameSafe(RiveFile), *SourceFilename);
        return EReimportResult::Failed;
    }
    
    UE_LOG(LogRiveEditor, Display, TEXT("Reimporting RiveFile '%s' with file '%s'"), *GetFullNameSafe(RiveFile), *SourceFilename);
    // We don't actually recreate the whole UObject via reimport (which would lose a lot of data), we just change the file and initialize.
    if (!RiveFile->EditorImport(SourceFilename, FileBuffer, true))
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Reimported the RiveFile '%s' with file '%s' but the initialization was unsuccessful"), *GetFullNameSafe(RiveFile), *SourceFilename);
        return EReimportResult::Failed;
    }
    else
    {
        for (TObjectIterator<URiveFile> It; It; ++It)
        {
            URiveFile* OtherRiveFile = *It;
            if (IsValid(OtherRiveFile) && OtherRiveFile->ParentRiveFile == RiveFile)
            {
                UE_LOG(LogRiveEditor, Warning, TEXT("OtherRiveFile '%s' with parent '%s'"), *GetFullNameSafe(OtherRiveFile), *GetFullNameSafe(RiveFile));
            }
        }
    }
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetReimport(RiveFile);

    return EReimportResult::Succeeded;
}

int32 URiveFileFactory::GetPriority() const
{
    return DefaultImportPriority + 1;
}
