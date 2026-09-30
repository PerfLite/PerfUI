$srcDll = "$PSScriptRoot\build\windows\x64\release\PerfUI.dll"

if (-not (Test-Path -LiteralPath $srcDll)) {
    Write-Error "DLL not found: $srcDll. Build first: xmake build -y"
    exit 1
}

$mo2Bases = @(
    "C:\Games\Skyrim_Test\Mod Organizer",
    "C:\Games\Skyrim\[STB] Mod Organizer"
)

foreach ($mo2 in $mo2Bases) {
    if (-not (Test-Path -LiteralPath $mo2)) {
        continue
    }

    $targetPluginsDir = "$mo2\mods\PerfUI\SKSE\Plugins"
    if (-not (Test-Path -LiteralPath $targetPluginsDir)) {
        New-Item -ItemType Directory -Force -Path $targetPluginsDir | Out-Null
    }

    $destDll = "$targetPluginsDir\PerfUI.dll"
    [System.IO.File]::Copy($srcDll, $destDll, $true)
    Write-Host "[SUCCESS] Deployed PerfUI.dll -> $destDll" -ForegroundColor Green

    # Enable in profiles
    $profilesDir = "$mo2\profiles"
    if (Test-Path -LiteralPath $profilesDir) {
        $profiles = Get-ChildItem -LiteralPath $profilesDir -Directory | Select-Object -ExpandProperty Name
        foreach ($prof in $profiles) {
            $modlistFile = "$profilesDir\$prof\modlist.txt"
            if (Test-Path -LiteralPath $modlistFile) {
                $lines = Get-Content -LiteralPath $modlistFile -Encoding utf8
                if (-not ($lines -match "^\+PerfUI$")) {
                    # If it was -PerfUI, replace it
                    if ($lines -match "^-PerfUI$") {
                        $lines = $lines -replace "^-PerfUI$", "+PerfUI"
                        $lines | Set-Content -LiteralPath $modlistFile -Encoding utf8
                    } else {
                        "+PerfUI" | Set-Content -LiteralPath $modlistFile -Encoding utf8
                        $lines | Add-Content -LiteralPath $modlistFile -Encoding utf8
                    }
                    Write-Host "[SUCCESS] Enabled +PerfUI in profile: $prof ($mo2)" -ForegroundColor Cyan
                } else {
                    Write-Host "[INFO] PerfUI already enabled in profile: $prof ($mo2)" -ForegroundColor Gray
                }
            }
        }
    }
}
