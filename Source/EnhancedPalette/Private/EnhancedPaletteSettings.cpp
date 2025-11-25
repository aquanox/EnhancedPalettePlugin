// Copyright 2025, Aquanox.

#include "EnhancedPaletteSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedPaletteSettings)

// ======================================================================
// ======================================================================

UEnhancedPaletteSettings::UEnhancedPaletteSettings()
{
}

void UEnhancedPaletteSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

TArray<TSoftObjectPtr<UScriptStruct>> UEnhancedPaletteSettings::GetUsableStaticItems()
{
	TArray<TSoftObjectPtr<UScriptStruct>> Result;
	Result.Add(FConfigPlaceableItem_FactoryClass::StaticStruct());
	Result.Add(FConfigPlaceableItem_FactoryObject::StaticStruct());
	Result.Add(FConfigPlaceableItem_ActorClass::StaticStruct());
	Result.Add(FConfigPlaceableItem_AssetObject::StaticStruct());
	return Result;
}

TArray<TSoftObjectPtr<UScriptStruct>> UEnhancedPaletteSettings::GetUnUsableStaticItems()
{
	TArray<TSoftObjectPtr<UScriptStruct>> Result;
	Result.Add(FConfigPlaceableItem_Native::StaticStruct());
	Result.Add(FConfigPlaceableItem_FactoryAssetData::StaticStruct());
	Result.Add(FConfigPlaceableItem_AssetData::StaticStruct());
	return Result;
}
