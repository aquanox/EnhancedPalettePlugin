// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteGlobals.h"

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
#include "Misc/DefinePrivateMemberPtr.h"
#else

#include "HAL/PreprocessorHelpers.h"
#include "Templates/Identity.h"

namespace UE_Core_Private
{
	template <auto Storage, auto PtrToMember>
	struct TPrivateAccess
	{
		TPrivateAccess()
		{
			*Storage = PtrToMember;
		}

		static TPrivateAccess Instance;
	};

	template <auto Storage, auto PtrToMember>
	TPrivateAccess<Storage, PtrToMember> TPrivateAccess<Storage, PtrToMember>::Instance;
}

#define UE_DEFINE_PRIVATE_MEMBER_PTR(TypeNamr, Name, Class, Member) \
	TIdentity<PREPROCESSOR_REMOVE_OPTIONAL_PARENS(TypeNamr)>::Type PREPROCESSOR_REMOVE_OPTIONAL_PARENS(Class)::* Name; \
	template struct UE_Core_Private::TPrivateAccess<&Name, &PREPROCESSOR_REMOVE_OPTIONAL_PARENS(Class)::Member>

#endif


#ifdef LEGACYHELPER


template<typename Accessor, typename Accessor::Member Member>
struct AccessPrivate
{
	friend typename Accessor::Member GetPrivate(Accessor InAccessor) { return Member; }
};

#define BM_IMPLEMENT_GET_PRIVATE_VAR(VarType, GVar, InClass, VarName) \
struct InClass##VarName##Accessor { using Member = VarType InClass::*; friend Member GetPrivate(InClass##VarName##Accessor); }; \
template struct AccessPrivate<InClass##VarName##Accessor, &InClass::VarName>; \
using GVar = InClass##VarName##Accessor; \

using PlacementCategoryMap = TMap<FName, FPlacementCategory>;
// UE_DEFINE_PRIVATE_MEMBER_PTR(PlacementCategoryMap, GCategories, FPlacementModeModule, Categories);

BM_IMPLEMENT_GET_PRIVATE_VAR(PlacementCategoryMap, GCategories, FPlacementModeModule, Categories);


#endif 