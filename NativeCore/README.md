# Wildbound C++ gameplay core

This portable C++17 library owns movement, jump/gravity, attack cooldowns and damage, ability damage, monster chase AI, boss unlocks, health, kills and XP. It does not call raylib or draw graphics. Unity owns rendering, scene objects, cameras, animation playback, audio and the Editor workflow; C# is the native plug-in adapter between Unity and the exported C ABI.

Build/test: run cmake -S NativeCore -B NativeCore/build -DCMAKE_BUILD_TYPE=Release, then cmake --build NativeCore/build --config Release and ctest --test-dir NativeCore/build -C Release --output-on-failure.

Unity expects a Windows DLL or Android ARM64 shared object in Assets/Plugins. scripts/build-native-core.ps1 builds and copies it. The binary plug-in is intentionally not stored in Git; build it for the target host/NDK.
