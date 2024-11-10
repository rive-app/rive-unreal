// Copyright Rive, Inc. All rights reserved.

#include "RiveFileEditor.h"
#include "Rive/RiveFile.h"
#include "Slate/SRiveWidget.h"
#include "PropertyEditorModule.h"
#include "Rive/RiveTextureObject.h"
#include "Widgets/Docking/SDockTab.h"

const FName FRiveFileEditor::RiveViewportTabID(TEXT("RiveViewportTabID"));
const FName FRiveFileEditor::DetailsTabID(TEXT("DetailsTabID"));
const FName FRiveFileEditor::AppIdentifier(TEXT("RiveFileApp"));

#define LOCTEXT_NAMESPACE "RiveFileAssetEditorToolkit"

FRiveFileEditor::~FRiveFileEditor()
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

void FRiveFileEditor::Initialize(URiveFile* InRiveFile,
                                 const EToolkitMode::Type InMode,
                                 const TSharedPtr<IToolkitHost>& InToolkitHost)
{
    check(InRiveFile);
    RiveFile = InRiveFile;

    // Setup our default layout
    const TSharedRef<FTabManager::FLayout> Layout =
        FTabManager::NewLayout(FName("RiveFileLayout"))
            ->AddArea(
                FTabManager::NewPrimaryArea()
                    ->SetOrientation(Orient_Vertical)
                    ->Split(
                        FTabManager::NewSplitter()
                            ->SetOrientation(Orient_Horizontal)
                            ->Split(FTabManager::NewStack()
                                        ->SetSizeCoefficient(0.7f)
                                        ->AddTab(RiveViewportTabID,
                                                 ETabState::OpenedTab)
                                        ->SetHideTabWell(true))
                            ->Split(FTabManager::NewSplitter()
                                        ->SetSizeCoefficient(0.3f)
                                        ->SetOrientation(Orient_Vertical)
                                        ->Split(FTabManager::NewStack()->AddTab(
                                            DetailsTabID,
                                            ETabState::OpenedTab)))));

    FAssetEditorToolkit::InitAssetEditor(InMode,
                                         InToolkitHost,
                                         FRiveFileEditor::AppIdentifier,
                                         Layout,
                                         true,
                                         true,
                                         InRiveFile);

    RegenerateMenusAndToolbars();
}

FText FRiveFileEditor::GetBaseToolkitName() const
{
    return LOCTEXT("RiveFileAssetAppLabel", "Rive File Editor");
}

FName FRiveFileEditor::GetToolkitFName() const
{
    return FName("RiveFileEditor");
}

FLinearColor FRiveFileEditor::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

FString FRiveFileEditor::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("WorldCentricTabPrefix", "RiveFile ").ToString();
}

void FRiveFileEditor::RegisterTabSpawners(
    const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    if (!AssetEditorTabsCategory.IsValid())
    {
        // Use the first child category of the local workspace root if there is
        // one, otherwise use the root itself
        const TArray<TSharedRef<FWorkspaceItem>>& LocalCategories =
            InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();
        AssetEditorTabsCategory =
            LocalCategories.Num() > 0
                ? LocalCategories[0]
                : InTabManager->GetLocalWorkspaceMenuRoot();
    }

    InTabManager
        ->RegisterTabSpawner(
            RiveViewportTabID,
            FOnSpawnTab::CreateSP(this,
                                  &FRiveFileEditor::SpawnTab_RiveViewportTab))
        .SetDisplayName(LOCTEXT("Viewport", "Viewport"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(),
                            "LevelEditor.Tabs.Viewports"));

    InTabManager
        ->RegisterTabSpawner(
            DetailsTabID,
            FOnSpawnTab::CreateSP(this,
                                  &FRiveFileEditor::SpawnTab_DetailsTabID))
        .SetDisplayName(LOCTEXT("Details", "Details"))
        .SetGroup(AssetEditorTabsCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(),
                            "LevelEditor.Tabs.Details"));
}

TSharedRef<SDockTab> FRiveFileEditor::SpawnTab_RiveViewportTab(
    const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == FRiveFileEditor::RiveViewportTabID);

    TSharedPtr<SWidget> ViewportWidget = nullptr;

    if (RiveFile)
    {
        if (!RiveTextureObject)
        {
            RiveTextureObject = NewObject<URiveTextureObject>();
            RiveTextureObject->Initialize(
                FRiveDescriptor{RiveFile,
                                "",
                                0,
                                "",
                                ERiveFitType::Contain,
                                ERiveAlignment::Center});
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

    TSharedRef<SDockTab> SpawnedTab =
        SNew(SDockTab)
            .Label(LOCTEXT("ViewportTab_Title", "Viewport"))
            .OnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda(
                [this](TSharedRef<SDockTab> ClosedTab) {

                }));

    SpawnedTab->SetContent(ViewportWidget.ToSharedRef());

    return SpawnedTab;
}

TSharedRef<SDockTab> FRiveFileEditor::SpawnTab_DetailsTabID(
    const FSpawnTabArgs& Args)
{
    DetailsTab = SNew(SDockTab).Label(LOCTEXT("DetailsTitle", "Details"));

    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            "PropertyEditor");

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
