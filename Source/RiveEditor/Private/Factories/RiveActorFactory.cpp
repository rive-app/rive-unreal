// Copyright Rive, Inc. All rights reserved.

#include "RiveActorFactory.h"

#include "Blueprint/UserWidget.h"
#include "Game/RiveWidgetActor.h"
#include "Rive/RiveFile.h"
#include "UMG/RiveWidget.h"

URiveActorFactory::URiveActorFactory()
{
    DisplayName =
        NSLOCTEXT("RiveActorFactory", "RiveActorDisplayName", "RiveActor");
    NewActorClass = ARiveWidgetActor::StaticClass();
}

AActor* URiveActorFactory::SpawnActor(
    UObject* InAsset,
    ULevel* InLevel,
    const FTransform& InTransform,
    const FActorSpawnParameters& InSpawnParams)
{
    ARiveWidgetActor* NewActor = Cast<ARiveWidgetActor>(
        Super::SpawnActor(InAsset, InLevel, InTransform, InSpawnParams));

    if (NewActor)
    {
        if (URiveFile* RiveFile = Cast<URiveFile>(InAsset))
        {
            NewActor->SetWidgetClass(RiveFile->GetWidgetClass());
        }
    }

    return NewActor;
}

bool URiveActorFactory::CanCreateActorFrom(const FAssetData& AssetData,
                                           FText& OutErrorMsg)
{
    if (AssetData.IsValid() &&
        !AssetData.GetClass()->IsChildOf(URiveFile::StaticClass()))
    {
        OutErrorMsg = NSLOCTEXT("CanCreateActor",
                                "NoRiveFileAsset",
                                "A valid rive file asset must be specified.");

        return false;
    }

    return true;
}

UObject* URiveActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
    if (ARiveWidgetActor* RiveActor = Cast<ARiveWidgetActor>(ActorInstance))
    {
        if (URiveWidget* RiveWidget =
                RiveActor->GetWidgetClass()->GetDefaultObject<URiveWidget>())
        {
            return RiveWidget->RiveDescriptor.RiveFile;
        }
    }

    return nullptr;
}
