// Copyright Rive, Inc. All rights reserved.

#include "RiveWidgetFactory.h"

#include "ContentBrowserMenuContexts.h"
#include "PackageTools.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Game/RiveActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"
#include "Templates/WidgetTemplateClass.h"
#include "UMG/RiveWidget.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "FRiveWidgetFactory"

FRiveWidgetFactory::FRiveWidgetFactory(URiveFile* InRiveFile)
	: RiveFile(InRiveFile)
{
}

UE_DISABLE_OPTIMIZATION

bool FRiveWidgetFactory::SaveAsset(UWidgetBlueprint* InWidgetBlueprint)
{
	if (!InWidgetBlueprint)
	{
		return false;
	}

	// auto-save asset outside of the editor
	UPackage* const Package = InWidgetBlueprint->GetOutermost();

	FString const PackageName = Package->GetName();

	FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	if (IFileManager::Get().IsReadOnly(*PackageFileName))
	{
		UE_LOG(LogRiveEditor, Error, TEXT("Could not save read only file: %s"), *PackageFileName);

		return false;
	}

	double StartTime = FPlatformTime::Seconds();

	UMetaData *MetaData = Package->GetMetaData();

	FSavePackageArgs SaveArgs;

	SaveArgs.TopLevelFlags = RF_Standalone;

	SaveArgs.SaveFlags = SAVE_NoError | SAVE_Async;

	UPackage::SavePackage(Package, NULL, *PackageFileName, SaveArgs);

	const double ElapsedTime = FPlatformTime::Seconds() - StartTime;

	UE_LOG(LogRiveEditor, Log, TEXT("Saved %s in %0.2f seconds"), *PackageName, ElapsedTime);

	Package->MarkPackageDirty();

	return true;
}

bool FRiveWidgetFactory::CreateWidgetStructure(UWidgetBlueprint* InWidgetBlueprint)
{
	check(InWidgetBlueprint);
	
	// Create the desired root widget specified by the project
	if ( InWidgetBlueprint->WidgetTree->RootWidget == nullptr )
	{
		UWidget* Root = InWidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(UCanvasPanel::StaticClass());

		InWidgetBlueprint->WidgetTree->RootWidget = Root;

		UWidget* Widget = FWidgetTemplateClass(URiveWidget::StaticClass()).Create(InWidgetBlueprint->WidgetTree);
		
		if (URiveWidget* RiveWidget = Cast<URiveWidget>(Widget))
		{
			RiveWidget->RiveFile = RiveFile;
		}
		
		if (UCanvasPanel* RootWidget = Cast<UCanvasPanel>(Root))
		{
			UCanvasPanelSlot* NewSlot = RootWidget->AddChildToCanvas(Widget);

			check(NewSlot);

			NewSlot->SetAnchors(FAnchors(0, 0, 1, 1));

			NewSlot->SetOffsets(FMargin(0,0,0,0));
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(InWidgetBlueprint);

		return true;
	}

	return false;
}

UWidgetBlueprint* FRiveWidgetFactory::CreateWidgetBlueprint()
{
	UClass* CurrentParentClass = UUserWidget::StaticClass();

	if ((CurrentParentClass == nullptr)
		|| !FKismetEditorUtilities::CanCreateBlueprintOfClass(CurrentParentClass)
		|| !CurrentParentClass->IsChildOf(UUserWidget::StaticClass()))
	{
		FFormatNamedArguments Args;

		Args.Add(TEXT("ClassName"), CurrentParentClass
			                            ? FText::FromString(CurrentParentClass->GetName())
			                            : LOCTEXT("Null", "(null)"));

		FMessageDialog::Open(EAppMsgType::Ok,
		                     FText::Format(LOCTEXT("CannotCreateWidgetBlueprint",
		                                           "Cannot create a Widget Blueprint based on the class '{ClassName}'."), Args));
		return nullptr;
	}
	
	const UPackage* const RivePackage = RiveFile->GetOutermost();

	const FString RivePackageName = RivePackage->GetName();

	const FString RiveWidgetFolderName = *FPackageName::GetLongPackagePath(RivePackageName);

	const FName RiveWidgetName = MakeUniqueObjectName(RiveFile, URiveWidget::StaticClass(), *(RiveFile->GetName() + FString("_Widget")));

	const FString PackageName = UPackageTools::SanitizePackageName(	RiveWidgetFolderName + TEXT("/") + RiveWidgetName.ToString());
	
	UPackage* RiveWidgetPackage = CreatePackage(*PackageName);
		
	return CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		CurrentParentClass, RiveWidgetPackage, RiveWidgetName, BPTYPE_Interface, UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass()));
}

bool FRiveWidgetFactory::Create()
{
	UWidgetBlueprint* NewBP = CreateWidgetBlueprint();

	if (!NewBP)
	{
		return false;
	}

	if (!SaveAsset(NewBP))
	{
		return false;
	}

	if (!CreateWidgetStructure(NewBP))
	{
		return false;
	}
	
	RiveFile->SetWidgetClass(TSubclassOf<UUserWidget>(NewBP->GeneratedClass));

	// Compile BP
	FCompilerResultsLog LogResults;

	constexpr EBlueprintCompileOptions CompileOptions = EBlueprintCompileOptions::None;

	FKismetEditorUtilities::CompileBlueprint(NewBP, CompileOptions, &LogResults);

	return true;
}

namespace
{
	static void SpawnRiveWidgetActor(const FToolMenuContext& MenuContext)
	{
		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
		{
			TArray<URiveFile*> RiveFiles = Context->LoadSelectedObjects<URiveFile>();
			for (URiveFile* RiveFile : RiveFiles)
			{
				if (UWorld* World = GEditor->GetEditorWorldContext().World())
				{
					ARiveActor* NewActor = Cast<ARiveActor>(
						World->SpawnActor<ARiveActor>(FVector::ZeroVector, FRotator::ZeroRotator,
						                              FActorSpawnParameters()));
					
					if (NewActor)
					{
						NewActor->RiveFile = RiveFile;
						NewActor->SetWidgetClass(RiveFile->GetWidgetClass());
					}
				}
			}
		}
	}
	
	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []{ 
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UTextureRenderTarget::StaticClass());
	        
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
			Section.AddDynamicEntry(TEXT("Rive"), FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				{
					const TAttribute<FText> Label = LOCTEXT("RiveFile_SpawnRiveWidget", "Spawn Rive Widget Actor");
					const TAttribute<FText> ToolTip = LOCTEXT("RiveFile_SpawnRiveWidgetTooltip", "Spawn Rive Widget Actors.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Texture2D");
					const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateStatic(&SpawnRiveWidgetActor);

					InSection.AddMenuEntry("RiveFile_SpawnRiveWidget", Label, ToolTip, Icon, UIAction);
				}
			}));
		}));
	});	
}

#undef LOCTEXT_NAMESPACE

UE_ENABLE_OPTIMIZATION
