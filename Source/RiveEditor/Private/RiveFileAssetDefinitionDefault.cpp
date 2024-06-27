// Copyright Rive, Inc. All rights reserved.

#include "RiveFileAssetDefinitionDefault.h"

#include "ContentBrowserMenuContexts.h"
#include "IAssetTools.h"
// #include "RiveFileAssetToolkit.h"
#include "RiveFileAssetEditorToolkit.h"
#include "Factories/RiveWidgetFactory.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"

#include "EditorFramework/AssetImportData.h"

#define LOCTEXT_NAMESPACE "URiveFile_AssetDefinitionDefault"

namespace MenuExtension_RiveFile
{
	void ExecuteCreateWidget(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);

		TArray<URiveFile*> RiveFiles = CBContext->LoadSelectedObjects<URiveFile>();
		for (URiveFile* RiveFile : RiveFiles)
		{
			if (IsValid(RiveFile))
			{
				FRiveWidgetFactory(RiveFile).Create();
			}
		}
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []{ 
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(URiveFile::StaticClass());
		
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
				Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
				{
					if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InSection))
					{
						{
							const TAttribute<FText> Label = LOCTEXT("RiveFile_CreateWidget", "Create Rive Widget");
							const TAttribute<FText> ToolTip = LOCTEXT("RiveFile_CreateWidgetTooltip", "Creates a new Rive Widget asset using this file.");
							const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.WidgetBlueprint");
							const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateStatic(&ExecuteCreateWidget);
							InSection.AddMenuEntry("RiveFile_CreateWidget", Label, ToolTip, Icon, UIAction);
						}
					}
				}));
		}));
	});
}

FText URiveFileAssetDefinitionDefault::GetAssetDisplayName() const
{
	return LOCTEXT("AssetTypeActions_RiveFile", "Rive File");
}

FLinearColor URiveFileAssetDefinitionDefault::GetAssetColor() const
{
	return FLinearColor::Red;
}

TSoftClassPtr<> URiveFileAssetDefinitionDefault::GetAssetClass() const
{
	return URiveFile::StaticClass();
}

TConstArrayView<FAssetCategoryPath> URiveFileAssetDefinitionDefault::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Misc };

	return Categories;
}

EAssetCommandResult URiveFileAssetDefinitionDefault::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (URiveFile* RiveFile : OpenArgs.LoadObjects<URiveFile>())
	{
		const TSharedRef<FRiveFileAssetEditorToolkit> EditorToolkit = MakeShared<FRiveFileAssetEditorToolkit>();
		EditorToolkit->Initialize(RiveFile, OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost);
	}

	return EAssetCommandResult::Handled;
}

EAssetCommandResult URiveFileAssetDefinitionDefault::GetSourceFiles(const FAssetSourceFilesArgs& InArgs, TFunctionRef<bool(const FAssetSourceFilesResult& InSourceFile)> SourceFileFunc) const
{
	for (URiveFile* RiveFile : InArgs.LoadObjects<URiveFile>())
	{
		FAssetSourceFilesResult ImportInfoResult;
		ImportInfoResult.FilePath = RiveFile->AssetImportData->GetFirstFilename();
		if (SourceFileFunc(ImportInfoResult))
		{
			return EAssetCommandResult::Handled;
		}
	}
	return EAssetCommandResult::Unhandled;
}

#undef LOCTEXT_NAMESPACE
