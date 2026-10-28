# Native C++ plug-ins

The portable gameplay rules are in NativeCore/. Build the Windows x64 DLL with Wildbound > Build Native C++ Core (Windows x64) or run scripts/build-native-core.ps1. For Android ARM64, use the matching Wildbound menu entry or pass the Unity Hub NDK folder to the same script. The result is copied under Assets/Plugins for the platform.

The Editor can open the scene without a native DLL (managed preview mode), but the C++ gameplay core is used when a platform-specific library is present.
