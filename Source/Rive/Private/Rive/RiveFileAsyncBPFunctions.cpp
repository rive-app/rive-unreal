// Copyright Rive, Inc. All rights reserved.


#include "Rive/RiveFileAsyncBPFunctions.h"

URiveFileWhenReadyAsync* URiveFileWhenReadyAsync::RiveFileWhenReadyAsync(URiveFile* RiveFile, URiveFile*& OutRiveFile)
{
	URiveFileWhenReadyAsync* AsyncAction = NewObject<URiveFileWhenReadyAsync>();
	AsyncAction->RiveFile = RiveFile;
	OutRiveFile = RiveFile;
	return AsyncAction;
}

void URiveFileWhenReadyAsync::Activate()
{
	if (IsValid(RiveFile))
	{
		RiveFile->WhenInitialized(URiveFile::FOnRiveFileInitialized::FDelegate::CreateUObject(this, &URiveFileWhenReadyAsync::OnRiveFileInitialized));
	}
	else
	{
		ReportFailed();
	}
}

void URiveFileWhenReadyAsync::OnRiveFileInitialized(URiveFile* InRiveFile, bool bSuccess)
{
	if (ensure(InRiveFile == RiveFile && IsValid(RiveFile)) && bSuccess)
	{
		ReportActivated();
	}
	else
	{
		ReportFailed();
	}
}

void URiveFileWhenReadyAsync::ReportActivated()
{
	Ready.Broadcast();
	SetReadyToDestroy();
}

void URiveFileWhenReadyAsync::ReportFailed()
{
	OnInitializedFailed.Broadcast();
	SetReadyToDestroy();
}
