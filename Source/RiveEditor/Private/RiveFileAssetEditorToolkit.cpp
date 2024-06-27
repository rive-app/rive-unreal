// Copyright Rive, Inc. All rights reserved.

#include "RiveFileAssetEditorToolkit.h"
#include "Rive/RiveFile.h"
#include "Slate/SRiveWidget.h"
#include "PropertyEditorModule.h"
#include "Rive/RiveTextureObject.h"
#include "Widgets/Docking/SDockTab.h"

const FName FRiveFileAssetEditorToolkit::RiveViewportTabID(TEXT("RiveViewportTabID"));
const FName FRiveFileAssetEditorToolkit::DetailsTabID(TEXT("DetailsTabID"));
const FName FRiveFileAssetEditorToolkit::AppIdentifier(TEXT("RiveFileApp"));

#define LOCTEXT_NAMESPACE "RiveFileAssetEditorToolkit"

FRiveFileAssetEditorToolkit::~FRiveFileAssetEditorToolkit()
{
    if (RiveWidget)
    {
        RiveWidget->SetRiveTexture(nullptr);
        RiveWidget.Reset();
    }
    
    if (RiveTextureObject)
    {
        RiveTextureObject->bRenderInEditor = false;
        RiveTextureObject->MarkAsGarbage();
        RiveTextureObject->ConditionalBeginDestroy();
        RiveTextureObject = nullptr;
    }
}

void FRiveFileAssetEditorToolkit::Initialize(URiveFile* InRiveFile, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost)
{
    check(InRiveFile);
    RiveFile = InRiveFile;
    
    // Setup our default layout
    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(FName("RiveFileLayout"))
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
        FRiveFileAssetEditorToolkit::AppIdentifier,
        Layout,
        true,
        true,
        InRiveFile
    );
    
    RegenerateMenusAndToolbars();
}

FText FRiveFileAssetEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("RiveFileAssetAppLabel", "Rive File Editor");
}

FName FRiveFileAssetEditorToolkit::GetToolkitFName() const
{
    return FName("RiveFileEditor");
}

FLinearColor FRiveFileAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

FString FRiveFileAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("WorldCentricTabPrefix", "RiveFile ").ToString();
}

void FRiveFileAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    
    if (!AssetEditorTabsCategory.IsValid())
    {
        // Use the first child category of the local workspace root if there is one, otherwise use the root itself
        const TArray<TSharedRef<FWorkspaceItem>>& LocalCategories = InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();
        AssetEditorTabsCategory = LocalCategories.Num() > 0 ? LocalCategories[0] : InTabManager->GetLocalWorkspaceMenuRoot();
    }
    
    InTabManager->RegisterTabSpawner(RiveViewportTabID, FOnSpawnTab::CreateSP(this, &FRiveFileAssetEditorToolkit::SpawnTab_RiveViewportTab))
        .SetDisplayName(LOCTEXT("Viewport", "Viewport"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
    
    InTabManager->RegisterTabSpawner(DetailsTabID, FOnSpawnTab::CreateSP(this, &FRiveFileAssetEditorToolkit::SpawnTab_DetailsTabID))
        .SetDisplayName(LOCTEXT("Details", "Details"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

TSharedRef<SDockTab> FRiveFileAssetEditorToolkit::SpawnTab_RiveViewportTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == FRiveFileAssetEditorToolkit::RiveViewportTabID);

    TSharedPtr<SWidget> ViewportWidget = nullptr;

    if (RiveFile)
    {
        if (!RiveTextureObject)
        {
            RiveTextureObject = NewObject<URiveTextureObject>();
            RiveTextureObject->Initialize(FRiveDescriptor{
                RiveFile,
                "",
                0,
                "",
                ERiveFitType::Contain,
                ERiveAlignment::Center
            });
        }

        RiveTextureObject->bRenderInEditor = true;
        RiveWidget = SNew(SRiveWidget)
#if WITH_EDITOR
            .bDrawCheckerboardInEditor(true);
#endif
        RiveWidget->SetRiveTexture(RiveTextureObject);
        ViewportWidget = RiveWidget;
    }
    else
    {
        ViewportWidget = SNullWidget::NullWidget;
    }

    TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
    .Label(LOCTEXT("ViewportTab_Title", "Viewport"))
    .OnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([this](TSharedRef<SDockTab> ClosedTab) {
        
    }));
    
    SpawnedTab->SetContent(ViewportWidget.ToSharedRef());

    return SpawnedTab;
}

TSharedRef<SDockTab> FRiveFileAssetEditorToolkit::SpawnTab_DetailsTabID(const FSpawnTabArgs& Args)
{
    DetailsTab = SNew(SDockTab).Label(LOCTEXT("DetailsTitle", "Details"));

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bAllowSearch = true;
    DetailsAssetView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    
    if (RiveFile)
    {
        DetailsAssetView->SetObject(RiveFile);
    }
    
    DetailsTab->SetContent(DetailsAssetView.ToSharedRef());

    return DetailsTab.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
