<#
.SYNOPSIS
    Copies the engine (TheEngine/) from this repo into another project, so you can
    update the backend without overwriting that project's Game/ (main.cpp, scenes).

.DESCRIPTION
    Run this from the engine repo root (the one you keep updated). It copies:
    - TheEngine/ -> target's TheEngine/ (overwrites)
    - Optionally: vcpkg.json, BACKEND_SWITCHING.md, ENGINE_UPDATING.md
    It does NOT overwrite the target's Game/ folder.

.PARAMETER TargetPath
    Full path to the root of the project you want to update (the one that uses this engine).

.PARAMETER SourcePath
    Root of this engine repo. Defaults to the script's repo root (parent of scripts/).

.PARAMETER CopyVcpkgJson
    If set, copy vcpkg.json to the target. Default: true.

.PARAMETER CopyDocs
    If set, copy BACKEND_SWITCHING.md and ENGINE_UPDATING.md. Default: true.

.PARAMETER WhatIf
    If set, only report what would be copied; do not copy.

.EXAMPLE
    .\scripts\update_engine_in_project.ps1 -TargetPath "C:\MyGames\RacingGame"
.EXAMPLE
    .\scripts\update_engine_in_project.ps1 -TargetPath "D:\Projects\MyGame" -WhatIf
#>
param(
    [Parameter(Mandatory = $true)]
    [string] $TargetPath,

    [Parameter(Mandatory = $false)]
    [string] $SourcePath = (Split-Path -Parent (Split-Path -Parent $PSCommandPath)),

    [bool] $CopyVcpkgJson = $true,
    [bool] $CopyDocs = $true,
    [switch] $WhatIf
)

$ErrorActionPreference = 'Stop'

$engineFolder = 'TheEngine'
$sourceEngine = Join-Path $SourcePath $engineFolder
$targetEngine = Join-Path $TargetPath $engineFolder

if (-not (Test-Path $sourceEngine -PathType Container)) {
    Write-Error "Engine folder not found: $sourceEngine. Run this script from the engine repo (or set -SourcePath)."
}

if (-not (Test-Path $TargetPath -PathType Container)) {
    Write-Error "Target project path does not exist: $TargetPath"
}

function Copy-DirectoryContent {
    param([string]$From, [string]$To, [string]$Label)
    if (-not (Test-Path $From -PathType Container)) {
        Write-Warning "Skip $Label (missing): $From"
        return
    }
    if ($WhatIf) {
        Write-Host "[WhatIf] Would copy directory: $From -> $To"
        return
    }
    if (-not (Test-Path $To -PathType Container)) {
        New-Item -ItemType Directory -Path $To -Force | Out-Null
    }
    robocopy $From $To /E /IS /IT /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) {
        Write-Warning "Robocopy reported issues (exit $LASTEXITCODE) for $Label"
    } else {
        Write-Host "Updated: $Label -> $To"
    }
}

Write-Host "Engine updater: Source=$SourcePath -> Target=$TargetPath"
Write-Host ""

# Copy entire TheEngine/ into target (overwrites target's TheEngine/)
Copy-DirectoryContent -From $sourceEngine -To $targetEngine -Label "TheEngine/"

# Optional root files (never overwrite Game/)
if ($CopyVcpkgJson) {
    $vcpkg = Join-Path $SourcePath 'vcpkg.json'
    if (Test-Path $vcpkg -PathType Leaf) {
        if ($WhatIf) {
            Write-Host "[WhatIf] Would copy: vcpkg.json -> $TargetPath"
        } else {
            Copy-Item -Path $vcpkg -Destination (Join-Path $TargetPath 'vcpkg.json') -Force
            Write-Host "Updated: vcpkg.json"
        }
    }
}

if ($CopyDocs) {
    foreach ($doc in @('BACKEND_SWITCHING.md', 'ENGINE_UPDATING.md')) {
        $src = Join-Path $SourcePath $doc
        if (Test-Path $src -PathType Leaf) {
            if ($WhatIf) {
                Write-Host "[WhatIf] Would copy: $doc -> $TargetPath"
            } else {
                Copy-Item -Path $src -Destination (Join-Path $TargetPath $doc) -Force
                Write-Host "Updated: $doc"
            }
        }
    }
}

Write-Host ""
Write-Host "Done. Your Game/ folder was not modified. Rebuild the target project."
if ($WhatIf) {
    Write-Host "(WhatIf: no files were actually copied.)"
}
