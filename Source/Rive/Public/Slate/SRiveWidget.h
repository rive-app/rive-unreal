// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"

class URiveArtboard;
class URiveTexture;
class URiveFile;
class SRiveWidgetView;

/**
 *
 */
class RIVE_API SRiveWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SRiveWidget)
#if WITH_EDITOR
		: _bDrawCheckerboardInEditor(false)
#endif
        {
        }
#if WITH_EDITOR
	SLATE_ARGUMENT(bool, bDrawCheckerboardInEditor)
#endif
    SLATE_END_ARGS()

	~SRiveWidget() override;
    /**
     * Implementation(s)
     */

public:

    /** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void SetRiveTexture(URiveTexture* InRiveTexture);
	void RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards);
	void SetRiveFile(URiveFile* InRiveFile);

    /**
     * Attribute(s)
     */

private:

    /** Reference to Avalanche View */
    TSharedPtr<SRiveWidgetView> RiveWidgetView;
	URiveFile* RiveFile = nullptr;
	FDelegateHandle OnArtboardChangedHandle;
};
