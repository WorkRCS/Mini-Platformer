# Android build

The Android project is configured for a landscape NativeActivity and packages the game's root `assets/` folder into the APK. The debug APK targets `arm64-v8a` only, optimized for testing on a modern ARM64 Android phone. It will not run on 32-bit-only phones or x86_64 emulators.

## Build versions

- Gradle Wrapper 9.6.0 (Java 25 compatible)
- Android Gradle Plugin 9.4.0
- JDK 17 or 25
- Android SDK Platform 35
- Android SDK Build Tools 36.0.0
- Android NDK 27.0.12077973
- Android SDK CMake 4.1.2
- Internet access for the first build (Gradle and raylib sources are downloaded)

The NDK version is explicitly pinned to 27.0.12077973 so the build does not select an incomplete 28.2.13676358 installation. The Android build workflow verifies that the NDK metadata file exists before starting Gradle.

## Gradle JDK settings

This repository sets the Android Studio Gradle JDK option to `JAVA_HOME` in its project settings. If you use the current Android Studio bundled Java 25, Gradle 9.6.0 is compatible with it. The project also builds under JDK 17.

If Android Studio still displays a stale JDK warning after pulling, close Android Studio fully and reopen the repository so it reloads the updated Gradle wrapper. In Settings > Build, Execution, Deployment > Build Tools > Gradle, the Gradle JDK can be set to `JAVA_HOME` or the current Studio JDK; both Java 17 and Java 25 are compatible with this updated Gradle/AGP pair.

## Build a debug APK on Windows

Open PowerShell in the repository folder and run:

```powershell
cd android
.\gradlew.bat assembleDebug
```

On Linux/macOS:

```bash
cd android
chmod +x ./gradlew
./gradlew assembleDebug
```

The output APK is:

`android/app/build/outputs/apk/debug/app-debug.apk`

Install it on a connected Android phone with USB debugging enabled:

```powershell
adb install -r .\app\build\outputs\apk\debug\app-debug.apk
```

Or copy the APK to the phone and open it there. Android may ask you to allow installation from that file manager.

## SDK components

If a native-build component is missing, open Android Studio > Tools > SDK Manager and install:
- Android SDK Platform 35
- Android SDK Build-Tools 36.0.0
- Android SDK Command-line Tools (latest)
- Android NDK 27.0.12077973
- CMake 4.1.2

The game uses bundled assets rather than downloading resources at startup. The five-second splash is an animated progress display while the game initializes; it does not mean game data is being downloaded on the phone.
