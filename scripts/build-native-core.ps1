param([switch]$Android,[string]$NdkPath="")
$ErrorActionPreference="Stop"
$root=Split-Path -Parent $PSScriptRoot
$core=Join-Path $root "NativeCore"
$buildRoot=Join-Path $core "build"
if($Android){
 if(-not $NdkPath){$NdkPath=$env:UNITY_ANDROID_NDK}
 if(-not $NdkPath){throw "Pass -NdkPath with Unity Hub Android NDK or set UNITY_ANDROID_NDK."}
 $toolchain=Join-Path $NdkPath "build/cmake/android.toolchain.cmake"
 if(-not(Test-Path $toolchain)){throw "Android NDK toolchain not found: $toolchain"}
 $build=Join-Path $buildRoot "android-arm64"
 cmake -S $core -B $build "-DCMAKE_TOOLCHAIN_FILE=$toolchain" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
 if($LASTEXITCODE -ne 0){throw "Android configure failed."}
 cmake --build $build --config Release
 if($LASTEXITCODE -ne 0){throw "Android native core build failed."}
 $file=Get-ChildItem $build -Recurse -Filter "libWildboundCore.so" | Select-Object -First 1
 if(-not $file){throw "libWildboundCore.so was not produced."}
 $dest=Join-Path $root "Assets/Plugins/Android/arm64-v8a"
}else{
 $build=Join-Path $buildRoot "windows-x64"
 cmake -S $core -B $build -DCMAKE_BUILD_TYPE=Release
 if($LASTEXITCODE -ne 0){throw "CMake configure failed. Install CMake and a C++ compiler (such as Visual Studio Build Tools)."}
 cmake --build $build --config Release
 if($LASTEXITCODE -ne 0){throw "Native core build failed."}
 $file=Get-ChildItem $build -Recurse -Filter "WildboundCore.dll" | Select-Object -First 1
 if(-not $file){throw "WildboundCore.dll was not produced."}
 $dest=Join-Path $root "Assets/Plugins/x86_64"
}
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $dest $file.Name) -Force
Write-Host "Installed native core: $dest"
