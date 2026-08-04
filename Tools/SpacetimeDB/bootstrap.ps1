$ErrorActionPreference = "Stop"

$SpacetimeDbVersion = "2.7.1"
$SpacetimeDbTag = "v$SpacetimeDbVersion"
$UpstreamRepository = "https://github.com/clockworklabs/SpacetimeDB.git"
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$ModuleDirectory = Join-Path $ProjectRoot "spacetimedb"
$PluginDirectory = Join-Path $ProjectRoot "Plugins/SpacetimeDbSdk"
$VersionMarker = Join-Path $PluginDirectory ".aetherfront-version"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required to fetch the pinned Unreal SDK."
}
if (-not (Get-Command spacetime -ErrorAction SilentlyContinue)) {
    throw "SpacetimeDB CLI $SpacetimeDbVersion is required: https://spacetimedb.com/install"
}

if (Test-Path $PluginDirectory -PathType Container) {
    if (-not (Test-Path $VersionMarker -PathType Leaf)) {
        throw "$PluginDirectory exists but has no Aetherfront version marker. Move it aside and rerun."
    }
    $InstalledVersion = (Get-Content $VersionMarker -Raw).Trim()
    if ($InstalledVersion -ne $SpacetimeDbVersion) {
        throw "$PluginDirectory contains SDK $InstalledVersion; version $SpacetimeDbVersion is required."
    }
    Write-Host "SpacetimeDB Unreal SDK $SpacetimeDbVersion is already installed."
}
else {
    $TempDirectory = Join-Path ([IO.Path]::GetTempPath()) ("aetherfront-spacetimedb-" + [Guid]::NewGuid())
    try {
        git clone --depth 1 --branch $SpacetimeDbTag --filter=blob:none --sparse $UpstreamRepository (Join-Path $TempDirectory "upstream")
        if ($LASTEXITCODE -ne 0) { throw "Failed to clone SpacetimeDB $SpacetimeDbTag." }

        $UpstreamDirectory = Join-Path $TempDirectory "upstream"
        git -C $UpstreamDirectory sparse-checkout set sdks/unreal/src/SpacetimeDbSdk
        if ($LASTEXITCODE -ne 0) { throw "Failed to select the SpacetimeDB Unreal SDK." }

        New-Item -ItemType Directory -Force -Path (Join-Path $ProjectRoot "Plugins") | Out-Null
        Copy-Item -Recurse -Path (Join-Path $UpstreamDirectory "sdks/unreal/src/SpacetimeDbSdk") -Destination $PluginDirectory
        Set-Content -Path $VersionMarker -Value $SpacetimeDbVersion -NoNewline
        Write-Host "Installed the pinned SpacetimeDB Unreal SDK $SpacetimeDbVersion."
    }
    finally {
        if (Test-Path $TempDirectory) {
            Remove-Item -Recurse -Force -Path $TempDirectory
        }
    }
}

Push-Location $ModuleDirectory
try {
    spacetime build
    if ($LASTEXITCODE -ne 0) { throw "SpacetimeDB module build failed." }
}
finally {
    Pop-Location
}

spacetime generate --lang unrealcpp --uproject-dir $ProjectRoot --module-path $ModuleDirectory --unreal-module-name Aetherfront
if ($LASTEXITCODE -ne 0) { throw "Unreal binding generation failed." }

Write-Host "SpacetimeDB module built and Unreal bindings generated."
