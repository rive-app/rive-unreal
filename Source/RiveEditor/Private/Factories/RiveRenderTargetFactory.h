#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

class URiveFile;
class URiveRenderTarget2D;
class RIVEEDITOR_API FRiveRenderTargetFactory
{
public:
    FRiveRenderTargetFactory(URiveFile* InRiveFile);

    bool Create();
    URiveRenderTarget2D* CreateRenderTarget();

    bool SaveAsset(URiveRenderTarget2D* InRiveObject);

private:
    TObjectPtr<URiveFile> RiveFile;
};
