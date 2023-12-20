// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Components/Widget.h"
#include "RiveWidget.generated.h"

class SRiveWidget;
class URiveFile;

/**
 *
 */
UCLASS()
class RIVE_API URiveWidget : public UWidget
{
    GENERATED_BODY()

protected:

    //~ BEGIN : UWidget Interface

#if WITH_EDITOR

    virtual const FText GetPaletteCategory() override;

#endif // WITH_EDITOR

    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    virtual TSharedRef<SWidget> RebuildWidget() override;

    //~ END : UWidget Interface

    /**
     * Implementation(s)
     */

public:

    void SetRiveFile(URiveFile* InRiveFile);

    /**
     * Attribute(s)
     */

public:

    /** Reference to Ava Blueprint Asset */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    TObjectPtr<URiveFile> RiveFile;

private:

    /** Rive Widget */
    TSharedPtr<SRiveWidget> RiveWidget;
};
