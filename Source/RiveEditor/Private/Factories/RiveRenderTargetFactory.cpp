#include "RiveRenderTargetFactory.h"

#include "AssetToolsModule.h"
#include "Logs/RiveEditorLog.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveRenderTarget2D.h"
#include "UObject/SavePackage.h"

FRiveRenderTargetFactory::FRiveRenderTargetFactory(URiveFile* InRiveFile) :
    RiveFile(InRiveFile)
{}

URiveRenderTarget2D* FRiveRenderTargetFactory::CreateRenderTarget()
{
    const UPackage* const RivePackage = RiveFile->GetOutermost();
    const FString RivePackageName = RivePackage->GetName();
    const FString FolderName =
        *FPackageName::GetLongPackagePath(RivePackageName);

    const FString BasePackageName =
        FolderName + TEXT("/") + RiveFile->GetName();

    FString RiveObjectName;
    FString RiveObjectPackageName;
    const FAssetToolsModule& AssetToolsModule =
        FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(
            "AssetTools");
    AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName,
                                                 FString("_RenderTarget"),
                                                 /*out*/ RiveObjectPackageName,
                                                 /*out*/ RiveObjectName);

    UPackage* RiveObjectPackage = CreatePackage(*RiveObjectPackageName);

    URiveRenderTarget2D* RiveRenderTarget2D =
        NewObject<URiveRenderTarget2D>(RiveObjectPackage,
                                       URiveRenderTarget2D::StaticClass(),
                                       FName(RiveObjectName),
                                       RF_Public | RF_Standalone);

    RiveRenderTarget2D->RiveDescriptor.RiveFile = RiveFile;

    RiveRenderTarget2D->bSupportsUAV = true;
    RiveRenderTarget2D->InitCustomFormat(256, 256, PF_R8G8B8A8, false);
    RiveRenderTarget2D->InitRiveRenderTarget2D();

    return RiveRenderTarget2D;
}

bool FRiveRenderTargetFactory::SaveAsset(URiveRenderTarget2D* InRiveObject)
{
    if (!InRiveObject)
    {
        return false;
    }

    // auto-save asset outside of the editor
    UPackage* const Package = InRiveObject->GetOutermost();

    FString const PackageName = Package->GetName();

    FString const PackageFileName = FPackageName::LongPackageNameToFilename(
        PackageName,
        FPackageName::GetAssetPackageExtension());

    if (IFileManager::Get().IsReadOnly(*PackageFileName))
    {
        UE_LOG(LogRiveEditor,
               Error,
               TEXT("Could not save read only file: %s"),
               *PackageFileName);

        return false;
    }

    double StartTime = FPlatformTime::Seconds();

    FSavePackageArgs SaveArgs;

    SaveArgs.TopLevelFlags = RF_Standalone;

    SaveArgs.SaveFlags = SAVE_NoError | SAVE_Async;

    UPackage::SavePackage(Package, NULL, *PackageFileName, SaveArgs);

    const double ElapsedTime = FPlatformTime::Seconds() - StartTime;

    UE_LOG(LogRiveEditor,
           Log,
           TEXT("Saved %s in %0.2f seconds"),
           *PackageName,
           ElapsedTime);

    return Package->MarkPackageDirty();
}

bool FRiveRenderTargetFactory::Create()
{
    URiveRenderTarget2D* NewObject = CreateRenderTarget();

    if (!NewObject)
    {
        return false;
    }

    if (!SaveAsset(NewObject))
    {
        return false;
    }

    return true;
}