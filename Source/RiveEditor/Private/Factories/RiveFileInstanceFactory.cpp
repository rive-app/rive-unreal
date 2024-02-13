// Fill out your copyright notice in the Description page of Project Settings.


#include "RiveFileInstanceFactory.h"

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

	auto NewRiveFileInstance = NewObject<URiveFile>(InParent, URiveFile::StaticClass(), InName, RF_Public | RF_Standalone);
	NewRiveFileInstance->ParentRiveFile = InitialRiveFile;
	NewRiveFileInstance->Initialize();
	return NewRiveFileInstance;
}
