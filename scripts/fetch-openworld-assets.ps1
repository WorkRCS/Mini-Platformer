param(
    [switch]$SkipAudio
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$assetsRoot = Join-Path $repoRoot 'assets'
$threeD = Join-Path $assetsRoot '3d'
$sfxDir = Join-Path $assetsRoot 'sfx'
$tempRoot = Join-Path $env:TEMP 'wildbound-asset-setup'

New-Item -ItemType Directory -Force -Path $threeD, $sfxDir, $tempRoot | Out-Null

function Get-KenneyZipUrl([string]$slug) {
    $page = "https://kenney.nl/assets/$slug"
    Write-Host "Checking official Kenney asset page: $page"
    $response = Invoke-WebRequest -Uri $page -UseBasicParsing -Headers @{ 'User-Agent' = 'WildboundAssetSetup/1.0' }
    $html = [string]$response.Content

    $absolutePattern = 'https?://kenney\.nl/media/pages/assets/[^"''\s<>]+\.zip'
    $urls = [regex]::Matches($html, $absolutePattern) | ForEach-Object { $_.Value }
    $url = $urls | Where-Object { $_ -match "/assets/$([regex]::Escape($slug))/" } | Select-Object -First 1
    if ($url) { return $url }

    $relativePattern = '/media/pages/assets/' + [regex]::Escape($slug) + '/[^"''\s<>]+\.zip'
    $relative = [regex]::Match($html, $relativePattern)
    if ($relative.Success) { return "https://kenney.nl$($relative.Value)" }

    throw "Could not find an official ZIP link on $page. Open the page once in a browser and retry later."
}

function Download-KenneyPack([string]$slug) {
    $dest = Join-Path $tempRoot $slug
    $zip = Join-Path $tempRoot "$slug.zip"
    if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $dest | Out-Null

    $uri = Get-KenneyZipUrl $slug
    Write-Host "Downloading $slug from the official publisher..."
    Invoke-WebRequest -Uri $uri -OutFile $zip -UseBasicParsing -Headers @{ 'User-Agent' = 'WildboundAssetSetup/1.0' }
    Expand-Archive -LiteralPath $zip -DestinationPath $dest -Force
    return $dest
}

function Install-Model([string]$packRoot, [string]$namePattern, [string]$targetName) {
    $models = Get-ChildItem -Path $packRoot -Recurse -File | Where-Object {
        $_.Extension -in @('.glb', '.gltf') -and $_.BaseName -match $namePattern
    }
    $model = $models | Sort-Object @{ Expression = { if ($_.Extension -eq '.glb') { 0 } else { 1 } } }, FullName | Select-Object -First 1
    if (-not $model) {
        Write-Warning "No glTF/GLB matching '$namePattern' was found in this pack; procedural scenery remains available."
        return
    }

    if ($model.Extension -eq '.glb') {
        Copy-Item -LiteralPath $model.FullName -Destination (Join-Path $threeD "$targetName.glb") -Force
        Write-Host "Installed $targetName.glb"
        return
    }

    # glTF .gltf files can reference sibling .bin/texture files. Copy the model's
    # complete source directory so its relative references remain valid.
    $modelFolder = Join-Path $threeD "kenney-$targetName"
    if (Test-Path $modelFolder) { Remove-Item $modelFolder -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $modelFolder | Out-Null
    Copy-Item -Path (Join-Path $model.Directory.FullName '*') -Destination $modelFolder -Recurse -Force
    $manifestPath = Join-Path $threeD ($targetName + '_path.txt')
    $relativeModelPath = "assets/3d/kenney-$targetName/$($model.Name)"
    Set-Content -LiteralPath $manifestPath -Value $relativeModelPath -Encoding ascii
    Write-Host "Installed $relativeModelPath"
}

function Install-Sound([string]$packRoot, [string]$pattern, [string]$targetName) {
    $candidate = Get-ChildItem -Path $packRoot -Recurse -File -Filter '*.wav' |
        Where-Object { $_.BaseName -match $pattern } |
        Sort-Object FullName | Select-Object -First 1
    if (-not $candidate) {
        Write-Warning "No WAV matching '$pattern' found for $targetName. The game will use its synthesized fallback sound."
        return
    }
    Copy-Item -LiteralPath $candidate.FullName -Destination (Join-Path $sfxDir $targetName) -Force
    Write-Host "Installed sound: $targetName"
}

$natureRoot = Download-KenneyPack 'nature-kit'
Install-Model $natureRoot '(?i)(tree|pine|oak|birch)' 'world_tree'
Install-Model $natureRoot '(?i)(rock|boulder|stone)' 'world_rock'

if (-not $SkipAudio) {
    $rpgAudioRoot = Download-KenneyPack 'rpg-audio'
    $impactRoot = Download-KenneyPack 'impact-sounds'
    $uiRoot = Download-KenneyPack 'interface-sounds'
    Install-Sound $rpgAudioRoot '(?i)(sword|weapon|impact|hit)' 'attack.wav'
    Install-Sound $impactRoot '(?i)(monster|flesh|hurt|damage|impact)' 'enemy_down.wav'
    Install-Sound $impactRoot '(?i)(flesh|hurt|damage|body)' 'player_hurt.wav'
    Install-Sound $rpgAudioRoot '(?i)(magic|spell|power|whoosh|swoosh)' 'ability.wav'
    Install-Sound $uiRoot '(?i)(click|confirm|select|button)' 'ui_click.wav'
}

@"
Assets used by Wildbound are fetched from official Kenney pages:
- Nature Kit: https://kenney.nl/assets/nature-kit
- RPG Audio: https://kenney.nl/assets/rpg-audio
- Impact Sounds: https://kenney.nl/assets/impact-sounds
- Interface Sounds: https://kenney.nl/assets/interface-sounds

These packs are marked CC0 on their publisher pages. The game keeps procedural
fallbacks if a model or sound is unavailable. Review each source page before
redistributing a release, and do not bundle the full extracted source packs.
"@ | Set-Content -LiteralPath (Join-Path $assetsRoot '3D_ASSET_SOURCES.txt') -Encoding utf8

Write-Host ''
Write-Host 'Wildbound asset setup finished. Rebuild the Android APK to bundle assets.'
Write-Host "Downloaded archives are cached under: $tempRoot"
