# Android build

The Android project is configured for a landscape NativeActivity and packages the game's root `assets/` folder into the APK. The debug APK includes `arm64-v8a`, `armeabi-v7a`, and `x86_64` native libraries for broad device and emulator compatibility.

## Requirements

- Android Studio (or Android SDK command-line tools)
- JDK 17
- Android SDK Platform 35 and Build Tools 35.0.0
- Android NDK (side-by-side)
- Android SDK CMake 4.1.2
- Internet access for the first build (Gradle and raylib sources are downloaded)

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
- Android SDK Build-Tools 35.0.0
- Android SDK Command-line Tools (latest)
- NDK (Side by side)
- CMake 4.1.2

The game uses bundled assets rather than downloading resources at startup. The five-second splash is an animated progress display while the game initializes; it does not mean game data is being downloaded on the phone.
