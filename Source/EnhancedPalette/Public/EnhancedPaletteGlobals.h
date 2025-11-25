// Copyright 2025, Aquanox.

#pragma once

#include "Logging/LogMacros.h"
#include "Misc/EngineVersionComparison.h"

class UEnhancedPaletteSettings;
class UEnhancedPaletteCategory;
class UEnhancedPaletteSubsystem;

#if UE_VERSION_OLDER_THAN(5, 5, 0)
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif

#define WITH_ARCHETYPE_EXPERIMENT 0

#ifndef UE_VERSION_NEWER_THAN_OR_EQUAL
#define UE_VERSION_NEWER_THAN_OR_EQUAL(MajorVersion, MinorVersion, PatchVersion) \
	UE_GREATER_SORT(ENGINE_MAJOR_VERSION, MajorVersion, UE_GREATER_SORT(ENGINE_MINOR_VERSION, MinorVersion, UE_GREATER_SORT(ENGINE_PATCH_VERSION, PatchVersion, true)))
#endif

#ifndef UE_VERSION_OLDER_THAN_OR_EQUAL
#define UE_VERSION_OLDER_THAN_OR_EQUAL(MajorVersion, MinorVersion, PatchVersion) \
	UE_GREATER_SORT(MajorVersion, ENGINE_MAJOR_VERSION, UE_GREATER_SORT(MinorVersion, ENGINE_MINOR_VERSION, UE_GREATER_SORT(PatchVersion, ENGINE_PATCH_VERSION, true)))
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogEnhancedPalette, Log, All);
