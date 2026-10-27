# Android build

The game source uses raylib's cross-platform input API, including on-screen touch controls and gamepad input. The Android project files are in this folder; the same `src/main.cpp` is compiled into the Android NativeActivity.

## Requirements

- Android Studio
- Android SDK Platform 35
- Android NDK (side-by-side)
- CMake 3.22.1 installed from Android Studio's SDK Tools

## Build an APK

1. Open Android Studio.
2. Select **Open** and choose this repository's `android` folder.
3. Allow Gradle sync to finish. The first native build downloads raylib 6.0 from its official GitHub repository, so an internet connection is needed.
4. Choose the `app` run configuration and run it on a connected Android phone, or choose **Build > Build APK(s)**.
5. The debug APK is normally written to `app/build/outputs/apk/debug/app-debug.apk`.

The app is locked to landscape orientation for the first prototype. It includes on-screen left/right/jump controls; Bluetooth or USB controllers can use the gamepad mapping supported by raylib.

If Android Studio reports that the Android SDK or NDK is missing, install it from **Tools > SDK Manager** before building.
