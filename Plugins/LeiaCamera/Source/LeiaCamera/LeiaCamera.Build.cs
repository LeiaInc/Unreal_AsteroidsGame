// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LeiaCamera : ModuleRules
{
	public LeiaCamera(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "RHI",
                "RenderCore",
				"HeadMountedDisplay",
				"Projects",
				"UMG"
				// ... add private dependencies that you statically link with here ...	
			}
            );
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);


		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Add UPL to add configrules.txt to our APK
			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "LeiaCamera_UPL.xml"));
		}
		//this actually never worked for x64, but just to be safe it shouldnt get triggered. Deal with everything in the x64 handler
#if false
		else if (false && Target.Platform == UnrealTargetPlatform.Win32)
        {
            RuntimeDependencies.Add("$(TargetOutputDir)/LeiaDisplayService.Wcf.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaDisplayService.Wcf.dll"));
            RuntimeDependencies.Add("$(TargetOutputDir)/LeiaSharedHandshake.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaSharedHandshake.dll"));
            RuntimeDependencies.Add("$(TargetOutputDir)/LeiaSharedInterlaceCalculations.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaSharedInterlaceCalculations.dll"));
            RuntimeDependencies.Add("$(TargetOutputDir)/jsoncpp.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/jsoncpp.dll"));
		}
#endif
		else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			RuntimeDependencies.Add("$(BinaryOutputDir)/LeiaDisplayService.Wcf.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaDisplayService.Wcf.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/LeiaDisplaySdkCpp.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaDisplaySdkCpp.dll"));

#if true
			RuntimeDependencies.Add("$(BinaryOutputDir)/blinkTrackingWrapper.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/blink/bin/blinkTrackingWrapper.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/blink.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/blink/bin/blink.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/opencv_world3413.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/opencv/bin/opencv_world3413.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/realsense2.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/realsense/bin/realsense2.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/realsense2-gl.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/realsense/bin/realsense2-gl.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/blink_landmark_detector.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/blink/bin/blink_landmark_detector.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/cryptolens_c.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/blink/bin/cryptolens_c.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/tensorflowlite_c.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/blink/bin/tensorflowlite_c.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/gpu_delegate.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Blink/third_party/blink/bin/gpu_delegate.dll"));

            PublicSystemLibraryPaths.AddRange(new string[] {
            System.IO.Path.Combine(PluginDirectory,"Source/LeiaCamera/Libraries/Blink/third_party/opencv/lib"),
            System.IO.Path.Combine(PluginDirectory,"Source/LeiaCamera/Libraries/Blink/third_party/blink/lib/"),
            System.IO.Path.Combine(PluginDirectory,"Source/LeiaCamera/Libraries/Blink/third_party/realsense/lib/")
        });
            PublicPreBuildLibraries.AddRange(new string[] { "blinkTrackingWrapper.lib", "blink.lib", "fw.lib", "glfw3.lib", "realsense2.lib", "realsense2-gl.lib", "realsense-file.lib", "opencv_world3413.lib" });
#endif
        }
    }
}
 