/**
 * GeoLocation
 *
 * Copyright (c) 2016 Steven Thompson
**/

#pragma once

#include "CoreMinimal.h"
#include "World.h"
#include "UnrealMathUtility.h"
#include "ConfigCacheIni.h"
#include "TimerManager.h"
#include "Resources/Version.h"
#include "Modules/ModuleManager.h"
#include "GeoLocationProvider.h"

class UGeoFenceComponent;

class FGeoLocationModule : public IModuleInterface
{

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get singleton instance
	 */
	static FGeoLocationModule& Get();

	/**
	 * Get the Platform Provider
	 */
	AGeoLocationProvider* GetPlatformProvider(UWorld* world);

	/**
	 * Shutdown the Platform Provider
	 */
	void ShutdownPlatformProvider();

	/**
	 * Returns the array of active GeoFences
	 */
	TArray<UGeoFenceComponent*>& GeoFences();

private:

	/**
	 * Add the iOS location when in use description setting to the Plist. This is a required by iOS to access location services.
	 */
	void AddIOSLocationWhenInUseDescriptionToPlist(FString usageDescription);

	/**
	 * Add the Android Google Play Services version to use to the config. The version to use is based on the current Unreal Engine version.
	 */
	void AddAndroidGooglePlayServicesVersionConfig();

	/**
	 * Get the Project Default Engine Ini path
	 */
	FString GetProjectDefaultEngineIniPath();

	/**
	 * World Tick
	 */
	void WorldTick(UWorld* world, ELevelTick tickType, float deltaTime);

	/* Boolean for if the Geo Location Provider is ready to be used */
	bool GeoLocationProviderReady = false;

	/* Geo Location Provider */
	AGeoLocationProvider* GeoLocationProvider;

	/* Active GeoFences array */
	TArray<UGeoFenceComponent*> _GeoFences;

	/* Boolean for if the GeoFence array is ready to be used */
	bool GeoFencesReady = false;

};
