# AutoCompare.ps1 - Automated Fetch, Gen, & Compare

param(
    [Parameter(Mandatory=$true)]
    [string]$Url,
    
    [switch]$ForceBuild
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path $MyInvocation.MyCommand.Path -Parent
$RootDir = Split-Path $ScriptDir -Parent

# 1. Fetch
Write-Host ">>> STEP 1: Fetching $Url..." -ForegroundColor Cyan
& "$ScriptDir\Fetch.ps1" -Url $Url -Output "$RootDir\fetched"

# 2. Extract Base Name
$uri = [System.Uri]$Url
$baseName = $uri.Host -replace '\.', '_' 
# Fetch.ps1 logic for name might differ slightly if -Name param used, but default matches this.
# Check actual file
$fetchedHtml = "$RootDir\fetched\$baseName.html"
if (-not (Test-Path $fetchedHtml)) {
    Write-Error "Expected fetched HTML at $fetchedHtml but not found."
}

# 3. Build (Optional/Incremental)
Write-Host ">>> STEP 2: Building A_WCG..." -ForegroundColor Cyan
Set-Location $RootDir
if ($ForceBuild -or -not (Test-Path "$RootDir\out\build\x64-release\bin\Release\awcg.exe")) {
    & "$ScriptDir\Build.ps1" -Configuration Release
}

# 4. Generate
Write-Host ">>> STEP 3: Generating..." -ForegroundColor Cyan
$ExePath = "$RootDir\out\build\x64-release\bin\Release\awcg.exe"
& $ExePath --source "$RootDir\fetched\$baseName.html" --css "$RootDir\fetched\$baseName.css" --output "$RootDir\generated" --class "$baseName" --module "MyProject"

# 5. Compare
Write-Host ">>> STEP 4: Comparing..." -ForegroundColor Cyan
$previewPath = "$RootDir\generated\${baseName}_preview.html"

if (Test-Path $previewPath) {
    Write-Host "Opening Original: $Url" -ForegroundColor Green
    Start-Process "chrome" $Url
    
    Write-Host "Opening Preview: $previewPath" -ForegroundColor Green
    Start-Process "chrome" $previewPath
} else {
    Write-Error "Preview file not found: $previewPath"
}
