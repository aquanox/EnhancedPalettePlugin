// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteSettings.h"
#include "IPlacementModeModule.h"
#include "EnhancedPaletteCategory.generated.h"

enum class EManagedCategoryDirtyFlags;

/**
 * Base class for palette categories.
 *
 * Category classes must be created in Editor or UncookedOnly modules!
 * Trying to make one in Runtime will result in packaging errors.
 * 
 * Category can be either a native class or a blueprint overriding respective gather function.
 */
UCLASS(Abstract, Blueprintable)
class ENHANCEDPALETTE_API UEnhancedPaletteCategory : public UObject
{
	GENERATED_BODY()

public:
	// Category unique identifier. None = use asset name
	UPROPERTY(Transient, VisibleAnywhere, Category="PaletteCategory")
	FName UniqueId;
	
	// Category full display name (while it is actually text for tooltip) 
	UPROPERTY(EditAnywhere, Category="PaletteCategory")
	FText DisplayName;
	
	// Category short display name (text under icon)
	UPROPERTY(EditAnywhere, Category="PaletteCategory")
	FText ShortDisplayName;
	
//#if WITH_SLATE_ICON_REFERENCE
	//UPROPERTY(VisibleAnywhere, Category="PaletteCategory", meta=(DisplayMode="Compact,NoSmall"))
	//FSlateIconReference DisplayIcon;
//#else
	// Category display icon code (non-SIR alternative)
	UPROPERTY(EditAnywhere, Category="PaletteCategory")
	FSimpleIconSelector DisplayIcon;
//#endif
	// Content is sortable
	UPROPERTY(EditAnywhere, Category="PaletteCategory")
	bool bSortable = true;
	
	// Assets with specified tag will be added to category
	// TBD: Not used anymore in newer engines?
	UPROPERTY(EditAnywhere, Category="PaletteCategory")
	FString TagMetaData;
	
	// Category sorting order within toolbar (Lower - Higher)
	UPROPERTY(EditAnywhere, Category="PaletteCategory")
	int32 SortOrder = 0;

	// Should category request periodic updates
	UPROPERTY(EditAnywhere, Category="PaletteCategory|Tracking")
	bool bTickable = false;
	UPROPERTY(EditAnywhere, Category="PaletteCategory|Tracking", meta=(EditCondition="bTickable"))
	float TickInterval = 10.;
	// Should category listen to blueprint changes
	UPROPERTY(EditAnywhere, Category="PaletteCategory|Tracking")
	bool bTrackingBlueprintChanges = false;
	// Should category listen to asset changes
	UPROPERTY(EditAnywhere, Category="PaletteCategory|Tracking")
	bool bTrackingAssetChanges = false;
	// Should category listen to world changes
	UPROPERTY(EditAnywhere, Category="PaletteCategory|Tracking")
	bool bTrackingWorldChanges = false;

private:
	UPROPERTY(Transient)
	TArray<TInstancedStruct<FConfigPlaceableItem>> Descriptors;
	
	bool bGathering = false;
	TOptional<int32> AutoOrder;
	float Accumulator = 0.f;

public:
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual FName GetCategoryUniqueId() const;
	virtual FText GetDisplayName() const;
	virtual FText GetShortDisplayName() const;
	virtual FSlateIcon GetDisplayIcon() const;
	virtual bool IsSortable() const { return bSortable; }
	virtual FString GetTagMetaData() const;
	virtual int32 GetSortOrder() const { return SortOrder; }

	/**
 	 * 
	 */
	void Initialize(class UEnhancedPaletteSubsystem* InSubystem);

	virtual void NativeInitialize()
	{
	}

	UFUNCTION(BlueprintImplementableEvent, Category=EnhancedPalette, meta=(DisplayName="Initialize"))
	void K2_Initialize();

	/**
	 * 
	 */
	void Tick(float DeltaTime);

	virtual void NativeTick()
	{
	}

	UFUNCTION(BlueprintImplementableEvent, Category=EnhancedPalette, meta=(DisplayName="Update"))
	void K2_Tick();

	/**
	 * The data collection entry pointer 
	 */
	void GatherItems(TArray<TConfigPlaceableItem>& OutResult);

	virtual void NativeGatherItems(TArray<TConfigPlaceableItem>& OutResult)
	{
	}

	UFUNCTION(BlueprintImplementableEvent, Category=EnhancedPalette, meta=(DisplayName="Gather Items"))
	void K2_GatherItems();

	void AddDescriptorInternal(TConfigPlaceableItem&& Desc);

	/**
	 * Templated version of AddDescriptor
	 */
	template <typename T = FConfigPlaceableItem, typename... TArgs>
	void AddDescriptor(TArgs&&... Args)
	{
		TInstancedStruct<FConfigPlaceableItem> Item;
		Item.template InitializeAs<T>(Forward<TArgs>(Args)...);
		AddDescriptorInternal(MoveTemp(Item));
	}

	/**
	 * Templated version of AddDescriptor_Native that wraps FPlaceableItem
	 */
	template <typename T = FPlaceableItem, typename... TArgs>
	void AddDescriptor_Item(TArgs&&... Args)
	{
		FConfigPlaceableItem_Native Cfg;
		Cfg.Item = MakeShared<T>(Forward<TArgs>(Args)...);
		AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_Native>(Cfg));
	}

	/**
	 * Add native type descriptor (directly wraps FPlaceableItem) 
	 * @param InItem 
	 */
	void AddDescriptor_Native(TSharedPtr<FPlaceableItem> InItem)
	{
		FConfigPlaceableItem_Native Cfg;
		Cfg.Item = MoveTemp(InItem);
		AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_Native>(Cfg));
	}

	/**
	 * Add generic descriptor from instanced struct
	 * @param ItemStruct Input descriptor
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=(DisplayName="Add (Generic)", Keywords="adi addi", AdvancedDisplay=1, BlueprintProtected=true))
	void AddDescriptorStruct(TInstancedStruct<FConfigPlaceableItem> ItemStruct);

	/**
	 * Add item descriptor with Factory Class
	 * @param FactoryClass 
	 * @param ItemName 
	 * @param ItemOrder 
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=(DisplayName="Add FactoryClass Class", Keywords="adfc addfc", AdvancedDisplay=1, BlueprintProtected=true))
	void AddDescriptor_FactoryClass(TSoftClassPtr<UActorFactory> FactoryClass, FText ItemName = INVTEXT(""), int32 ItemOrder = 0);

	/**
	 * Add item descriptor with Factory Class and Asset 
	 * @param Factory 
	 * @param AssetData 
	 * @param ItemName 
	 * @param ItemOrder 
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=(DisplayName="Add Factory with Asset", Keywords="adfa addfa", AdvancedDisplay=2, BlueprintProtected=true))
	void AddDescriptor_FactoryAsset(TSoftClassPtr<UActorFactory> Factory, const FAssetData& AssetData, FText ItemName = INVTEXT(""), int32 ItemOrder = 0);

	/**
	 * Add item descriptor with Factory Class and Asset 
	 * @param FactoryClass 
	 * @param AssetObject 
	 * @param ItemName 
	 * @param ItemOrder 
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=(DisplayName="Add Factory with Object", Keywords="adfo addfo", AdvancedDisplay=2, BlueprintProtected=true))
	void AddDescriptor_FactoryObject(TSoftClassPtr<UActorFactory> FactoryClass, TSoftObjectPtr<UObject> AssetObject, FText ItemName = INVTEXT(""), int32 ItemOrder = 0);

	/**
	 * Add item descriptor for Actor Class.
	 * Factory detected automatically.
	 * @param ActorClass 
	 * @param ItemName 
	 * @param ItemSortOrder 
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=(DisplayName="Add Actor Class", Keywords="adc addc", AdvancedDisplay=1, BlueprintProtected=true))
	void AddDescriptor_ActorClass(TSoftClassPtr<AActor> ActorClass, FText ItemName = INVTEXT(""), int32 ItemSortOrder = 0);

	/**
	 * Add item descriptor for generic Object (Mesh, DataAsset).
	 * Factory detected automatically.
	 * @param AssetObject 
	 * @param ItemName 
	 * @param ItemSortOrder 
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=(DisplayName="Add Asset Object", Keywords="ado addo", AdvancedDisplay=1, BlueprintProtected=true))
	void AddDescriptor_AssetObject(TSoftObjectPtr<UObject> AssetObject, FText ItemName = INVTEXT(""), int32 ItemSortOrder = 0);

	/**
	 * Automatically assign order for each newly added element starting from specified value
	 * @param StartOrder 
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=( BlueprintProtected=true))
	void SetAutoOrder(int32 StartOrder = 0);

	/**
	 * Notify subsystem that this category requires content refresh
	 */
	UFUNCTION(BlueprintCallable, Category=EnhancedPalette,
		meta=( BlueprintProtected=true))
	void NotifyContentChanged();

private:
	UFUNCTION(BlueprintCallable)
	void PrintDebugInfo();
};
