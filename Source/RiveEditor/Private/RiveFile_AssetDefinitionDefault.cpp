// Copyright Rive, Inc. All rights reserved.

#include "RiveFile_AssetDefinitionDefault.h"

#include "ContentBrowserMenuContexts.h"
#include "IAssetTools.h"
#include "RiveAssetToolkit.h"
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

FText URiveFile_AssetDefinitionDefault::GetAssetDisplayName() const
{
	return LOCTEXT("AssetTypeActions_RiveFile", "Rive File");
}

FLinearColor URiveFile_AssetDefinitionDefault::GetAssetColor() const
{
	return FLinearColor::Red;
}

TSoftClassPtr<> URiveFile_AssetDefinitionDefault::GetAssetClass() const
{
	return URiveFile::StaticClass();
}

TConstArrayView<FAssetCategoryPath> URiveFile_AssetDefinitionDefault::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Misc };

	return Categories;
}

EAssetCommandResult URiveFile_AssetDefinitionDefault::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (URiveFile* RiveFile : OpenArgs.LoadObjects<URiveFile>())
	{
		const TSharedRef<FRiveAssetToolkit> EditorToolkit = MakeShared<FRiveAssetToolkit>();
		EditorToolkit->Initialize(RiveFile, OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost);
	}

	return EAssetCommandResult::Handled;
}

EAssetCommandResult URiveFile_AssetDefinitionDefault::GetSourceFiles(const FAssetSourceFilesArgs& InArgs, TFunctionRef<bool(const FAssetSourceFilesResult& InSourceFile)> SourceFileFunc) const
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
