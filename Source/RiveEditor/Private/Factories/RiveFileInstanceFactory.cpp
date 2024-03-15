// Copyright Rive, Inc. All rights reserved.


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

	// Finding the real parent, otherwise an instance could have another instance as parent
	URiveFile* RiveFile = InitialRiveFile;
	while (IsValid(RiveFile) && RiveFile->ParentRiveFile)
	{
		RiveFile = RiveFile->ParentRiveFile;
	}

	if (!ensure(IsValid(RiveFile)))
	{
		UE_LOG(LogRiveEditor, Error, TEXT("Unable to find a valid parent of Rive file '%s'"), *GetFullNameSafe(InitialRiveFile));
		return nullptr;
	}
	
	URiveFile* NewRiveFileInstance = NewObject<URiveFile>(InParent, URiveFile::StaticClass(), InName, RF_Public | RF_Standalone);
	NewRiveFileInstance->ParentRiveFile = RiveFile;

	TArray<uint8> EmptyData{};
	if (!NewRiveFileInstance->EditorImport(FString{},EmptyData))
	{
		UE_LOG(LogRiveEditor, Error, TEXT("Could not create instance of RiveFile '%s'"), *GetFullNameSafe(InitialRiveFile));
		return nullptr;
	}
	
	return NewRiveFileInstance;
}
