// Copyright 2025, Aquanox.

#pragma once

#include "Internationalization/Text.h"
#include "Textures/SlateIcon.h"

#include "IconReference.generated.h"

// Showcase on how to make SSearchableComboBox that only accepts strings with custom data structure
#define WITH_MAGIC_COMBO_CAST 0

/**
 * This is simple helper to prevent dependency of SlateIconReference plugin
 */
USTRUCT()
struct ENHANCEDPALETTE_API FSimpleIconReference
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, Category=Icon)
	FString IconCode;

	UPROPERTY(VisibleAnywhere, Category=Icon)
	FName StyleSetName;

	UPROPERTY(VisibleAnywhere, Category=Icon)
	FName StyleName;

public:
	FSimpleIconReference() = default;
	FSimpleIconReference(const FString& InCode);
	FSimpleIconReference(const FName& InStyleSet, const FName& InStyle);

	const FString& GetIconCode() const;

	FText GetDisplayText() const;

	FSlateIcon GetSlateIcon() const;

	const FSlateBrush* GetBrush() const;
};
