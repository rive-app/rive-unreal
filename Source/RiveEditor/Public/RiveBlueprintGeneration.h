// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UEnum;
class URiveFile;
struct FEnumDefinition;
struct FViewModelDefinition;

// Generates the UEnums and URiveViewModel Blueprints a .riv's view models
// need. The import factory saves them under the file's generated folder;
// tests generate into a transient folder without saving.
struct RIVEEDITOR_API FRiveBlueprintGeneration
{
    // Generates every enum and view model class for File under FolderPath
    // and registers the classes on the file.
    static void GenerateForFile(URiveFile* File,
                                const FString& FolderPath,
                                bool bSave = true);

    static UEnum* GenerateEnum(const FString& FolderPath,
                               const FEnumDefinition& Definition,
                               bool bSave = true);

    static UBlueprint* GenerateViewModelBlueprint(
        const FString& FolderPath,
        const FViewModelDefinition& Definition,
        const TArray<UEnum*>& Enums,
        bool bSave = true);
};
