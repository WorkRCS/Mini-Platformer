# Android build

This is the only supported runtime target. The project uses C++17, raylib 6, Android NDK, and NativeActivity.

## Prerequisites
- JDK 17 or 25
- Android SDK Platform 35
- Android Build Tools 36.0.0
- Android NDK 27.0.12077973
- Android CMake 4.1.2

## Build
Windows PowerShell:
\`\`\`powershell
.\gradlew.bat assembleDebug
\`\`\`

Linux/macOS:
\`\`\`bash
chmod +x ./gradlew
./gradlew assembleDebug
\`\`\`

The APK is at \`app/build/outputs/apk/debug/app-debug.apk\`. The first build downloads Gradle dependencies and raylib. Runtime assets are packaged from the repository's lowercase \`assets/\` directory.
