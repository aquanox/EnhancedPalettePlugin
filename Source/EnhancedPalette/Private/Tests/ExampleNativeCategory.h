// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteCategory.h"
#include "ExampleNativeCategory.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class UExampleNativeCategory : public UEnhancedPaletteCategory
{
	GENERATED_BODY()
public:
	UExampleNativeCategory();

	virtual void NativeInitialize() override;
	virtual void NativeTick() override;
	virtual void NativeGatherItems(TArray<TConfigPlaceableItem>& OutResult) override;
};
