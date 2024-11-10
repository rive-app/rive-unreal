// Copyright Rive, Inc. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 *
 */
class RIVEEDITOR_API FRiveFileDetailCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
