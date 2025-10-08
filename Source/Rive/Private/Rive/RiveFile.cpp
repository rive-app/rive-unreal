// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"

#if WITH_EDITOR
#include "EditorCategoryUtils.h"
#include "Types/AttributeStorage.h"
#endif

#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Rive/Assets/RiveFileAssetImporter.h"
#include "Rive/Assets/RiveFileAssetLoader.h"
#include "Rive/RiveViewModel.h"
#include "Rive/RiveArtboard.h"
#include "Blueprint/UserWidget.h"
#include "Rive/RiveUtils.h"
#include "RiveRenderer.h"
#include "RiveCommandBuilder.h"
#include "IRiveRendererModule.h"
#include "ObjectEditorUtils.h"
#include "rive/command_queue.hpp"
#include "StructUtils/PropertyBag.h"

#if WITH_EDITOR
#include "EditorFramework/AssetImportData.h"
#endif

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/animation/state_machine_input.hpp"
#include "rive/renderer/render_context.hpp"
#include "Rive/command_server.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

class FRiveFileAssetImporter;
class FRiveFileAssetLoader;

static ERiveDataType RiveDataTypeFromDataType(rive::DataType type)
{
    switch (type)
    {
        case rive::DataType::none:
            return ERiveDataType::None;
        case rive::DataType::string:
            return ERiveDataType::String;
        case rive::DataType::number:
            return ERiveDataType::Number;
        case rive::DataType::boolean:
            return ERiveDataType::Boolean;
        case rive::DataType::color:
            return ERiveDataType::Color;
        case rive::DataType::list:
            return ERiveDataType::List;
        case rive::DataType::enumType:
            return ERiveDataType::EnumType;
        case rive::DataType::trigger:
            return ERiveDataType::Trigger;
        case rive::DataType::viewModel:
            return ERiveDataType::ViewModel;
        case rive::DataType::assetImage:
            return ERiveDataType::AssetImage;
        case rive::DataType::artboard:
            return ERiveDataType::Artboard;
        case rive::DataType::integer:
        case rive::DataType::symbolListIndex:
            RIVE_UNREACHABLE();
    }

    return ERiveDataType::None;
}

class FRiveFileListener final : public rive::CommandQueue::FileListener
{
public:
    FRiveFileListener(TWeakObjectPtr<URiveFile> ListeningFile) :
        ListeningFile(ListeningFile)
    {}

    virtual void onFileError(const rive::FileHandle,
                             uint64_t requestId,
                             std::string error) override
    {
        FUTF8ToTCHAR Conversion(error.c_str());
        UE_LOG(LogRive,
               Error,
               TEXT("Rive File RequestId: %llu Error {%s}"),
               requestId,
               Conversion.Get());
    }
    virtual void onViewModelsListed(
        const rive::FileHandle,
        uint64_t requestId,
        std::vector<std::string> viewModelNames) override
    {
        check(IsInGameThread());
        if (auto StrongFile = ListeningFile.Pin(); StrongFile.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongFile->ViewModelNamesListed(std::move(viewModelNames));
#endif
        }
    }

    virtual void onViewModelInstanceNamesListed(
        const rive::FileHandle,
        uint64_t requestId,
        std::string viewModelName,
        std::vector<std::string> instanceNames) override
    {
        check(IsInGameThread());
        if (auto StrongFile = ListeningFile.Pin(); StrongFile.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongFile->ViewModelInstanceNamesListed(std::move(viewModelName),
                                                     std::move(instanceNames));
#endif
        }
    }

    virtual void onViewModelPropertiesListed(
        const rive::FileHandle,
        uint64_t requestId,
        std::string viewModelName,
        std::vector<rive::CommandQueue::FileListener::ViewModelPropertyData>
            properties) override
    {
        check(IsInGameThread());
        if (auto StrongFile = ListeningFile.Pin(); StrongFile.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongFile->ViewModelPropertyDefinitionsListed(
                std::move(viewModelName),
                std::move(properties));
#endif
        }
    }

    virtual void onViewModelEnumsListed(
        const rive::FileHandle,
        uint64_t requestId,
        std::vector<rive::ViewModelEnum> enums) override
    {
        check(IsInGameThread());
        if (auto StrongFile = ListeningFile.Pin(); StrongFile.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongFile->EnumsListed(std::move(enums));
#endif
        }
    }

    virtual void onArtboardsListed(
        const rive::FileHandle Handle,
        uint64_t RequestId,
        std::vector<std::string> artboardNames) override
    {
        check(IsInGameThread());
        if (auto StrongFile = ListeningFile.Pin(); StrongFile.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongFile->ArtboardsListed(std::move(artboardNames));
#endif
        }
    }

    virtual void onFileDeleted(const rive::FileHandle Handle,
                               uint64_t RequestId) override
    {
        check(IsInGameThread());
        if (auto StrongFile = ListeningFile.Pin(); StrongFile.IsValid())
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive File Named %s deleted"),
                   *StrongFile->GetName());
        }
        else
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive File Handle %p deleted"),
                   Handle);
        }

        delete this;
    }

private:
    TWeakObjectPtr<URiveFile> ListeningFile = nullptr;
};

void URiveFile::BeginDestroy()
{
    RiveNativeFileSpan = {};

    if (!IsRunningCommandlet() && !HasAnyFlags(RF_ClassDefaultObject) &&
        NativeFileHandle != RIVE_NULL_HANDLE)
    {
        auto Renderer = IRiveRendererModule::Get().GetRenderer();
        check(Renderer);
        auto& CommandBuilder = Renderer->GetCommandBuilder();
        CommandBuilder.DestroyFile(NativeFileHandle);
    }

    Super::BeginDestroy();
}

void URiveFile::PostLoad()
{
    UObject::PostLoad();

    // Don't load stuff for default object
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return;
    }

#if WITH_EDITORONLY_DATA
    if (AssetImportData == nullptr)
    {
        AssetImportData =
            NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
    }

#endif

    if (!IsRunningCommandlet())
    {
        Initialize();
    }
}

void URiveFile::Initialize()
{
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);

    Initialize(RiveRenderer->GetCommandBuilder());
}

void URiveFile::Initialize(FRiveCommandBuilder& CommandBuilder)
{
    check(IsInGameThread());

    if (RiveNativeFileSpan.empty() || bNeedsImport)
    {
        if (RiveFileData.IsEmpty())
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("Could not load an empty Rive File Data."));
            return;
        }
        RiveNativeFileSpan.resize(RiveFileData.Num());
        FMemory::Memcpy(RiveNativeFileSpan.data(),
                        RiveFileData.GetData(),
                        RiveFileData.Num() * sizeof(uint8_t));
    }

#if WITH_EDITORONLY_DATA
    if (bNeedsImport)
    {
        ArtboardDefinitions.Empty();
        EnumDefinitions.Empty();
        ViewModelDefinitions.Empty();

        bHasArtboardData = false;
        bHasViewModelData = false;
        bHasViewModelInstanceDefaultsData = false;
        bHasEnumsData = false;

        bNeedsImport = false;
        NativeFileHandle = CommandBuilder.LoadFile(RiveNativeFileSpan,
                                                   new FRiveFileListener(this));

        CommandBuilder.RequestViewModelEnums(NativeFileHandle);
        CommandBuilder.RequestArtboardNames(NativeFileHandle);
        CommandBuilder.RequestViewModelNames(NativeFileHandle);

        return;
    }
    bHasArtboardData = true;
    bHasViewModelData = true;
    bHasEnumsData = true;
    bHasViewModelInstanceDefaultsData = true;
#endif
    NativeFileHandle = CommandBuilder.LoadFile(RiveNativeFileSpan,
                                               new FRiveFileListener(this));
}

URiveArtboard* URiveFile::CreateArtboardNamed(const FString& Name,
                                              bool inAutoBindViewModel)
{
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);

    return CreateArtboardNamed(RiveRenderer->GetCommandBuilder(),
                               Name,
                               inAutoBindViewModel);
}

URiveArtboard* URiveFile::CreateArtboardNamed(FRiveCommandBuilder& builder,
                                              const FString& Name,
                                              bool inAutoBindViewModel)
{
    auto ArtboardDefinition = GetArtboardDefinition(Name);
    if (!ArtboardDefinition)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateArtboardNamed, artboard %s not found in "
                    "file %s"),
               *Name,
               *GetName());
        return nullptr;
    }

    auto ArtboardName = MakeUniqueObjectName(this,
                                             URiveArtboard::StaticClass(),
                                             TEXT("URiveArtboard"));
    auto Artboard = NewObject<URiveArtboard>(this, ArtboardName);
    Artboard->Initialize(this,
                         *ArtboardDefinition,
                         inAutoBindViewModel,
                         builder);
    return Artboard;
}
#if WITH_EDITORONLY_DATA

void URiveFile::EnumsListed(std::vector<rive::ViewModelEnum> InEnumNames)
{
    EnumDefinitions.Empty();
    EnumDefinitions.Reserve(InEnumNames.size());
    for (const auto& Enum : InEnumNames)
    {
        FUTF8ToTCHAR Conversion(Enum.name.c_str());
        FEnumDefinition Definition;
        Definition.Name = Conversion;
        Definition.Values.Reserve(Enum.enumerants.size());
        for (const auto& EnumValue : Enum.enumerants)
        {
            FUTF8ToTCHAR ValueConversion(EnumValue.c_str());
            Definition.Values.Add(ValueConversion.Get());
        }
        EnumDefinitions.Add(MoveTemp(Definition));
    }

    bHasEnumsData = true;
    CheckShouldBroadcastDataReady();
}

void URiveFile::ArtboardsListed(std::vector<std::string> InArtboardNames)
{

    FRiveRenderer* renderer = IRiveRendererModule::Get().GetRenderer();
    check(renderer);
    FRiveCommandBuilder& CommandBuilder = renderer->GetCommandBuilder();

    ArtboardDefinitions.Empty();
    ArtboardDefinitions.Reserve(InArtboardNames.size());
    for (const auto& ArtboardName : InArtboardNames)
    {
        FUTF8ToTCHAR Conversion(ArtboardName.c_str());
        FString Name = Conversion.Get();

        FArtboardDefinition Definition;
        Definition.Name = Name;
        auto Index = ArtboardDefinitions.Add(Definition);
        // Create a temp artboard to get data about statemachines within it
        auto UniqueArtboardName =
            MakeUniqueObjectName(this,
                                 URiveArtboard::StaticClass(),
                                 TEXT("URiveArtboardTemp"));
        auto tempArtboard = NewObject<URiveArtboard>(this, UniqueArtboardName);
        // Prevents deletion from GC
        tempArtboard->AddToRoot();
        tempArtboard->InitializeForDataImport(this, Name, CommandBuilder);
        tempArtboard->OnDataReady.AddLambda(
            [this, Index, Max = InArtboardNames.size()](
                URiveArtboard* artboard) {
                ArtboardDefinitions[Index].StateMachineNames =
                    artboard->GetStateMachineNames();
                ArtboardDefinitions[Index].DefaultViewModel =
                    artboard->GetDefaultViewModel();
                ArtboardDefinitions[Index].DefaultViewModelInstance =
                    artboard->GetDefaultViewModelInstance();
                // If this is the last artboard we set getting the data to true
                // and attempt broadcast.
                if (Index == Max - 1)
                {
                    bHasArtboardData = true;
                    CheckShouldBroadcastDataReady();
                }
                // Allow GC again
                artboard->RemoveFromRoot();
            });
    }

    GenerateArtboardEnum();
}

void URiveFile::ViewModelNamesListed(std::vector<std::string> InViewModelNames)
{
    ViewModelDefinitions.Empty();
    ViewModelDefinitions.SetNum(InViewModelNames.size());
    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    FRiveCommandBuilder& builder = RiveRenderer->GetCommandBuilder();

    for (int i = 0; i < InViewModelNames.size(); i++)
    {
        FUTF8ToTCHAR Conversion(InViewModelNames[i].c_str());
        ViewModelDefinitions[i].Name = Conversion;

        builder.RequestViewModelInstanceNames(NativeFileHandle,
                                              InViewModelNames[i]);
        builder.RequestViewModelProperties(NativeFileHandle,
                                           InViewModelNames[i]);
    }

    if (InViewModelNames.empty())
    {
        bHasViewModelData = true;
        bHasViewModelInstanceDefaultsData = true;
        CheckShouldBroadcastDataReady();
    }

    GenerateViewModelEnum();
}

void URiveFile::ViewModelInstanceNamesListed(
    std::string viewModelName,
    std::vector<std::string> InInstanceNames)
{
    FUTF8ToTCHAR Conversion(viewModelName.c_str());
    FString ViewModelNames = Conversion.Get();
    auto ViewModelDefinition =
        ViewModelDefinitions.FindByPredicate([ViewModelNames](const auto& val) {
            return val.Name == ViewModelNames;
        });

    if (!ViewModelDefinition)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("ViewModel name %s not found when trying to get view model "
                    "instance names"),
               *ViewModelNames);
        return;
    }

    ViewModelDefinition->InstanceNames.Reserve(InInstanceNames.size());
    for (const auto& Name : InInstanceNames)
    {
        FUTF8ToTCHAR NameConversion(Name.c_str());
        ViewModelDefinition->InstanceNames.Add(NameConversion.Get());
    }

    GenerateViewModelInstanceEnums(*ViewModelDefinition);
}

void URiveFile::ViewModelPropertyDefinitionsListed(
    std::string viewModelName,
    std::vector<rive::CommandQueue::FileListener::ViewModelPropertyData>
        properties)
{
    FUTF8ToTCHAR Conversion(viewModelName.c_str());
    FString ViewModelName = Conversion.Get();
    auto ViewModelDefinition = ViewModelDefinitions.FindByPredicate(
        [ViewModelName](const auto& val) { return val.Name == ViewModelName; });

    if (!ViewModelDefinition)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("ViewModel name %s not found when trying to update "
                    "property definitions"),
               *ViewModelName);
        return;
    }

    ViewModelDefinition->PropertyDefinitions.SetNum(properties.size());
    for (int i = 0; i < properties.size(); i++)
    {
        FUTF8ToTCHAR PropertyNameConversion(properties[i].name.c_str());
        ViewModelDefinition->PropertyDefinitions[i].Name =
            PropertyNameConversion;
        ViewModelDefinition->PropertyDefinitions[i].Type =
            RiveDataTypeFromDataType(properties[i].type);
        if (properties[i].type == rive::DataType::enumType ||
            properties[i].type == rive::DataType::viewModel)
        {
            FUTF8ToTCHAR MetaDataConversion(properties[i].metaData.c_str());
            ViewModelDefinition->PropertyDefinitions[i].MetaData =
                MetaDataConversion;
        }
    }
    // Because everything is guaranteed order we know that the last view model
    // received is in fact the last thing we need.
    const bool bIsLastViewModel =
        (ViewModelDefinition == &ViewModelDefinitions.Last());
    // Used for the edge case that Instance Names is empty or only contains
    // invalid names and bIsLastViewModel is true.
    bool bWasUpdated = false;
    // We request the instance names before the proeprty data. So that means we
    // are garunteed the instance names here assuming nothing broke along the
    // way. This is to get the default values for the generated blueprints.
    auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
    TWeakObjectPtr<URiveFile> WeakThis(this);

    // First get the "default" instance name;
    auto DefaultViewModelHandle =
        CommandBuilder.CreateDefaultViewModel(NativeFileHandle,
                                              *ViewModelDefinition->Name);

    CommandBuilder.RunOnce([DefaultViewModelHandle, ViewModelName, WeakThis](
                               rive::CommandServer* Server) {
        auto DefaultInstance =
            Server->getViewModelInstance(DefaultViewModelHandle);
        check(DefaultInstance);
        auto DefaultInstanceName = DefaultInstance->instance()->name();
        AsyncTask(ENamedThreads::GameThread,
                  [DefaultViewModelHandle,
                   DefaultInstanceName,
                   ViewModelName,
                   WeakThis]() {
                      if (auto StrongThis = WeakThis.Pin())
                      {
                          auto ViewModel =
                              StrongThis->ViewModelDefinitions.FindByPredicate(
                                  [ViewModelName](const auto& ViewModel) {
                                      return ViewModel.Name == ViewModelName;
                                  });
                          check(ViewModel);
                          FUTF8ToTCHAR DefaultInstanceNameConversion(
                              DefaultInstanceName.c_str());
                          ViewModel->DefaultInstanceName =
                              DefaultInstanceNameConversion;
                      }
                      IRiveRendererModule::GetCommandBuilder().DestroyViewModel(
                          DefaultViewModelHandle);
                  });
    });

    for (const auto& InstanceName : ViewModelDefinition->InstanceNames)
    {
        // The editor was fixed so we should never have empty instance names
        check(!InstanceName.IsEmpty());

        bWasUpdated = true;
        const bool bCanBroadcast =
            bIsLastViewModel &&
            ViewModelDefinition->InstanceNames.Last() == InstanceName;
        // The simplest solution here is to generate each view model instance
        // and get the values with a run once and then delete each instance
        // afterwords.
        auto NativeViewModel =
            CommandBuilder.CreateViewModel(NativeFileHandle,
                                           *ViewModelDefinition->Name,
                                           *InstanceName);

        // Copy the view model property data because we are going to access it
        // on the render thread.
        auto CopyPropertyDefinitions = ViewModelDefinition->PropertyDefinitions;
        auto CopyViewModelName = ViewModelDefinition->Name;
        CommandBuilder.RunOnce([bCanBroadcast,
                                WeakThis,
                                CopyPropertyDefinitions,
                                NativeViewModel,
                                CopyViewModelName,
                                InstanceName](rive::CommandServer* Server) {
            auto NativeViewModelInstance =
                Server->getViewModelInstance(NativeViewModel);
            check(NativeViewModelInstance);
            TArray<FPropertyDefaultData> DefaultValues;
            for (const auto& Property : CopyPropertyDefinitions)
            {
                FTCHARToUTF8 UTFPropName(Property.Name);
                std::string TerminatedPropName(UTFPropName.Get(),
                                               UTFPropName.Length());
                switch (Property.Type)
                {
                    case ERiveDataType::String:
                    {
                        auto Prop = NativeViewModelInstance->propertyString(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            DefaultValues.Add({Property.Name,
                                               FString(Prop->value().c_str())});
                        }
                    }
                    break;
                    case ERiveDataType::Number:
                    {
                        auto Prop = NativeViewModelInstance->propertyNumber(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            DefaultValues.Add(
                                {Property.Name,
                                 FString::SanitizeFloat(Prop->value())});
                        }
                    }
                    break;
                    case ERiveDataType::Boolean:
                    {
                        auto Prop = NativeViewModelInstance->propertyBoolean(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            DefaultValues.Add(
                                {Property.Name,
                                 Prop->value() ? TEXT("True") : TEXT("False")});
                        }
                    }
                    break;
                    case ERiveDataType::Color:
                    {
                        auto Prop = NativeViewModelInstance->propertyColor(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            DefaultValues.Add(
                                {Property.Name,
                                 FString::FromInt(Prop->value())});
                        }
                    }
                    break;
                    case ERiveDataType::EnumType:
                    {
                        auto Prop = NativeViewModelInstance->propertyEnum(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            DefaultValues.Add({Property.Name,
                                               FString(Prop->value().c_str())});
                        }
                    }
                    break;
                    case ERiveDataType::ViewModel:
                    {
                        auto Prop = NativeViewModelInstance->propertyViewModel(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            // Value is view_model_type/instance_name
                            FString Value =
                                FString(Prop->instance()
                                            ->viewModel()
                                            ->name()
                                            .c_str()) +
                                TEXT("/") +
                                FString(Prop->instance()->name().c_str());
                            DefaultValues.Add({Property.Name, Value});
                        }
                    }
                    break;
                    case ERiveDataType::Artboard:
                    {
                        auto Prop = NativeViewModelInstance->propertyArtboard(
                            TerminatedPropName);
                        if (ensure(Prop))
                        {
                            DefaultValues.Add(
                                {Property.Name, FString(Prop->name().c_str())});
                        }
                    }
                    break;
                    case ERiveDataType::AssetImage:
                    case ERiveDataType::List:
                    default:
                        continue;
                }
            }
            // Sync back to game thread to update actual view model data.
            AsyncTask(
                ENamedThreads::GameThread,
                [bCanBroadcast,
                 WeakThis,
                 DefaultValues,
                 CopyViewModelName,
                 InstanceName,
                 NativeViewModel]() {
                    if (auto StrongThis = WeakThis.Pin())
                    {
                        auto ViewModel =
                            StrongThis->ViewModelDefinitions.FindByPredicate(
                                [CopyViewModelName](const auto& ViewModel) {
                                    return ViewModel.Name == CopyViewModelName;
                                });
                        check(ViewModel);
                        ViewModel->InstanceDefaults.Add(
                            {InstanceName, DefaultValues});

                        if (bCanBroadcast)
                        {
                            StrongThis->bHasViewModelInstanceDefaultsData =
                                true;
                            StrongThis->CheckShouldBroadcastDataReady();
                        }
                    }

                    IRiveRendererModule::GetCommandBuilder().DestroyViewModel(
                        NativeViewModel);
                });
        });
    }

    //  In the off case we don't have anything to update for the last view
    //  model, sync against the render thread and dispatch back to the game
    //  thread. This ensures we get all other view model data first.
    if (!bWasUpdated && bIsLastViewModel)
    {
        CommandBuilder.RunOnce([WeakThis](rive::CommandServer* Server) {
            AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
                if (auto StrongThis = WeakThis.Pin())
                {
                    StrongThis->bHasViewModelInstanceDefaultsData = true;
                    StrongThis->CheckShouldBroadcastDataReady();
                }
            });
        });
    }

    if (bIsLastViewModel)
    {
        bHasViewModelData = true;
        CheckShouldBroadcastDataReady();
    }
}

void URiveFile::CheckShouldBroadcastDataReady()
{
    if (bHasArtboardData && bHasViewModelData && bHasEnumsData &&
        bHasViewModelInstanceDefaultsData)
    {
        OnDataReady.Broadcast(this);
    }
}

void URiveFile::RegisterGeneratedBlueprint(
    const FName& BlueprintName,
    UBlueprintGeneratedClass* GeneratedClass)
{
    GeneratedClassMap.Add(BlueprintName, {GeneratedClass});
}
#endif
UClass* URiveFile::GetGeneratedClassForViewModel(
    const FName& ViewModelName) const
{
    if (const auto GeneratedClass = GeneratedClassMap.Find(ViewModelName))
    {
        return GeneratedClass->Entry.LoadSynchronous();
    }
    return nullptr;
}

URiveViewModel* URiveFile::CreateViewModelByName(const URiveFile* InputFile,
                                                 const FString& ViewModelName,
                                                 const FString& InstanceName)
{
    if (!ensure(IsValid(InputFile)))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByName Input File was invalid"));
        return nullptr;
    }

    if (!ensure(!ViewModelName.IsEmpty()))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByName Input ViewModelName was "
                    "empty"));
        return nullptr;
    }

    if (!ensure(!InputFile->ViewModelDefinitions.IsEmpty()))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByName Input File "
                    "ViewModelDefinitions was empty"));
        return nullptr;
    }

    auto ViewModelDefinition = InputFile->ViewModelDefinitions.FindByPredicate(
        [ViewModelName](const FViewModelDefinition& L) {
            return L.Name == ViewModelName;
        });

    if (!ensure(ViewModelDefinition))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByName Input Could not find "
                    "viewmodel with name %s"),
               *ViewModelName);
        return nullptr;
    }

    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();

    const auto SanitizedViewModelName =
        RiveUtils::SanitizeObjectName(ViewModelName + TEXT("_") + InstanceName);
    auto ViewModelClass =
        InputFile->GetGeneratedClassForViewModel(*ViewModelName);
    check(ViewModelClass);

    const auto ObjectName = MakeUniqueObjectName(ViewModelClass->GetOuter(),
                                                 ViewModelClass,
                                                 *SanitizedViewModelName);

    URiveViewModel* ViewModel = NewObject<URiveViewModel>(InputFile->GetOuter(),
                                                          ViewModelClass,
                                                          ObjectName);
    ViewModel->Initialize(Builder,
                          InputFile,
                          *ViewModelDefinition,
                          InstanceName);
    return ViewModel;
}

URiveViewModel* URiveFile::CreateViewModelByArtboardName(
    const URiveFile* InputFile,
    const FString& ArtboardName,
    const FString& InstanceName)
{
    if (!ensure(IsValid(InputFile)))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByArtboardName Input File was "
                    "invalid"));
        return nullptr;
    }

    auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    check(RiveRenderer);
    auto& Builder = RiveRenderer->GetCommandBuilder();

    FString ViewModelName;
    bool bFound = false;

    for (auto& Artboard : InputFile->ArtboardDefinitions)
    {
        if (Artboard.Name == ArtboardName)
        {
            for (auto& ViewModel : InputFile->ViewModelDefinitions)
            {
                if (ViewModel.Name == Artboard.DefaultViewModel)
                {
                    ViewModelName = Artboard.DefaultViewModel;
                    bFound = true;
                    break;
                }
            }
            if (bFound)
                break;
        }
    }

    if (!ensure(bFound) && !ensure(!ViewModelName.IsEmpty()))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByArtboardName Could not find "
                    "View Model for Artboard %s"),
               *ArtboardName);
        return nullptr;
    }

    auto ViewModelDefinition = InputFile->ViewModelDefinitions.FindByPredicate(
        [ViewModelName](const FViewModelDefinition& L) {
            return L.Name == ViewModelName;
        });

    if (!ensure(ViewModelDefinition))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveFile::CreateViewModelByName Input Could not find "
                    "viewmodel with name %s"),
               *ViewModelName);
        return nullptr;
    }

    auto SanitizedName =
        RiveUtils::SanitizeObjectName(ViewModelName + TEXT("_") + InstanceName);

    UClass* ViewModelClass =
        InputFile->GetGeneratedClassForViewModel(*ViewModelName);
    check(ViewModelClass);
    const auto ObjectName = MakeUniqueObjectName(ViewModelClass->GetOuter(),
                                                 ViewModelClass,
                                                 *SanitizedName);

    URiveViewModel* ViewModel = NewObject<URiveViewModel>(InputFile->GetOuter(),
                                                          ViewModelClass,
                                                          ObjectName);
    ViewModel->Initialize(Builder,
                          InputFile,
                          *ViewModelDefinition,
                          InstanceName);
    return ViewModel;
}

URiveViewModel* URiveFile::CreateDefaultViewModel(const URiveFile* InputFile,
                                                  const FString& ViewModelName)
{
    UE_LOG(LogRive,
           Error,
           TEXT("URiveFile::GetViewModelByName "
                "Command Qeueue not implemented view models."));
    // not implemented in command queue yet
    return nullptr;
}

URiveViewModel* URiveFile::CreateDefaultViewModelForArtboard(
    const URiveFile* InputFile,
    URiveArtboard* Artboard)
{
    UE_LOG(LogRive,
           Error,
           TEXT("URiveFile::GetDefaultArtboardViewModel "
                "Command Qeueue not implemented view models."));
    return nullptr;
}

URiveViewModel* URiveFile::CreateArtboardViewModelByName(
    const URiveFile* InputFile,
    URiveArtboard* Artboard,
    const FString& InstanceName)
{
    UE_LOG(LogRive,
           Error,
           TEXT("URiveFile::GetDefaultArtboardViewModel "
                "Command Qeueue not implemented view models."));
    return nullptr;
}

#if WITH_EDITORONLY_DATA
void URiveFile::GenerateArtboardEnum()
{
    const FString FileName = RiveUtils::SanitizeObjectName(GetName());
    const FString EnumName =
        FString::Printf(TEXT("EArtboardEnum_%s"), *FileName);

    if (!IsValid(ArtboardEnum))
    {
        ArtboardEnum = NewObject<UEnum>(GetOuter(), *EnumName, RF_Public);
    }
    else
        ViewModelEnum->SetFlags(ViewModelEnum->GetFlags() |
                                EObjectFlags::RF_Public);

    TArray<TPair<FName, int64>> EnumsValues;

    for (int64 i = 0; i < ArtboardDefinitions.Num(); ++i)
    {
        const auto& ArtboardDefinition = ArtboardDefinitions[i];
        const FString ArtboardEnumName =
            FString::Printf(TEXT("%s::%s"),
                            *EnumName,
                            *ArtboardDefinition.Name);
        EnumsValues.Add(TPair<FName, int64>(ArtboardEnumName, i));
    }
    ArtboardEnum->CppType = EnumName;
    ArtboardEnum->SetEnums(EnumsValues, UEnum::ECppForm::Namespaced);
}

void URiveFile::GenerateViewModelEnum()
{
    auto FileName = RiveUtils::SanitizeObjectName(GetName());
    const FString EnumName =
        FString::Printf(TEXT("EViewModelEnum_%s"), *FileName);
    if (!ViewModelEnum)
        ViewModelEnum = NewObject<UEnum>(GetOuter(), *EnumName, RF_Public);
    else
        ViewModelEnum->SetFlags(ViewModelEnum->GetFlags() |
                                EObjectFlags::RF_Public);

    TArray<TPair<FName, int64>> EnumsValues;
    for (int64 i = 0; i < ViewModelDefinitions.Num(); ++i)
    {
        const auto& ViewModelDefinition = ViewModelDefinitions[i];
        const FString ViewModelEnumName =
            FString::Printf(TEXT("%s::%s"),
                            *EnumName,
                            *ViewModelDefinition.Name);
        EnumsValues.Add(TPair<FName, int64>(ViewModelEnumName, i));
    }

    ViewModelEnum->CppType = EnumName;
    ViewModelEnum->SetEnums(EnumsValues, UEnum::ECppForm::Namespaced);
}

void URiveFile::GenerateViewModelInstanceEnums(
    const FViewModelDefinition& SelectedViewModel)
{
    // ensure we are using a view model of this file.
    check(ViewModelDefinitions.Find(SelectedViewModel) != INDEX_NONE);
    const FString ViewModelName = SelectedViewModel.Name;

    const FString SanitizedViewModelName =
        RiveUtils::SanitizeObjectName(ViewModelName);
    const FString SanitizedFileName = RiveUtils::SanitizeObjectName(GetName());
    const FString EnumName =
        FString::Printf(TEXT("EViewModelInstanceEnum_%s_%s"),
                        *SanitizedViewModelName,
                        *SanitizedFileName);

    // Just update the current enum instead of spawn a new one if it already
    // exists.
    UEnum* Enum = nullptr;
    TObjectPtr<UEnum>* EnumPtr = ViewModelInstanceEnums.Find(ViewModelName);
    if (EnumPtr)
    {
        Enum = *EnumPtr;
        Enum->SetFlags(Enum->GetFlags() | EObjectFlags::RF_Public);
    }
    else
    {
        Enum = NewObject<UEnum>(this, *EnumName, RF_Public);
    }

    TArray<TPair<FName, int64>> EnumsValues;
    EnumsValues.Add(TPair<FName, int64>(
        FString::Printf(TEXT("%s::%s"),
                        *EnumName,
                        *GViewModelInstanceBlankName.ToString()),
        0));
    EnumsValues.Add(TPair<FName, int64>(
        FString::Printf(TEXT("%s::%s"),
                        *EnumName,
                        *GViewModelInstanceDefaultName.ToString()),
        1));

    for (int64 i = 0; i < SelectedViewModel.InstanceNames.Num(); ++i)
    {
        EnumsValues.Add(TPair<FName, int64>(
            FString::Printf(TEXT("%s::%s"),
                            *EnumName,
                            *SelectedViewModel.InstanceNames[i]),
            i + 2));
    }

    Enum->CppType = EnumName;
    Enum->SetEnums(EnumsValues, UEnum::ECppForm::Namespaced);
    if (EnumPtr == nullptr)
    {
        ViewModelInstanceEnums.Add(ViewModelName, Enum);
    }
}
#endif
#if WITH_EDITOR
void URiveFile::PrintStats() const
{

    FFormatNamedArguments RiveFileLoadArgs;

    RiveFileLoadArgs.Add(
        TEXT("NumArtboards"),
        FText::AsNumber(static_cast<uint32>(ArtboardDefinitions.Num())));
    RiveFileLoadArgs.Add(
        TEXT("NumViewModels"),
        FText::AsNumber(static_cast<uint32>(ViewModelDefinitions.Num())));
    RiveFileLoadArgs.Add(
        TEXT("NumEnums"),
        FText::AsNumber(static_cast<uint32>(EnumDefinitions.Num())));

    const FText RiveFileLoadMsg =
        FText::Format(NSLOCTEXT("FURFile",
                                "RiveFileLoadMsg",
                                "Using Rive Runtime : {Major}.{Minor}; "
                                "Artboard(s) Count : {NumArtboards}; "
                                "Asset(s) Count : {NumAssets}; Animation(s) "
                                "Count : {NumAnimations}"),
                      RiveFileLoadArgs);

    UE_LOG(LogRive, Display, TEXT("%s"), *RiveFileLoadMsg.ToString());
}

bool URiveFile::EditorImport(const FString& InRiveFilePath,
                             TArray<uint8>& InRiveFileBuffer,
                             bool bIsReimport)
{
    bNeedsImport = true;

#if WITH_EDITORONLY_DATA
    if (AssetImportData == nullptr)
    {
        AssetImportData =
            NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
    }

    AssetImportData->Update(InRiveFilePath);

#endif

    auto Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);

    auto& CommanBuilder = Renderer->GetCommandBuilder();

    RiveFileData = MoveTemp(InRiveFileBuffer);
    if (bIsReimport)
    {
        Initialize(CommanBuilder);
    }
    else
    {
        SetFlags(RF_NeedPostLoad);
        ConditionalPostLoad();
    }

    return true;
}
#endif // WITH_EDITOR
