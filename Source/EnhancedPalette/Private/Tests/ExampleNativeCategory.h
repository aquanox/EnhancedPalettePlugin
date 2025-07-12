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
    /**
     * Constructor for category filling basic information. 
     */
	UExampleNativeCategory();

    /**
     * Invoked when category is created. 
     */
	virtual void NativeInitialize() override;
	/**
	 * Invoked every UpdateInterval if set to be bTickable
     */
	virtual void NativeTick() override;
    /**
     * Invoked every time when category content refreshes 
     */	
	virtual void NativeGatherItems() override;

private:
	void ExampleGatherExplicit();
	void ExampleGatherBlueprintsOfClass();
	void ExampleGatherAssetsOfClass();
	void ExampleCustomItemType();
};
