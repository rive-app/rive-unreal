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

    // In case of reimport, we want to reuse the same widget and not create a new one
    URiveFile* ExistingRiveFile = FindObject<URiveFile>(InParent, *InName.ToString(), true);
    TSubclassOf<UUserWidget> ExistingWidgetClass = IsValid(ExistingRiveFile) ? ExistingRiveFile->GetWidgetClass() : nullptr;
    
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
    
    // Create Rive UMG unless there is already one from the re-imported RIveFile
    if (ExistingWidgetClass.Get())
    {
        RiveFile->SetWidgetClass(ExistingWidgetClass);
    }
    else if (!FRiveWidgetFactory(RiveFile).Create())
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

    if (!IsValid(Obj))
    {
        return EReimportResult::Failed;
    }

    const FString SourceFilename = RiveFile->RiveFilePath;
    if (!FPaths::FileExists(SourceFilename))
    {
        return EReimportResult::Failed;
    }

    const FString WidgetFullName = RiveFile->GetWidgetClass() ? RiveFile->GetWidgetClass()->GetFullName() : "";
    
    bool bOutCanceled = false;
    if (ImportObject(Obj->GetClass(), Obj->GetOuter(), *Obj->GetName(), RF_Public | RF_Standalone, SourceFilename, *WidgetFullName, bOutCanceled))
    {
        return EReimportResult::Succeeded;
    }

    return bOutCanceled ? EReimportResult::Cancelled : EReimportResult::Failed;
}

int32 URiveFileFactory::GetPriority() const
{
    return DefaultImportPriority + 1;
}
