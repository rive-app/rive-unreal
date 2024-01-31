// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"

class URiveFile;
class SRiveWidgetView;

/**
 *
 */
class RIVE_API SRiveWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SRiveWidget)
        {
        }

    SLATE_END_ARGS()

    /**
     * Implementation(s)
     */

public:

    /** Constructs this widget with InArgs */
    void Construct(const FArguments& InArgs, URiveFile* InRiveFile);

    void SetRiveFile(URiveFile* InRiveFile);

    /**
     * Attribute(s)
     */

private:

    /** Reference to Avalanche View */
    TSharedPtr<SRiveWidgetView> RiveWidgetView;

    TObjectPtr<URiveFile> RiveFile;
};
