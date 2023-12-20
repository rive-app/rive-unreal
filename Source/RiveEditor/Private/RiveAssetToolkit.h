// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Tools/BaseAssetToolkit.h"

class IDetailsView;
class SRiveWidget;

class RIVEEDITOR_API FRiveAssetToolkit : public FBaseAssetToolkit, public FTickableEditorObject
{
    /**
     * Structor(s)
     */

public:

    explicit FRiveAssetToolkit(UAssetEditor* InOwningAssetEditor);

    virtual ~FRiveAssetToolkit();

    //~ BEGIN : FBaseAssetToolkit Interface

public:

    virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

    //~ END : FBaseAssetToolkit Interface

    //~ BEGIN FTickableEditorObject Interface

public:

    virtual void Tick(float DeltaTime) override;

    virtual ETickableTickType GetTickableTickType() const override
    {
        return ETickableTickType::Always;
    }

    virtual TStatId GetStatId() const override;

    //~ END FTickableEditorObject Interface

    /**
     * Implementation(s)
     */

public:

    TSharedRef<SDockTab> SpawnTab_RiveViewportTab(const FSpawnTabArgs& Args);

    TSharedRef<SDockTab> SpawnTab_DetailsTabID(const FSpawnTabArgs& Args);

    /**
     * Attribute(s)
     */

private:

    /** Additional Tab to select mesh/actor to add a 3D preview in the scene. */
    static const FName RiveViewportTabID;

    static const FName DetailsTabID;

    TSharedPtr<SDockTab> DetailsTab;

    TSharedPtr<IDetailsView> DetailsAssetView;

    TSharedPtr<SRiveWidget> RiveWidget;
};
