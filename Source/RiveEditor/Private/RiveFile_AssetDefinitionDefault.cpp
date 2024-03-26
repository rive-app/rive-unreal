// Copyright Rive, Inc. All rights reserved.

#include "RiveFile_AssetDefinitionDefault.h"

#include "ContentBrowserMenuContexts.h"
#include "IAssetTools.h"
#include "RiveAssetToolkit.h"
#include "Factories/RiveFileInstanceFactory.h"
#include "Factories/RiveWidgetFactory.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"

#define LOCTEXT_NAMESPACE "URiveFile_AssetDefinitionDefault"

namespace MenuExtension_RiveFile
{
	void ExecuteCreateInstance(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);

		TArray<URiveFile*> x = CBContext->LoadSelectedObjects<URiveFile>();

		IAssetTools::Get().CreateAssetsFrom<URiveFile>(CBContext->LoadSelectedObjects<URiveFile>(), URiveFile::StaticClass(), TEXT("_Inst"), [](URiveFile* SourceObject)
		{
			UE_LOG(LogRiveEditor, Log, TEXT("Instancing Rive File: %s"), *SourceObject->GetName());
			URiveFileInstanceFactory* Factory = NewObject<URiveFileInstanceFactory>();
			Factory->InitialRiveFile = SourceObject;
			return Factory;
			
		});
	}
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
							const TAttribute<FText> Label = LOCTEXT("RiveFile_CreateInstance", "Create Rive Instance");
							const TAttribute<FText> ToolTip = LOCTEXT("RiveFile_CreateInstanceTooltip", "Creates a new Rive instance using this file.");
							const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Material");
							const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateStatic(&ExecuteCreateInstance);
							InSection.AddMenuEntry("RiveFile_CreateInstance", Label, ToolTip, Icon, UIAction);
						}
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

#undef LOCTEXT_NAMESPACE
