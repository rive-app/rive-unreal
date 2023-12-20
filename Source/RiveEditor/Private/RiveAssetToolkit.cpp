// Copyright Rive, Inc. All rights reserved.

#include "RiveAssetToolkit.h"
#include "Rive/RiveFile.h"
#include "Slate/SRiveWidget.h"

const FName FRiveAssetToolkit::RiveViewportTabID(TEXT("RiveViewportTabID"));

const FName FRiveAssetToolkit::DetailsTabID(TEXT("DetailsTabID"));

#define LOCTEXT_NAMESPACE "FRiveAssetToolkit"

FRiveAssetToolkit::FRiveAssetToolkit(UAssetEditor* InOwningAssetEditor)
    : FBaseAssetToolkit(InOwningAssetEditor)
{
    // Setup our default layout
    StandaloneDefaultLayout = FTabManager::NewLayout(FName("SmartObjectAssetEditorLayout3"))
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
}

FRiveAssetToolkit::~FRiveAssetToolkit()
{
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

void FRiveAssetToolkit::Tick(float DeltaTime)
{
}

TStatId FRiveAssetToolkit::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FRiveAssetToolkit, STATGROUP_Tickables);
}

TSharedRef<SDockTab> FRiveAssetToolkit::SpawnTab_RiveViewportTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == FRiveAssetToolkit::RiveViewportTabID);

    TSharedPtr<SWidget> ViewportWidget = nullptr;

    if (URiveFile* RiveFile = CastChecked<URiveFile>(GetEditingObject()))
    {
        RiveWidget = SNew(SRiveWidget, RiveFile);

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

    if (URiveFile* RiveFile = CastChecked<URiveFile>(GetEditingObject()))
    {
        DetailsAssetView->SetObject(RiveFile);
    }

    DetailsTab->SetContent(DetailsAssetView.ToSharedRef());

    return DetailsTab.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
