#pragma once

#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Textures/SlateIcon.h"

// TBD: if struct magic works fine on other platforms than windows
// then can drop IconCode and use StyleSet/Style name pair like in SlateIconReference
#define WITH_GATHER_ITEMS_MAGIC 0

namespace EnhancedPaletteCustomizations
{
	void Register();
	void Unregister();
}

struct FSimpleIconSelector;
class SSearchableComboBox;
class SWidget;

// Customization for simple slate icon reference stub to make plugin self-contained 
class FSimpleIconSelectorCustomization : public IPropertyTypeCustomization
{
	using ThisClass = FSimpleIconSelectorCustomization;
	friend class UEnhancedPaletteLibrary;
	
#if WITH_GATHER_ITEMS_MAGIC
	using ItemType = FSimpleIconSelector;
#else
	using ItemType = FString;
#endif
public:
	static TSharedRef<IPropertyTypeCustomization> Make();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
protected:

	bool CanEdit() const;
	const FSlateBrush* GetDisplayIcon() const;
	FText GetDisplayText() const;
	TSharedRef<SWidget> OnGenerateComboWidget( TSharedPtr<FString> InElement );
	void OnSelectionChangedInternal(TSharedPtr<FString> Selected, ESelectInfo::Type);

	TSharedPtr<IPropertyHandle> Handle;
	TSharedPtr<SSearchableComboBox> Combobox;

#if WITH_GATHER_ITEMS_MAGIC
	TArray<TSharedPtr<FSimpleIconSelector>> ComboItemList;

	static TArray<TSharedPtr<FSimpleIconSelector>> GatherIcons();
#else
	TArray<TSharedPtr<FString>> ComboItemList;

	static TArray<FString> GatherIcons();
#endif
	
	FSlateFontInfo Font;
};
