// Copyright Rive, Inc. All rights reserved.

#include "RiveAssetEditor.h"
#include "RiveAssetToolkit.h"
#include "Tools/BaseAssetToolkit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RiveAssetEditor)

void URiveAssetEditor::GetObjectsToEdit(TArray<UObject*>& OutObjectsToEdit)
{
    OutObjectsToEdit.Add(ObjectToEdit);
}

TSharedPtr<FBaseAssetToolkit> URiveAssetEditor::CreateToolkit()
{
    return MakeShared<FRiveAssetToolkit>(this);
}

void URiveAssetEditor::SetObjectToEdit(UObject* InObject)
{
    ObjectToEdit = InObject;
}
