// Copyright 2025, Aquanox.

#pragma once

#include "IconReference.h"
#include "IPropertyTypeCustomization.h"

namespace EnhancedPaletteCustomizations
{
	void Register();
	void Unregister();
}

class SSearchableComboBox;
class SWidget;

// Customization for simple slate icon reference stub to make plugin self-contained
class FSimpleIconReferenceCustomization : public IPropertyTypeCustomization
{
	using ThisClass = FSimpleIconReferenceCustomization;

public:
	static TSharedRef<IPropertyTypeCustomization> Make();

	static void GatherIcons(TFunction<void(FName, FName)> Func);

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

protected:
	bool CanEdit() const;
	const FSlateBrush* GetDisplayIcon() const;
	FText GetDisplayText() const;
	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<FString> InElement);
	void OnSelectionChangedInternal(TSharedPtr<FString> Selected, ESelectInfo::Type);

	void BuildComboItemList(TArray<TSharedPtr<FString>>& Output) const;
	void ExtractIcon(const TSharedPtr<FString>& Element, FSimpleIconReference& Output) const;

	TSharedPtr<IPropertyHandle> Handle;
	TSharedPtr<SSearchableComboBox> Combobox;

	TArray<TSharedPtr<FString>> ComboItemList;

	FSlateFontInfo Font;
};
