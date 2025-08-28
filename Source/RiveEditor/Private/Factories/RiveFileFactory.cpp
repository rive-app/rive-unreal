// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveFileFactory.h"

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
#include "Rive/RiveFile.h"
#include "Rive/RiveUtils.h"
#include "Rive/RiveViewModel.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Types/AttributeStorage.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealTypePrivate.h"

extern UNREALED_API class UEditorEngine* GEditor;

static FName RiveDataTypeToPinCatagory(ERiveDataType DataType)
{
    switch (DataType)
    {
        case ERiveDataType::None:
            return "None";
        case ERiveDataType::String:
            return UEdGraphSchema_K2::PC_String;
        case ERiveDataType::Number:
            return UEdGraphSchema_K2::PC_Real;
        case ERiveDataType::Boolean:
            return UEdGraphSchema_K2::PC_Boolean;
        case ERiveDataType::Color:
            return UEdGraphSchema_K2::PC_Struct;
        case ERiveDataType::List:
            return UEdGraphSchema_K2::PC_Struct;
        case ERiveDataType::EnumType:
            return UEdGraphSchema_K2::PC_Byte;
        case ERiveDataType::Trigger:
            return UEdGraphSchema_K2::PC_MCDelegate;
        case ERiveDataType::ViewModel:
            return UEdGraphSchema_K2::PC_Object;
        case ERiveDataType::AssetImage:
            return UEdGraphSchema_K2::PC_Object;
        case ERiveDataType::Artboard:
            return UEdGraphSchema_K2::PC_Object;
    }

    return "None";
}

static UEnum* GenerateBlueprintForEnum(const FString& FolderPath,
                                       const FEnumDefinition& EnumDefinition)
{
    UPackage* FolderPackage = CreatePackage(*FolderPath);
    FString EnumName =
        TEXT("E") + ObjectTools::SanitizeObjectName(EnumDefinition.Name);
    FString PackageName = FolderPath + "/" + EnumName;
    FString SanitizedPackageName =
        UPackageTools::SanitizePackageName(PackageName);

    UUserDefinedEnum* Enum = Cast<UUserDefinedEnum>(
        FEnumEditorUtils::CreateUserDefinedEnum(FolderPackage,
                                                *EnumName,
                                                RF_Standalone | RF_Public));

    TArray<TPair<FName, int64>> EnumsValues;
    int64 Index = 0;
    for (auto Value : EnumDefinition.Values)
    {
        EnumsValues.Add(
            TPair<FName, int64>{EnumName + TEXT("::") + Value, Index++});
    }
    Enum->CppType = EnumName;
    Enum->SetEnums(EnumsValues, UEnum::ECppForm::Namespaced);

    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(Enum);

    // Mark the package dirty...
    if (!FolderPackage->MarkPackageDirty())
    {
        UE_LOG(LogRiveEditor,
               Warning,
               TEXT("Folder Package %s Failed to mark dirty."),
               *FolderPackage->GetName());
    }

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        SanitizedPackageName,
        FPackageName::GetAssetPackageExtension());
    FSavePackageArgs PackageArgs;
    PackageArgs.Error = GError;
    PackageArgs.TopLevelFlags =
        EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;

    bool bSaved = UPackage::SavePackage(FolderPackage,
                                        Enum,
                                        *PackageFileName,
                                        PackageArgs);

    if (bSaved)
    {
        UE_LOG(LogTemp,
               Log,
               TEXT("Enum '%s' successfully created and saved at '%s'."),
               *EnumName,
               *PackageFileName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save Enum '%s'."), *EnumName);
    }

    return Enum;
}

static UClass* GenerateBlueprintForViewModel(
    const FString& FolderPath,
    const FViewModelDefinition& ViewModelDefinition,
    const TArray<UEnum*>& Enums)
{
    UPackage* FolderPackage = CreatePackage(*FolderPath);
    FString BlueprintName =
        ObjectTools::SanitizeObjectName(ViewModelDefinition.Name);
    FString PackageName = FolderPath;
    PackageName.PathAppend(*BlueprintName, BlueprintName.Len());
    FString SanitizedPackageName =
        UPackageTools::SanitizePackageName(PackageName);

    // Create and init a new blank Blueprint
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        URiveViewModel::StaticClass(),
        FolderPackage,
        *BlueprintName,
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass());
    if (Blueprint)
    {
        // Editing the BluePrint
        UClass* BlueprintClass = Blueprint->GeneratedClass.Get();
        URiveViewModel* CDO =
            BlueprintClass->GetDefaultObject<URiveViewModel>();
        CDO->bIsGenerated = true;

        static FText VariableCategory = FText::FromString(TEXT("Rive"));
        Blueprint->Modify();

        for (auto PropertyDefinition : ViewModelDefinition.PropertyDefinitions)
        {
            FName NewVariableName = FName(PropertyDefinition.Name);
            FEdGraphPinType PinType = {};
            PinType.PinCategory =
                RiveDataTypeToPinCatagory(PropertyDefinition.Type);

            if (PropertyDefinition.Type == ERiveDataType::ViewModel)
            {
                PinType.PinSubCategoryObject = URiveViewModel::StaticClass();
            }
            else if (PropertyDefinition.Type == ERiveDataType::Artboard)
            {
                PinType.PinSubCategoryObject = URiveArtboard::StaticClass();
            }
            else if (PropertyDefinition.Type == ERiveDataType::EnumType)
            {
                FString SanitizedEnumName =
                    TEXT("E") + ObjectTools::SanitizeObjectName(
                                    PropertyDefinition.MetaData);
                UEnum* const* EnumPtr = Enums.FindByPredicate(
                    [SanitizedEnumName](const UEnum* Enum) {
                        return Enum->GetName() == SanitizedEnumName;
                    });
                UE_LOG(LogRiveEditor,
                       Error,
                       TEXT("Failed to find enum '%s'"),
                       *SanitizedEnumName);
                check(EnumPtr);
                PinType.PinSubCategoryObject = *EnumPtr;
            }
            else if (PropertyDefinition.Type == ERiveDataType::Number)
            {
                PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
            }
            else if (PropertyDefinition.Type == ERiveDataType::AssetImage)
            {
                PinType.PinSubCategoryObject = UTexture2D::StaticClass();
            }
            else if (PropertyDefinition.Type == ERiveDataType::Trigger)
            {
                FEdGraphPinType DelegateType;
                DelegateType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
                const bool bVarCreatedSuccess =
                    FBlueprintEditorUtils::AddMemberVariable(
                        Blueprint,
                        *PropertyDefinition.Name,
                        DelegateType);
                if (!bVarCreatedSuccess)
                {
                    UE_LOG(LogRiveEditor,
                           Error,
                           TEXT("Failed to add trigger property named \"%s\"."),
                           *PropertyDefinition.Name);
                    continue;
                }

                UEdGraph* const NewGraph =
                    FBlueprintEditorUtils::CreateNewGraph(
                        Blueprint,
                        *PropertyDefinition.Name,
                        UEdGraph::StaticClass(),
                        UEdGraphSchema_K2::StaticClass());
                if (!NewGraph)
                {
                    FBlueprintEditorUtils::RemoveMemberVariable(
                        Blueprint,
                        *PropertyDefinition.Name);
                    UE_LOG(LogRiveEditor,
                           Error,
                           TEXT("Failed to add trigger property named \"%s\" "
                                "signature graph."),
                           *PropertyDefinition.Name);
                    continue;
                }

                NewGraph->bEditable = false;

                const UEdGraphSchema_K2* K2Schema =
                    GetDefault<UEdGraphSchema_K2>();
                check(nullptr != K2Schema);

                K2Schema->CreateDefaultNodesForGraph(*NewGraph);
                K2Schema->CreateFunctionGraphTerminators(
                    *NewGraph,
                    static_cast<UClass*>(nullptr));
                K2Schema->AddExtraFunctionFlags(NewGraph,
                                                (FUNC_BlueprintCallable |
                                                 FUNC_BlueprintEvent |
                                                 FUNC_Public));
                K2Schema->MarkFunctionEntryAsEditable(NewGraph, true);

                Blueprint->DelegateSignatureGraphs.Add(NewGraph);
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(
                    Blueprint);
                continue;
            }
            else if (PropertyDefinition.Type == ERiveDataType::Color)
            {
                PinType.PinSubCategoryObject =
                    TBaseStructure<FLinearColor>::Get();
            }
            else if (PropertyDefinition.Type == ERiveDataType::List)
            {
                PinType.PinSubCategoryObject = FRiveList::StaticStruct();
            }

            FBPVariableDescription VariableDescription = {};
            VariableDescription.Category = VariableCategory;
            VariableDescription.VarName = NewVariableName;
            VariableDescription.FriendlyName = PropertyDefinition.Name;
            VariableDescription.VarGuid = FGuid::NewGuid();
            VariableDescription.VarType = PinType;
            VariableDescription.PropertyFlags |=
                (CPF_Edit | CPF_BlueprintVisible);
            if (PropertyDefinition.Type == ERiveDataType::String)
            {
                VariableDescription.SetMetaData(TEXT("MultiLine"),
                                                TEXT("true"));
            }
            else if (PropertyDefinition.Type == ERiveDataType::List)
            {
                VariableDescription.DefaultValue =
                    FString::Format(TEXT("(Path=\"{0}\")"),
                                    {PropertyDefinition.Name});
            }

            VariableDescription.SetMetaData(FBlueprintMetadata::MD_FieldNotify,
                                            TEXT(""));

            Blueprint->NewVariables.Add(VariableDescription);

            FBlueprintEditorUtils::ValidateBlueprintChildVariables(
                Blueprint,
                NewVariableName);
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(
                Blueprint);
        }
        // Compile the changes
        FCompilerResultsLog LogResults;
        FKismetEditorUtilities::CompileBlueprint(Blueprint,
                                                 EBlueprintCompileOptions::None,
                                                 &LogResults);

        // Notify the asset registry
        FAssetRegistryModule::AssetCreated(Blueprint);

        // Mark the package dirty...
        if (!FolderPackage->MarkPackageDirty())
        {
            UE_LOG(LogRiveEditor,
                   Warning,
                   TEXT("Folder Package %s Failed to mark dirty."),
                   *FolderPackage->GetName());
        }

        FString PackageFileName = FPackageName::LongPackageNameToFilename(
            SanitizedPackageName,
            FPackageName::GetAssetPackageExtension());
        FSavePackageArgs PackageArgs;
        PackageArgs.Error = GError;
        PackageArgs.TopLevelFlags =
            EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;

        bool bSaved = UPackage::SavePackage(FolderPackage,
                                            Blueprint,
                                            *PackageFileName,
                                            PackageArgs);

        if (bSaved)
        {
            UE_LOG(
                LogTemp,
                Log,
                TEXT("Blueprint '%s' successfully created and saved at '%s'."),
                *BlueprintName,
                *PackageFileName);
        }
        else
        {
            UE_LOG(LogTemp,
                   Error,
                   TEXT("Failed to save Blueprint '%s'."),
                   *BlueprintName);
        }

        return BlueprintClass;
    }

    return nullptr;
}

static void DeletePreviousAssetsForFile(URiveFile* File)
{
    auto PackagePath = RiveUtils::GetPackagePathForFile(File);
    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
            "AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();
    TArray<FAssetData> AssetData;
    if (AssetRegistry.GetAssetsByPackageName(*PackagePath, AssetData))
    {
        TArray<UObject*> Objects;

        for (const auto& ViewModelAssetData : AssetData)
        {
            Objects.Add(ViewModelAssetData.GetAsset());
        }

        auto MissedAssets =
            ObjectTools::ForceDeleteObjects(Objects, false) - AssetData.Num();
        if (MissedAssets > 0)
        {
            UE_LOG(LogRiveEditor,
                   Error,
                   TEXT("Failed to force delete all assets for file '%s'. "
                        "Number missed %i"),
                   *File->GetName(),
                   MissedAssets);
        }
    }
    // Remove files.
    UEditorAssetSubsystem* EditorAssetSubsystem =
        GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
    if (EditorAssetSubsystem != nullptr)
    {
        if (!EditorAssetSubsystem->DeleteDirectory(PackagePath))
        {
            UE_LOG(
                LogRiveEditor,
                Error,
                TEXT("Failed to delete the previous assets for the file '%s'"),
                *File->GetName());
        }
    }
}

static void GenerateBlueprintsForFile(URiveFile* RiveFile)
{
    DeletePreviousAssetsForFile(RiveFile);

    TArray<UEnum*> GeneratedEnums;
    GeneratedEnums.SetNumUninitialized(RiveFile->EnumDefinitions.Num());

    const FString FolderPath = RiveUtils::GetPackagePathForFile(RiveFile);

    for (size_t i = 0; i < RiveFile->EnumDefinitions.Num(); i++)
    {
        auto& EnumDefinition = RiveFile->EnumDefinitions[i];
        GeneratedEnums[i] =
            GenerateBlueprintForEnum(FolderPath, EnumDefinition);
    }

    for (auto& ViewModel : RiveFile->ViewModelDefinitions)
    {
        GenerateBlueprintForViewModel(FolderPath, ViewModel, GeneratedEnums);
    }
}

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

    auto Lambda = [](URiveFile* RiveFile) {
        GenerateBlueprintsForFile(RiveFile);
    };

    if (RiveFile->GetHasData())
    {
        Lambda(RiveFile);
    }
    else
    {
        RiveFile->OnDataReady.AddLambda(Lambda);
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

    if (!IsValid(RiveFile) && !ensure(GEditor))
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

    auto Lambda = [](URiveFile* RiveFile) {
        GenerateBlueprintsForFile(RiveFile);
    };

    if (RiveFile->GetHasData())
    {
        Lambda(RiveFile);
    }
    else
    {
        RiveFile->OnDataReady.AddLambda(Lambda);
    }

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetReimport(
        RiveFile);

    return EReimportResult::Succeeded;
}

int32 URiveFileFactory::GetPriority() const
{
    return DefaultImportPriority + 1;
}
