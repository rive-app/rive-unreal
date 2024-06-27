// Copyright Rive, Inc. All rights reserved.

#include "RiveTextureObjectAssetEditorToolkit.h"
#include "Slate/SRiveWidget.h"
#include "PropertyEditorModule.h"
#include "Rive/RiveTextureObject.h"
#include "Widgets/Docking/SDockTab.h"

const FName FRiveTextureObjectAssetEditorToolkit::RiveViewportTabID(TEXT("RiveTextureObjectViewportTabID"));
const FName FRiveTextureObjectAssetEditorToolkit::DetailsTabID(TEXT("DetailsTabID"));
const FName FRiveTextureObjectAssetEditorToolkit::AppIdentifier(TEXT("RiveTextureObjectApp"));

#define LOCTEXT_NAMESPACE "FRiveTextureObjectAssetToolkit"

FRiveTextureObjectAssetEditorToolkit::~FRiveTextureObjectAssetEditorToolkit()
{
    if (RiveWidget)
    {
        RiveWidget->SetRiveTexture(nullptr);
        RiveWidget.Reset();
    }

    if (RiveTextureObject)
    {
        RiveTextureObject->bRenderInEditor = false;
        RiveTextureObject = nullptr;
    }
}

void FRiveTextureObjectAssetEditorToolkit::Initialize(URiveTextureObject* InRiveTextureObject, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost)
{
    check(InRiveTextureObject);
    RiveTextureObject = InRiveTextureObject;
    
    // Setup our default layout
    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(FName("RiveTextureObjectLayout"))
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            ->Split
            (
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Horizontal)
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.7f)
                    ->AddTab(RiveViewportTabID, ETabState::OpenedTab)
                    ->SetHideTabWell(true)
                )
                ->Split
                (
                    FTabManager::NewSplitter()
                    ->SetSizeCoefficient(0.3f)
                    ->SetOrientation(Orient_Vertical)
                    ->Split
                    (
                        FTabManager::NewStack()
                        ->AddTab(DetailsTabID, ETabState::OpenedTab)
                    )
                )
            )
        );
    
    FAssetEditorToolkit::InitAssetEditor(
        InMode,
        InToolkitHost,
        FRiveTextureObjectAssetEditorToolkit::AppIdentifier,
        Layout,
        true,
        true,
        RiveTextureObject
    );

    RiveTextureObject->bRenderInEditor = true;
    
    RegenerateMenusAndToolbars();
}

FText FRiveTextureObjectAssetEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("RiveTextureObjectAppLabel", "Rive Texture Object Editor");
}

FName FRiveTextureObjectAssetEditorToolkit::GetToolkitFName() const
{
    return FName("RiveTextureObjectEditor");
}

FLinearColor FRiveTextureObjectAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

FString FRiveTextureObjectAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("WorldCentricTabPrefix", "RiveTextureObject ").ToString();
}

void FRiveTextureObjectAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    
    if (!AssetEditorTabsCategory.IsValid())
    {
        // Use the first child category of the local workspace root if there is one, otherwise use the root itself
        const TArray<TSharedRef<FWorkspaceItem>>& LocalCategories = InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();
        AssetEditorTabsCategory = LocalCategories.Num() > 0 ? LocalCategories[0] : InTabManager->GetLocalWorkspaceMenuRoot();
    }
    
    InTabManager->RegisterTabSpawner(RiveViewportTabID, FOnSpawnTab::CreateSP(this, &FRiveTextureObjectAssetEditorToolkit::SpawnTab_RiveViewportTab))
        .SetDisplayName(LOCTEXT("Viewport", "Viewport"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
    
    InTabManager->RegisterTabSpawner(DetailsTabID, FOnSpawnTab::CreateSP(this, &FRiveTextureObjectAssetEditorToolkit::SpawnTab_DetailsTabID))
        .SetDisplayName(LOCTEXT("Details", "Details"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

TSharedRef<SDockTab> FRiveTextureObjectAssetEditorToolkit::SpawnTab_RiveViewportTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == FRiveTextureObjectAssetEditorToolkit::RiveViewportTabID);

    TSharedPtr<SWidget> ViewportWidget = nullptr;
    
    if (RiveTextureObject)
    {
        RiveWidget = SNew(SRiveWidget).bDrawCheckerboardInEditor(true);
        RiveWidget->SetRiveTexture(RiveTextureObject);
        ViewportWidget = RiveWidget;
    }
    else
    {
        ViewportWidget = SNullWidget::NullWidget;
    }

    TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).Label(LOCTEXT("ViewportTab_Title", "Viewport"));
    
    SpawnedTab->SetContent(ViewportWidget.ToSharedRef());

    return SpawnedTab;
}

TSharedRef<SDockTab> FRiveTextureObjectAssetEditorToolkit::SpawnTab_DetailsTabID(const FSpawnTabArgs& Args)
{
    DetailsTab = SNew(SDockTab).Label(LOCTEXT("DetailsTitle", "Details"));

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bAllowSearch = true;
    DetailsAssetView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    
    if (RiveTextureObject)
    {
        DetailsAssetView->SetObject(RiveTextureObject);
    }
    
    DetailsTab->SetContent(DetailsAssetView.ToSharedRef());

    return DetailsTab.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
