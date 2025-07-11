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
 * Standard item definitions:
 * - Actor Factory Class (uses factory defaults)
 * - Actor Factory + Asset Data
 * - Actor Factory + SoftObjectPtr
 *
 * Shortcuts:
 * - Actor Class (searches for Factory)
 * - Object (searches for Factory)
 * - Asset Data (searches for Factory)
 *
 * Code use:
 * - PlaceableItem (already contains FPlaceableItem)
 * 
 * Display Name - optional name to display
 * 
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem
{
	GENERATED_BODY()

	// Optional native name override. Native names must be unique within category. Leave empty for default.
	UPROPERTY(EditAnywhere, DisplayName="Native Name Override", Category=Descriptor, meta=(DisplayPriority=100))
	FName NativeName = NAME_None;
	// Optional display name override. Leave empty for default.
	UPROPERTY(EditAnywhere, DisplayName="Display Name Override", Category=Descriptor, meta=(DisplayPriority=101))
	FText DisplayName = INVTEXT("");
	// Optional display order override. Leave zero for default.
	UPROPERTY(EditAnywhere, DisplayName="Sort Order Override", Category=Descriptor, meta=(DisplayPriority=102))
	int32 SortOrder = 0;

	FConfigPlaceableItem() = default;
	
	virtual ~FConfigPlaceableItem() = default;
	
	/**
	 * Is current item identical to another. 
	 */
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const;

	/**
	 * Is current item contains valid data
	 */
	virtual bool IsValidData() const;
	
	/**
	 * Construct placeable item out of current data. Possible return null in case of error.
	 */
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
	
	/**
	 * Build string representation of current item for debug purposes
	 */
	virtual FString ToString() const;

	bool operator==(const FConfigPlaceableItem& Other) const { return IdenticalTo(Other); }
	bool operator!=(const FConfigPlaceableItem& Other) const { return !IdenticalTo(Other); }
};

using TConfigPlaceableItem = TInstancedStruct<FConfigPlaceableItem>;

template<>
struct TStructOpsTypeTraits<FConfigPlaceableItem>
	: public TStructOpsTypeTraitsBase2<FConfigPlaceableItem>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};

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

	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const;
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
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const;
	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

/**
 * Placeable item specifying Actor Factory Class and Asset to spawn.
 *
 * Not usable in Static config due to Asset Data
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_FactoryAssetData : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10, MustImplement="/Script/EditorFramework.AssetFactoryInterface"))
	TSoftClassPtr<UObject> FactoryClass;
	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=11, DisplayThumbnail=false))
	FAssetData AssetData;

	FConfigPlaceableItem_FactoryAssetData() = default;
	FConfigPlaceableItem_FactoryAssetData(TSoftClassPtr<UObject> InFactoryClass, const FAssetData& InAssetData);
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const override;
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
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const override;
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
	explicit FConfigPlaceableItem_ActorClass(TSoftClassPtr<AActor> InClass);
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const override;
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
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const override;
	virtual bool IsValidData() const override;
	virtual TSharedPtr<FPlaceableItem> MakeItem() const;
};

/**
 * Placeable item specifying asset data to spawn.
 *
 * Factory is automatically detected from actor class.
 *
 * Not usable in Static config due to Asset Data.
 */
USTRUCT(BlueprintType)
struct ENHANCEDPALETTE_API FConfigPlaceableItem_AssetData : public FConfigPlaceableItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Descriptor, meta=(DisplayPriority=10))
	FAssetData AssetData;

	FConfigPlaceableItem_AssetData() = default;
	explicit FConfigPlaceableItem_AssetData(const FAssetData& InAssetData);
	virtual bool IdenticalTo(const FConfigPlaceableItem& Other) const override;
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
	FString Factory;
	
	UPROPERTY(VisibleAnywhere, Category=Descriptor, meta=(NoAssetPicker, DisplayThumbnail=false))
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

	bool operator==(const FName& Key) const { return UniqueId == Key; }
};

/**
 * Static category details
 */
USTRUCT()
struct ENHANCEDPALETTE_API FStaticPlacementCategoryInfo : public FConfigPlacementCategoryInfo
{
	GENERATED_BODY()

	// List of elements to be displayed within category
	UPROPERTY(EditAnywhere, Category=General, NoClear, meta=(DisplayAfter="SortOrder", ExcludeBaseStruct=true, GetDisallowedClasses="GetUnUsableStaticItems"))
	TArray<TInstancedStruct<FConfigPlaceableItem>> Items;
};

/**
 * Engine category details
 */
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
	bool CanEditChange(const FProperty* InProperty) const;
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

	UFUNCTION()
	static TArray<TSoftObjectPtr<UScriptStruct>> GetUsableStaticItems();
	UFUNCTION()
	static TArray<TSoftObjectPtr<UScriptStruct>> GetUnUsableStaticItems();

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
