// Copyright Rive, Inc. All rights reserved.

#include "RiveAssetToolkit.h"
#include "Rive/RiveFile.h"
#include "Slate/SRiveWidget.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"

const FName FRiveAssetToolkit::RiveViewportTabID(TEXT("RiveViewportTabID"));
const FName FRiveAssetToolkit::DetailsTabID(TEXT("DetailsTabID"));
const FName FRiveAssetToolkit::AppIdentifier(TEXT("RiveFileApp"));

#define LOCTEXT_NAMESPACE "FRiveAssetToolkit"

void FRiveAssetToolkit::Initialize(URiveFile* InRiveFile, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost)
{
    check(InRiveFile);
    RiveFile = InRiveFile;
    
    // Setup our default layout
    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(FName("RiveFileEditorEditorLayout3"))
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
        FRiveAssetToolkit::AppIdentifier,
        Layout,
        true,
        true,
        InRiveFile
    );
    
    RegenerateMenusAndToolbars();
}

FText FRiveAssetToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("AppLabel", "Rive File Editor");
}

FName FRiveAssetToolkit::GetToolkitFName() const
{
    return FName("RiveFileEditor");
}

FLinearColor FRiveAssetToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

FString FRiveAssetToolkit::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("WorldCentricTabPrefix", "RiveFile ").ToString();
}

void FRiveAssetToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    
    if (!AssetEditorTabsCategory.IsValid())
    {
        // Use the first child category of the local workspace root if there is one, otherwise use the root itself
        const TArray<TSharedRef<FWorkspaceItem>>& LocalCategories = InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();
        AssetEditorTabsCategory = LocalCategories.Num() > 0 ? LocalCategories[0] : InTabManager->GetLocalWorkspaceMenuRoot();
    }
    
    InTabManager->RegisterTabSpawner(RiveViewportTabID, FOnSpawnTab::CreateSP(this, &FRiveAssetToolkit::SpawnTab_RiveViewportTab))
        .SetDisplayName(LOCTEXT("Viewport", "Viewport"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
    
    InTabManager->RegisterTabSpawner(DetailsTabID, FOnSpawnTab::CreateSP(this, &FRiveAssetToolkit::SpawnTab_DetailsTabID))
        .SetDisplayName(LOCTEXT("Details", "Details"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

TSharedRef<SDockTab> FRiveAssetToolkit::SpawnTab_RiveViewportTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == FRiveAssetToolkit::RiveViewportTabID);

    TSharedPtr<SWidget> ViewportWidget = nullptr;

    if (RiveFile)
    {
        RiveWidget = SNew(SRiveWidget)
#if WITH_EDITOR
            .bDrawCheckerboardInEditor(true);
#endif
        RiveWidget->SetRiveFile(RiveFile);
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

TSharedRef<SDockTab> FRiveAssetToolkit::SpawnTab_DetailsTabID(const FSpawnTabArgs& Args)
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
