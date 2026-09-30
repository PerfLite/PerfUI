$srcDll = "$PSScriptRoot\build\windows\x64\release\PerfUI.dll"

if (-not (Test-Path -LiteralPath $srcDll)) {
    Write-Error "DLL not found: $srcDll. Build first: xmake build -y"
    exit 1
}

$mo2ModDir = "C:\Games\Skyrim\[STB] Mod Organizer\mods\PerfUI"
$targetPluginsDir = "$mo2ModDir\SKSE\Plugins"

# Ensure target directory exists
if (-not (Test-Path -LiteralPath $targetPluginsDir)) {
    New-Item -ItemType Directory -Force -Path $targetPluginsDir | Out-Null
}

$destDll = "$targetPluginsDir\PerfUI.dll"
[System.IO.File]::Copy($srcDll, $destDll, $true)
Write-Host "[SUCCESS] Deployed PerfUI.dll -> $destDll" -ForegroundColor Green

# Ensure it's enabled in STB profile if present
$profiles = @("STB", "TEST")
foreach ($prof in $profiles) {
    $modlistFile = "C:\Games\Skyrim\[STB] Mod Organizer\profiles\$prof\modlist.txt"
    if (Test-Path -LiteralPath $modlistFile) {
        $lines = Get-Content -LiteralPath $modlistFile
        if (-not ($lines -match "^\+PerfUI$")) {
            # Add +PerfUI right at top of active mods
            "+PerfUI" | Set-Content -LiteralPath $modlistFile -Encoding utf8
            $lines | Add-Content -LiteralPath $modlistFile -Encoding utf8
            Write-Host "[SUCCESS] Enabled +PerfUI in profile: $prof" -ForegroundColor Cyan
        } else {
            Write-Host "[INFO] PerfUI already enabled in profile: $prof" -ForegroundColor Gray
        }
    }
}
