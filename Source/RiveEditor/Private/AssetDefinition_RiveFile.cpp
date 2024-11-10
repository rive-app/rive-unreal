// Copyright Rive, Inc. All rights reserved.

#include "AssetDefinition_RiveFile.h"

#include "ContentBrowserMenuContexts.h"
#include "IAssetTools.h"
// #include "RiveFileAssetToolkit.h"
#include "RiveFileEditor.h"
#include "Factories/RiveWidgetFactory.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"

#include "EditorFramework/AssetImportData.h"
#include "Factories/RiveTextureObjectFactory.h"

#define LOCTEXT_NAMESPACE "URiveFile_AssetDefinitionDefault"

namespace MenuExtension_RiveFile
{
void ExecuteCreateWidget(const FToolMenuContext& InContext)
{
    const UContentBrowserAssetContextMenuContext* CBContext =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(
            InContext);

    TArray<URiveFile*> RiveFiles = CBContext->LoadSelectedObjects<URiveFile>();
    for (URiveFile* RiveFile : RiveFiles)
    {
        if (IsValid(RiveFile))
        {
            FRiveWidgetFactory(RiveFile).Create();
        }
    }
}

void ExecuteCreateTextureObject(const FToolMenuContext& InContext)
{
    const UContentBrowserAssetContextMenuContext* CBContext =
        UContentBrowserAssetContextMenuContext::FindContextWithAssets(
            InContext);

    TArray<URiveFile*> RiveFileObjects =
        CBContext->LoadSelectedObjects<URiveFile>();
    for (URiveFile* RiveFile : RiveFileObjects)
    {
        if (IsValid(RiveFile))
        {
            FRiveTextureObjectFactory(RiveFile).Create();
        }
    }
}

static FDelayedAutoRegisterHelper DelayedAutoRegister(
    EDelayedRegisterRunPhase::EndOfEngineInit,
    [] {
        UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateLambda([]() {
                FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
                UToolMenu* Menu =
                    UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(
                        URiveFile::StaticClass());

                FToolMenuSection& Section =
                    Menu->FindOrAddSection("GetAssetActions");
                Section.AddDynamicEntry(
                    NAME_None,
                    FNewToolMenuSectionDelegate::CreateLambda(
                        [](FToolMenuSection& InSection) {
                            if (const UContentBrowserAssetContextMenuContext*
                                    Context =
                                        UContentBrowserAssetContextMenuContext::
                                            FindContextWithAssets(InSection))
                            {
                                {
                                    const TAttribute<FText> Label =
                                        LOCTEXT("RiveFile_CreateWidget",
                                                "Create Rive Widget");
                                    const TAttribute<FText> ToolTip =
                                        LOCTEXT("RiveFile_CreateWidgetTooltip",
                                                "Creates a new Rive Widget "
                                                "asset using this file.");
                                    const FSlateIcon Icon = FSlateIcon(
                                        FAppStyle::GetAppStyleSetName(),
                                        "ClassIcon.WidgetBlueprint");
                                    const FToolMenuExecuteAction UIAction =
                                        FToolMenuExecuteAction::CreateStatic(
                                            &ExecuteCreateWidget);
                                    InSection.AddMenuEntry(
                                        "RiveFile_CreateWidget",
                                        Label,
                                        ToolTip,
                                        Icon,
                                        UIAction);
                                }

                                {
                                    const TAttribute<FText> Label =
                                        LOCTEXT("RiveFile_CreateTextureObject",
                                                "Create Rive Texture Object");
                                    const TAttribute<FText> ToolTip =
                                        LOCTEXT("RiveFile_CreateWidgetTooltip",
                                                "Creates a new Rive Texture "
                                                "Object using this file.");
                                    const FSlateIcon Icon = FSlateIcon(
                                        FAppStyle::GetAppStyleSetName(),
                                        "ClassIcon.Texture2D");
                                    const FToolMenuExecuteAction UIAction =
                                        FToolMenuExecuteAction::CreateStatic(
                                            &ExecuteCreateTextureObject);
                                    InSection.AddMenuEntry(
                                        "RiveFile_CreateTextureObject",
                                        Label,
                                        ToolTip,
                                        Icon,
                                        UIAction);
                                }
                            }
                        }));
            }));
    });
} // namespace MenuExtension_RiveFile

FText UAssetDefinition_RiveFile::GetAssetDisplayName() const
{
    return LOCTEXT("AssetTypeActions_RiveFile", "Rive File");
}

FLinearColor UAssetDefinition_RiveFile::GetAssetColor() const
{
    return FLinearColor::Red;
}

TSoftClassPtr<> UAssetDefinition_RiveFile::GetAssetClass() const
{
    return URiveFile::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_RiveFile::
    GetAssetCategories() const
{
    static const FAssetCategoryPath Categories[] = {EAssetCategoryPaths::Misc};

    return Categories;
}

EAssetCommandResult UAssetDefinition_RiveFile::OpenAssets(
    const FAssetOpenArgs& OpenArgs) const
{
    for (URiveFile* RiveFile : OpenArgs.LoadObjects<URiveFile>())
    {
        const TSharedRef<FRiveFileEditor> EditorToolkit =
            MakeShared<FRiveFileEditor>();
        EditorToolkit->Initialize(RiveFile,
                                  OpenArgs.GetToolkitMode(),
                                  OpenArgs.ToolkitHost);
    }

    return EAssetCommandResult::Handled;
}

EAssetCommandResult UAssetDefinition_RiveFile::GetSourceFiles(
    const FAssetSourceFilesArgs& InArgs,
    TFunctionRef<bool(const FAssetSourceFilesResult& InSourceFile)>
        SourceFileFunc) const
{
    for (URiveFile* RiveFile : InArgs.LoadObjects<URiveFile>())
    {
        FAssetSourceFilesResult ImportInfoResult;
        ImportInfoResult.FilePath =
            RiveFile->AssetImportData->GetFirstFilename();
        if (SourceFileFunc(ImportInfoResult))
        {
            return EAssetCommandResult::Handled;
        }
    }
    return EAssetCommandResult::Unhandled;
}

#undef LOCTEXT_NAMESPACE
