# RunPreview.ps1 - Generate & Compare (Safety Edition)
# NOTE: This script does NOT fetch files to avoid Anti-Virus "Dropper" flags.
# Please run Fetch.ps1 manually if you need to download new assets.

param(
    [Parameter(Mandatory=$true)]
    [string]$Name, # e.g. "webflow-scroll-snap_webflow_io"
    
    [switch]$ForceBuild,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path $MyInvocation.MyCommand.Path -Parent
$RootDir = Split-Path $ScriptDir -Parent

# 1. Validate Input
$fetchedHtml = "$RootDir\fetched\$Name.html"
$fetchedCss = "$RootDir\fetched\$Name.css"

if (-not (Test-Path $fetchedHtml)) {
    Write-Error "Source file not found: $fetchedHtml"
    Write-Warning "Please run: .\Scripts\Fetch.ps1 -Url <URL>"
    exit 1
}

# 2. Build (Delegated to Build.ps1)
Write-Host ">>> STEP 1: Building A_WCG..." -ForegroundColor Cyan
if ($ForceBuild -or $Clean -or -not (Test-Path "$RootDir\out\build\x64-release\bin\Release\awcg.exe")) {
    if (Test-Path "$ScriptDir\Build.ps1") {
        if ($Clean) {
            & "$ScriptDir\Build.ps1" -Configuration Release -Clean
        } else {
            & "$ScriptDir\Build.ps1" -Configuration Release
        }
    } else {
        Write-Error "Build.ps1 not found."
        exit 1
    }
}

# 3. Generate
Write-Host ">>> STEP 2: Generating..." -ForegroundColor Cyan
$ExePath = "$RootDir\out\build\x64-release\bin\Release\awcg.exe"

# Use specific ClassName based on file name logic
$className = $Name -replace '\.', '_'

& $ExePath --source $fetchedHtml --css $fetchedCss --output "$RootDir\generated" --class $className --module "MyProject"

# 4. Compare
Write-Host ">>> STEP 3: Opening Preview..." -ForegroundColor Cyan
$previewPath = "$RootDir\generated\${className}_preview.html"

if (Test-Path $previewPath) {
    Write-Host "Opening Preview: $previewPath" -ForegroundColor Green
    # Use default browser association to be less suspicious than invoking specific exe
    Invoke-Item $previewPath
} else {
    Write-Error "Preview file not found: $previewPath"
}
