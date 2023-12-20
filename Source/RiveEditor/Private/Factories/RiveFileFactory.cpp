// Copyright Rive, Inc. All rights reserved.

#include "RiveFileFactory.h"
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

UE_DISABLE_OPTIMIZATION

UObject* URiveFileFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, const FString& InFilename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
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

    if (!FFileHelper::LoadFileToArray(RiveFile->TempFileBuffer, *InFilename)) // load entire DNA file into the array
    {
        UE_LOG(LogRiveEditor, Error, TEXT("Could not read DNA file %s!"), *InFilename);

        return nullptr;
    }

    // Do not Serialize, simple load to BiteArray
    RiveFile->Initialize();

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, RiveFile);

    return RiveFile;
}

UE_ENABLE_OPTIMIZATION