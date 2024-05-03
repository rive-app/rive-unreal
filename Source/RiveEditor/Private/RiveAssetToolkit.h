// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"

class URiveFile;
class IDetailsView;
class SRiveWidget;

class RIVEEDITOR_API FRiveAssetToolkit : public FAssetEditorToolkit
{
public:
    void Initialize(URiveFile* InRiveFile, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost);
	
    //~ BEGIN IToolkit interface
    virtual FText GetBaseToolkitName() const override;
    virtual FName GetToolkitFName() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
    //~ END : IToolkit Interface

public:

    TSharedRef<SDockTab> SpawnTab_RiveViewportTab(const FSpawnTabArgs& Args);

    TSharedRef<SDockTab> SpawnTab_DetailsTabID(const FSpawnTabArgs& Args);

private:

    /** Additional Tab to select mesh/actor to add a 3D preview in the scene. */
    static const FName RiveViewportTabID;
    static const FName DetailsTabID;
    static const FName AppIdentifier;

    TSharedPtr<SDockTab> DetailsTab;
    TSharedPtr<IDetailsView> DetailsAssetView;
    TSharedPtr<SRiveWidget> RiveWidget;

    /** The rive file asset being edited. */
    URiveFile* RiveFile = nullptr;
};
