// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteGlobals.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "ActorFactories/ActorFactory.h"
#include "EnhancedPaletteLibrary.generated.h"

struct FConfigPlaceableItem;

/**
 * Variety of editor functions for collectors
 */
UCLASS()
class ENHANCEDPALETTE_API UEnhancedPaletteLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="EnhancedPalette|Misc")
	static void RequestDiscoverCategories();

	UFUNCTION(BlueprintCallable, Category="EnhancedPalette|Misc")
	static void RequestPopulateCategories();

	UFUNCTION(BlueprintCallable, DisplayName="Register Placement Category", Category="EnhancedPalette|Externals")
	static bool RegisterExternalCategory(FName UniqueId,
		UPARAM(DisplayName=Tooltip) FText DisplayName,
		UPARAM(DisplayName=Title) FText ShortDisplayName,
		UPARAM(meta=(GetOptions="EnhancedPalette.EnhancedPaletteLibrary.K2_IconSelectorHelper")) FString DisplayIconCode,
		int32 SortOrder = 0,
		bool bSortable = true);

	UFUNCTION()
	static TArray<FString> K2_IconSelectorHelper();

	UFUNCTION(BlueprintCallable, DisplayName="Unregister Placement Category", Category="EnhancedPalette|Externals")
	static bool UnregisterExternalCategory(FName UniqueId);

	UFUNCTION(BlueprintCallable, DisplayName="Register Placeable Item", Category="EnhancedPalette|Externals")
	static bool AddCategoryPlaceableItem(FName UniqueId, TInstancedStruct<FConfigPlaceableItem> Descriptor);

	UFUNCTION(BlueprintCallable, Category="EnhancedPalette|Misc", meta=(DefaultToSelf="Category", AdvancedDisplay=1))
	static void NotifyCategoryChanged(UEnhancedPaletteCategory* Category, bool bContent = true, bool bInfo = false);
	
	UFUNCTION(BlueprintCallable, Category="EnhancedPalette|Misc")
	static UActorFactory* FindActorFactory(TSubclassOf<UActorFactory> Class);
	
	UFUNCTION(BlueprintCallable, Category="EnhancedPalette|Misc")
	static UActorFactory* FindActorFactoryForActor(TSubclassOf<AActor> Class);
	
	UFUNCTION(BlueprintCallable, Category="EnhancedPalette|Misc")
	static UActorFactory* FindActorFactoryByClassForActor(TSubclassOf<UActorFactory> FactoryClass, TSubclassOf<AActor> ActorClass);
};
