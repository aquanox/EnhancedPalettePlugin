#pragma once

#include "CoreMinimal.h"
#include "PlacementModeModuleAccess.h"
#include "EnhancedPaletteSettings.h"
#include "EnhancedPaletteSubsystem.h"

enum class EManagedCategoryFlags;
enum class EManagedCategoryDirtyFlags;
class UEnhancedPaletteSubsystem;
struct FPlacementModeModuleAccess;

/**
 * 
 */
struct FManagedCategory
{
	// category unique identifier 
	const FName UniqueId;
	// category is dynamic and needs to be dynamically repopulated (always dirty?)
	EManagedCategoryFlags Flags;
	// category was registered successfully
	bool bRegistered = false;
	// category is dirty and needs to be repopulated
	bool bDirtyContent = false;
	// category info is dirty and needs to update info (usually due to blueprint changes)
	bool bDirtyInfo = false;

	// list of registered placement items
	TArray<FPlacementModeID> ManagedIds;

	explicit FManagedCategory(FName InUniqueId, EManagedCategoryFlags InBase);

	virtual ~FManagedCategory() = default;
	virtual EManagedCategoryFlags GetCategoryTypeFlag() const = 0;
	virtual void Register(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) = 0;
	virtual void Unregister(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) = 0;
	virtual bool UpdateRegistration(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) = 0;
	virtual void GatherPlaceableItems(UEnhancedPaletteSubsystem* Owner, TArray<TInstancedStruct<FConfigPlaceableItem>>&) = 0;
	virtual void AddReferencedObjects(FReferenceCollector& Collector, UObject* Owner);
	virtual void Tick(float DeltaTime);

	inline bool HasFlag(EManagedCategoryFlags InFlag) const { return EnumHasAnyFlags(Flags, InFlag); }
	inline void SetFlag(EManagedCategoryFlags InFlag) { EnumAddFlags(Flags, InFlag); }
	inline void UnsetFlag(EManagedCategoryFlags InFlag) { EnumRemoveFlags(Flags, InFlag); }

	friend bool operator==(const TSharedPtr<FManagedCategory>& Item, const FName& Key) { return Item->UniqueId == Key; }
};

//  category configured in config panel
struct FConfigDrivenCategory : FManagedCategory
{
	explicit FConfigDrivenCategory(FName InUniqueId);
	explicit FConfigDrivenCategory(FName InUniqueId, EManagedCategoryFlags InBase);

	virtual EManagedCategoryFlags GetCategoryTypeFlag() const override;
	virtual const FStaticPlacementCategoryInfo* GetConfig() const;
	virtual void Register(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) override;
	virtual void Unregister(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) override;
	virtual bool UpdateRegistration(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) override;
	virtual void GatherPlaceableItems(UEnhancedPaletteSubsystem* Owner, TArray<TInstancedStruct<FConfigPlaceableItem>>&) override;
};

//  category configured via blueprint-collector
struct FAssetDrivenCategory : FManagedCategory
{
	TSoftClassPtr<UEnhancedPaletteCategory> Source;
	TObjectPtr<UEnhancedPaletteCategory> Instance;
	TObjectPtr<UEnhancedPaletteCategory> InstanceDefault;

	FDelegateHandle HandleOnChanged;
	FDelegateHandle HandleOnCompiled;
	FDelegateHandle HandleOnModified;

	explicit FAssetDrivenCategory(FName InUniqueId);

	virtual EManagedCategoryFlags GetCategoryTypeFlag() const override;
	virtual void Register(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) override;
	virtual void Unregister(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) override;
	virtual bool UpdateRegistration(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess&) override;
	void UpdateTraits(UEnhancedPaletteSubsystem* Owner, const UEnhancedPaletteCategory* InCategory);
	virtual void GatherPlaceableItems(UEnhancedPaletteSubsystem* Owner, TArray<TInstancedStruct<FConfigPlaceableItem>>&) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector, UObject* Owner) override;
	virtual void Tick(float DeltaTime) override;
};

// category created externally by interacting with subsystem API
struct FExternalCategory : FConfigDrivenCategory
{
	FStaticPlacementCategoryInfo Data;
	bool bPendingKill = false;

	explicit FExternalCategory(FName InUniqueId);

	virtual EManagedCategoryFlags GetCategoryTypeFlag() const override;
	virtual const FStaticPlacementCategoryInfo* GetConfig() const { return &Data; }
};

/**
 * 
 */
struct FManagedCategoryChangeTracker : public TSharedFromThis<FManagedCategoryChangeTracker>
{
	FManagedCategoryChangeTracker() = default;
	~FManagedCategoryChangeTracker();
	
	static bool IsTrackingEnabled();

	void RegisterTrackers(UEnhancedPaletteSubsystem* Owner);
	void UnregisterTrackers(UEnhancedPaletteSubsystem* Owner);
};
