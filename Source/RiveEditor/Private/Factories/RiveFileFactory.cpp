// Copyright Rive, Inc. All rights reserved.

#include "RiveFileFactory.h"

#include "RiveWidgetFactory.h"
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

    RiveFile->Initialize();
    
    // Create Rive UMG
    if (!FRiveWidgetFactory(RiveFile).Create())
    {
        return nullptr;
    }

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, RiveFile);
    
    return RiveFile;
}
