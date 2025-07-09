// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteGlobals.h"
#include "Engine/DeveloperSettings.h"
#include "Internationalization/Text.h"
#include "Misc/EnumClassFlags.h"
#include "AssetRegistry/AssetData.h"
#include "Textures/SlateIcon.h"
#include "EnhancedPaletteSettings.generated.h"

/**
 * This is crude simple helper to prevent dependency of SlateIconReference plugin
 */
USTRUCT()
struct FSimpleIconSelector
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=Icon)
	FString IconCode;
	
	UPROPERTY(VisibleAnywhere, Category=Icon)
	FName StyleSetName;
	UPROPERTY(VisibleAnywhere, Category=Icon)
	FName StyleName;

	FSimpleIconSelector() = default;
	FSimpleIconSelector(const FString& InCode);
	FSimpleIconSelector(const FName& InStyleSet, const FName& InStyle);
	
	FSlateIcon ToSlateIcon() const;
	FText ToDisplayText() const;
};

/**
 * PH placeable item 
 */
struct FPlaceableItem;

/**
 * Represents a single spawnable item within Placement menu.
 *
 * A valid placeable item defined by one of combinations:
 * - Valid Actor Factory (uses Default Actor)
 * - Valid Factory and Object Name
 * - Valid Actor Class Name (uses Actor Factory)
 * 
 * Display Name - optional name to display
 * 
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem
{
	GENERATED_BODY()

	// Item display name override. Leave empty for default value.
	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=9))
	FText DisplayName = INVTEXT("");
	// Item display order override. Leave empty for default order.
	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=90))
	int32 SortOrder = 0;

	FConfigPlaceableItem() = default;
	virtual ~FConfigPlaceableItem() = default;
	virtual bool Identical(const FConfigPlaceableItem& Other) const { return false; }

	virtual TSharedPtr<FPlaceableItem> MakeItem() const;

	virtual FString ToString() const;
	virtual bool IsValidData() const;
};

using TConfigPlaceableItem = TInstancedStruct<FConfigPlaceableItem>;


/**
 * Placeable item specifying already existing FPlaceableItem.
 *
 * For native use.
 */
USTRUCT()
struct ENHANCEDPALETTE_API FConfigPlaceableItem_Native : public FConfigPlaceableItem
{
	GENERATED_BODY()

	TSharedPtr<FPlaceableItem> Item;

	FConfigPlaceableItem_Native() = default;
	explicit FConfigPlaceableItem_Native(TSharedPtr<FPlaceableItem> InItem);

	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const override;
};

/**
 * Placeable item specifying Actor Factory Class.
 *
 * Uses factory defaults.
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_FactoryClass : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10, MustImplement="/Script/EditorFramework.AssetFactoryInterface"))
	TSoftClassPtr<UObject> FactoryClass;

	FConfigPlaceableItem_FactoryClass() = default;
	explicit FConfigPlaceableItem_FactoryClass(TSoftClassPtr<UObject> InClass);
	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

/**
 * Placeable item specifying Actor Factory Class and Asset to spawn.
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_FactoryAsset : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10, MustImplement="/Script/EditorFramework.AssetFactoryInterface"))
	TSoftClassPtr<UObject> FactoryClass;
	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=11, DisplayThumbnail=false))
	FAssetData ObjectData;

	FConfigPlaceableItem_FactoryAsset() = default;
	FConfigPlaceableItem_FactoryAsset(TSoftClassPtr<UObject> InFactoryClass, const FAssetData& InAssetData);

	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

/**
 * Placeable item specifying Actor Factory Class and Object to spawn.
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_FactoryObject : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10, MustImplement="/Script/EditorFramework.AssetFactoryInterface"))
	TSoftClassPtr<UObject> FactoryClass;
	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=11, DisplayThumbnail=false))
	TSoftObjectPtr<UObject> Object;

	FConfigPlaceableItem_FactoryObject() = default;
	FConfigPlaceableItem_FactoryObject(TSoftClassPtr<UObject> InFactoryClass, TSoftObjectPtr<UObject> InObject);

	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

/**
 * Placeable item specifying Actor Class to spawn.
 *
 * Factory is automatically detected from actor class.
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_ActorClass : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10))
	TSoftClassPtr<AActor> ActorClass;

	FConfigPlaceableItem_ActorClass() = default;
	explicit FConfigPlaceableItem_ActorClass( TSoftClassPtr<AActor> InClass);
	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

/**
 * Placeable item specifying Object to spawn.
 *
 * Factory is automatically detected from object.
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_AssetObject : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10))
	TSoftObjectPtr<UObject> Object;

	FConfigPlaceableItem_AssetObject() = default;
	explicit FConfigPlaceableItem_AssetObject(TSoftObjectPtr<UObject> InObject);
	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

struct FConfigPlaceableItemSorter
{
	bool operator()(const TConfigPlaceableItem& Left, const TConfigPlaceableItem& Right) const
	{
		if (Left.IsValid() && Right.IsValid())
		{
			return operator()(Left.Get<FConfigPlaceableItem>(), Right.Get<FConfigPlaceableItem>());
		}
		return Left.IsValid();
	}
	bool operator()(const FConfigPlaceableItem& Left, const FConfigPlaceableItem& Right) const
	{
		return Left.SortOrder < Right.SortOrder;
	}
};

/**
 * Recent list PH type
 */
class FActorPlacementInfo;

/**
 * Represents an item within Recent Actors list
 */
USTRUCT()
struct ENHANCEDPALETTE_API FConfigActorPlacementInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=Descriptor, meta=(EditCondition="false", EditConditionHides))
	FString DisplayLabel;
	UPROPERTY(VisibleAnywhere, Category=Descriptor, meta=(MustImplement="/Script/EditorFramework.AssetFactoryInterface"))
	// TSoftClassPtr<UObject> Factory;
	FString Factory;
	UPROPERTY(VisibleAnywhere, Category=Descriptor, meta=(NoAssetPicker, DisplayThumbnail=false))
	// TSoftObjectPtr<UObject> ObjectPath;
	FString ObjectPath;
};

/**
 * Placement module category
 */
struct FPlacementCategoryInfo;

/**
 * Base type for category descriptors
 */
USTRUCT()
struct ENHANCEDPALETTE_API FConfigPlacementCategoryInfo
{
	GENERATED_BODY()

public:
	// Category unique identifier. Must be globally unique.
	UPROPERTY(EditAnywhere, NoClear, Category=General)
	FName UniqueId;
	
	// Category button tooltip text
	UPROPERTY(EditAnywhere, NoClear, DisplayName="Tooltip Text", Category=General)
	FText DisplayName;
	
	// Category button text (UE 5.5+ New UI Only)
	UPROPERTY(EditAnywhere, NoClear, DisplayName="Icon Text", Category=General)
	FText ShortDisplayName;
	
//#if WITH_SLATE_ICON_REFERENCE
	//UPROPERTY(VisibleAnywhere, Category="General", meta=(DisplayMode="Compact,NoSmall"))
	//FSlateIconReference DisplayIcon;
//#else
	UPROPERTY(EditAnywhere, Category=General)
	FSimpleIconSelector DisplayIcon;
//#endif
	
	UPROPERTY(EditAnywhere, Category=General)
	// Category content would be sortable
	bool bSortable = true;
	
	// Tag for objects that would be added to this category content
	// TBD: Not used anymore in newer engines?
	UPROPERTY(EditAnywhere, Category=General, meta=(EditCondition="false", EditConditionHides))
	FString TagMetaData;
	
	// Category sort order within toolbar (Old UI: Less - Left, New UI: Less - Higher)
	UPROPERTY(EditAnywhere, Category=General)
	int32 SortOrder = 0;
};

USTRUCT()
struct ENHANCEDPALETTE_API FStaticPlacementCategoryInfo : public FConfigPlacementCategoryInfo
{
	GENERATED_BODY()

	// List of elements to be displayed within category
	UPROPERTY(EditAnywhere, Category=General, meta=(DisplayAfter="SortOrder", ExcludeBaseStruct=true))
	TArray<TInstancedStruct<FConfigPlaceableItem>> Items;

	bool operator==(const FName& Key) const { return UniqueId == Key; }
};

USTRUCT()
struct ENHANCEDPALETTE_API FStandardPlacementCategoryInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, NoClear, Category=General)
	FName UniqueId;
	UPROPERTY(EditAnywhere, Category=General)
	bool bVisible = true;
	UPROPERTY(EditAnywhere, Category=General)
	int32 Order = 0;

	bool operator==(const FName& Key) const { return UniqueId == Key; }
	bool CanEditChange(const FEditPropertyChain& PropertyChain) const;
};

template<>
struct TStructOpsTypeTraits<FStandardPlacementCategoryInfo>
	: public TStructOpsTypeTraitsBase2<FStandardPlacementCategoryInfo>
{
	enum
	{
		WithCanEditChange = true
	};
};

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
	
	// Hook for user actions
	TDelegate<void(FName)> OnUserAction;
public:
	UEnhancedPaletteSettings();

	virtual FName GetContainerName() const override { return "Editor"; }
	virtual FName GetCategoryName() const override { return "Plugins"; }
	virtual bool SupportsAutoRegistration() const override { return false; }
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	// Action to force category rediscover
	UFUNCTION(BlueprintCallable, Category=Misc, meta=(FixedCallInEditor, DisplayName="Trigger Discover"))
	void TriggerDiscover();
	// Action to force category content repopulate
	UFUNCTION(BlueprintCallable, Category=Misc, meta=(FixedCallInEditor, DisplayName="Trigger Populate Items"))
	void TriggerPopulateItems();
	// Action to force category registration update
	UFUNCTION(BlueprintCallable, Category=Misc, meta=(FixedCallInEditor, DisplayName="Trigger Update Categories"))
	void TriggerUpdateData();
	// Action to force PlaceActors widget update for UE5.5+
	UFUNCTION(BlueprintCallable, Category=Misc, meta=(FixedCallInEditor, DisplayName="Trigger Refresh Widget"))
	void RefreshToolbar();
	// Action to quickly reset RecentlyPlacedActors category and refresh panel
	UFUNCTION(BlueprintCallable, Category=Misc, meta=(FixedCallInEditor, DisplayName="Clear Recently Placed"))
	void ClearRecentlyPlaced();
};
