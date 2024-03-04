// Fill out your copyright notice in the Description page of Project Settings.


#include "RiveFileInstanceFactory.h"

#include "IRiveRendererModule.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"

URiveFileInstanceFactory::URiveFileInstanceFactory(const FObjectInitializer& ObjectInitializer)
{
	SupportedClass = URiveFile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* URiveFileInstanceFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags,
                                                    UObject* Context, FFeedbackContext* Warn)
{
	if (!UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
	{
		UE_LOG(LogRiveEditor, Error, TEXT("RiveRenderer is null, unable to create and instance of the Rive file '%s'"), *GetFullNameSafe(InitialRiveFile));
		return nullptr;
	}
	
	auto NewRiveFileInstance = NewObject<URiveFile>(InParent, URiveFile::StaticClass(), InName, RF_Public | RF_Standalone);
	NewRiveFileInstance->ParentRiveFile = InitialRiveFile;

	TArray<uint8> EmptyData{};
	if (!NewRiveFileInstance->EditorImport(FString{},EmptyData))
	{
		UE_LOG(LogRiveEditor, Error, TEXT("Could not create instance of RiveFile '%s'"), *GetFullNameSafe(InitialRiveFile));
		return nullptr;
	}
	
	return NewRiveFileInstance;
}
