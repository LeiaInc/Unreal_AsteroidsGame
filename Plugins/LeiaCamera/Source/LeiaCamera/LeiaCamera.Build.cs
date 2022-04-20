/****************************************************************
*
* Copyright 2022 � Leia Inc.
*
****************************************************************
*/

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
				"Projects"
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
#if false
		else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            //D:\Unreal Projects\LeiaUnrealSDK\Plugins\LeiaCamera\Source\LeiaCamera\Libraries\Windows
            RuntimeDependencies.Add("$(TargetOutputDir)/LeiaDisplayService.Wcf.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaDisplayService.Wcf.dll"));
            RuntimeDependencies.Add("$(TargetOutputDir)/LeiaSharedHandshake.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaSharedHandshake.dll"));
            RuntimeDependencies.Add("$(TargetOutputDir)/LeiaSharedInterlaceCalculations.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/LeiaSharedInterlaceCalculations.dll"));
            RuntimeDependencies.Add("$(TargetOutputDir)/jsoncpp.dll", System.IO.Path.Combine(PluginDirectory, "Source/LeiaCamera/Libraries/Windows/jsoncpp.dll"));
        }


//disabled for now

		PublicSystemLibraryPaths.AddRange(new string[] {
            "C:/Opencv3.4.13/opencv/build/x64/vc15/lib",
            "C:/nzNew/MetaNew/blink/third_party/blink/lib",
            "C:/nzNew/MetaNew/blink/third_party/realsense/lib",
            "C:/Users/lee.nagar/Documents/blink/headtracking-blink-windows/blinkTrackingWrapper/x64/Release"
        });
        PublicPreBuildLibraries.AddRange(new string[] { "blinkTrackingWrapper.lib", "blink.lib", "fw.lib", "glfw3.lib", "realsense2.lib", "realsense2-gl.lib", "realsense-file.lib", "opencv_world3413.lib" });

        PrivateIncludePaths.Add("C:/opencv3.4.13/opencv/build/include");
#endif
	}
}
