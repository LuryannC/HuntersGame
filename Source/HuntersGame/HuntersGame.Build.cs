// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class HuntersGame : ModuleRules
{
	public HuntersGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "LocationServicesBPLibrary", "XRBase", "AndroidRuntimeSettings" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "Niagara", "EnhancedInput", "HTTP", "ImageWrapper", "GeoLocation" });

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
	        string PluginPath = ModuleDirectory;
	        AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "HuntersGame_UPL.xml"));
        }
	}
}
