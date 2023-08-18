// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSuperManager_init() {}
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_SuperManager;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_SuperManager()
	{
		if (!Z_Registration_Info_UPackage__Script_SuperManager.OuterSingleton)
		{
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/SuperManager",
				nullptr,
				0,
				PKG_CompiledIn | 0x00000040,
				0x9FDEC676,
				0x3DB510FF,
				METADATA_PARAMS(nullptr, 0)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_SuperManager.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_SuperManager.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_SuperManager(Z_Construct_UPackage__Script_SuperManager, TEXT("/Script/SuperManager"), Z_Registration_Info_UPackage__Script_SuperManager, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x9FDEC676, 0x3DB510FF));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
