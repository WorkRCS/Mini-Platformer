param(
    [string]$HeroPath,
    [string]$MonsterPath,
    [string]$BossPath,
    [string]$TreePath,
    [string]$RockPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$threeD = Join-Path $repoRoot 'assets/3d'
New-Item -ItemType Directory -Force -Path $threeD | Out-Null

function Install-SketchfabModel {
    param(
        [string]$Slot,
        [string]$SourcePath
    )

    if ([string]::IsNullOrWhiteSpace($SourcePath)) { return }
    if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf)) {
        throw "Model file not found for '$Slot': $SourcePath"
    }

    $source = Get-Item -LiteralPath $SourcePath
    $stem = "sketchfab_$Slot"
    $glbTarget = Join-Path $threeD "$stem.glb"
    $gltfManifest = Join-Path $threeD ('{0}_path.txt' -f $stem)
    $gltfFolder = Join-Path $threeD ('{0}_files' -f $stem)

    if ($source.Extension -ieq '.glb') {
        Copy-Item -LiteralPath $source.FullName -Destination $glbTarget -Force
        if (Test-Path -LiteralPath $gltfManifest) { Remove-Item -LiteralPath $gltfManifest -Force }
        if (Test-Path -LiteralPath $gltfFolder) { Remove-Item -LiteralPath $gltfFolder -Recurse -Force }
        Write-Host "Installed $stem.glb"
        return
    }

    if ($source.Extension -ieq '.gltf') {
        if (Test-Path -LiteralPath $gltfFolder) { Remove-Item -LiteralPath $gltfFolder -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $gltfFolder | Out-Null

        # Keep .bin files and textures beside the glTF and preserve their relative paths.
        Copy-Item -Path (Join-Path $source.DirectoryName '*') -Destination $gltfFolder -Recurse -Force
        $relativeModelPath = ("assets/3d/{0}_files/{1}" -f $stem, $source.Name) -replace '\\', '/'
        Set-Content -LiteralPath $gltfManifest -Value $relativeModelPath -Encoding ascii
        if (Test-Path -LiteralPath $glbTarget) { Remove-Item -LiteralPath $glbTarget -Force }
        Write-Host "Installed $relativeModelPath with sibling files"
        return
    }

    throw "Use a .glb or .gltf file for '$Slot'. If Sketchfab gave you a ZIP, extract it first and pass its .glb/.gltf file."
}

Install-SketchfabModel -Slot 'hero' -SourcePath $HeroPath
Install-SketchfabModel -Slot 'monster' -SourcePath $MonsterPath
Install-SketchfabModel -Slot 'boss' -SourcePath $BossPath
Install-SketchfabModel -Slot 'tree' -SourcePath $TreePath
Install-SketchfabModel -Slot 'rock' -SourcePath $RockPath

$suppliedCount = @(@($HeroPath, $MonsterPath, $BossPath, $TreePath, $RockPath) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
if ($suppliedCount -eq 0) {
    Write-Host 'No models were supplied. Download models from their Sketchfab pages, extract the archive, then run for example:'
    Write-Host '.\scripts\install-sketchfab-models.ps1 -HeroPath "C:\Downloads\hero.glb" -MonsterPath "C:\Downloads\monster.glb"'
    Write-Host 'See assets/3d/SKETCHFAB_CREDITS.md for suggested model pages and attribution.'
}

Write-Host ''
Write-Host 'Finished. Re-run CMake configure and build so assets are copied beside the executable.'
