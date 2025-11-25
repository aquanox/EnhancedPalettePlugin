// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteTypes.h"
#include "Engine/DeveloperSettings.h"
#include "EnhancedPaletteSettings.generated.h"

namespace SettingsCommand
{
	inline static const FName DiscoverCategories = "DiscoverCategories";
	inline static const FName PopulateCategories = "PopulateCategories";
	inline static const FName UpdateCategores = "UpdateCategores";
	inline static const FName UpdateToolbar = "UpdateToolbar";
	inline static const FName ClearRecent = "ClearRecent";
}

class UEnhancedPaletteCategory;

/**
 * Settings of Enhanced Palette Plugin
 */
UCLASS(DisplayName="Enhanced Palette", Config=EditorPerProjectUserSettings)
class ENHANCEDPALETTE_API UEnhancedPaletteSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Should search for additional palette category classes on startup?
	UPROPERTY(Config, EditAnywhere, Category="Behavior")
	bool bEnableNativeCategoryDiscovery = false;

	// Should search for additional palette category assets on startup?
	UPROPERTY(Config, EditAnywhere, Category="Behavior")
	bool bEnableAssetCategoryDiscovery = false;

	// Enable set of change tracking feautures that would trigger dynamid category refresh
	// Alternative is to make own subscription hooks, register them during dynamic category initialize event, trigger notify.
	UPROPERTY(Config, EditAnywhere, Category="Behavior")
	bool bEnableChangeTrackingFeatures = false;

	// List of custom categories
	UPROPERTY(Config, EditAnywhere, Category="Categories", meta=(TitleProperty="UniqueId", NoElementDuplicate))
	TArray<FStaticPlacementCategoryInfo> StaticCategories;

	// List of custom categories that represented by assets or native classes
	UPROPERTY(Config, EditAnywhere, Category="Categories", meta=(NoCreateNew, NoElementDuplicate))
	TArray<TSoftClassPtr<UEnhancedPaletteCategory>> DynamicCategories;

	// List of settings for engine categories
	// Populated automatically based on registered categories
	UPROPERTY(Config, EditAnywhere, Category="Categories", meta=(TitleProperty="UniqueId", NoElementDuplicate))
	TArray<FStandardPlacementCategoryInfo> EngineCategories;

	// Recently placed objects list
	// Operations on this list reflected in palette
	UPROPERTY(Config, EditAnywhere, Category="RecentList", meta=(TitleProperty="DisplayLabel", NoElementDuplicate))
	TArray<FConfigActorPlacementInfo> RecentlyPlaced;

	// Internal storage for picker
	UPROPERTY(Transient)
	TArray<FName> CachedKnownCategories;

public:
	UEnhancedPaletteSettings();

	virtual FName GetContainerName() const override { return "Editor"; }
	virtual FName GetCategoryName() const override { return "Plugins"; }
	virtual bool SupportsAutoRegistration() const override { return false; }
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UFUNCTION()
	static TArray<TSoftObjectPtr<UScriptStruct>> GetUsableStaticItems();
	UFUNCTION()
	static TArray<TSoftObjectPtr<UScriptStruct>> GetUnUsableStaticItems();
};
