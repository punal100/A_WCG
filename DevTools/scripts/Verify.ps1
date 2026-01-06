<#
.SYNOPSIS
    Builds the A_WCG tool and runs verification on valid samples.
#>

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = "$ProjectRoot\out\build"
$BinDir = "$ProjectRoot\out\bin\Debug" # CMake default on Windows usually
$GeneratedDir = "$ProjectRoot\generated"
$FetchedDir = "$ProjectRoot\fetched"

Write-Host "--- A_WCG Verification Script ---" -ForegroundColor Cyan

# 1. Build
Write-Host "1. Building A_WCG..." -ForegroundColor Yellow
if (Test-Path "$ScriptDir\Build.ps1") {
    & "$ScriptDir\Build.ps1"
} else {
    Write-Error "Build.ps1 not found!"
    exit 1
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit 1
}

# 2. Check for Executable
$ExePath = "$BinDir\awcg.exe"
# Start-Process/cmake might output to Release/Debug depending on generator
if (-not (Test-Path $ExePath)) {
    # Try VS CMake Release path
    $ExePath = "$ProjectRoot\out\build\x64-release\bin\Release\awcg.exe"
}
if (-not (Test-Path $ExePath)) {
    # Try Release
    $ExePath = "$ProjectRoot\out\bin\Release\awcg.exe"
}

if (-not (Test-Path $ExePath)) {
    Write-Error "Could not find awcg.exe at $ExePath"
    exit 1
}

Write-Host "Found executable: $ExePath" -ForegroundColor Green

# 3. specific test case: WebflowScrollSnap
$SourceFile = "$FetchedDir\webflow-scroll-snap_webflow_io.html"

if (Test-Path $SourceFile) {
    Write-Host "2. Verifying 'WebflowScrollSnap'..." -ForegroundColor Yellow
    
    $Args = @("-s", $SourceFile, "-o", $GeneratedDir, "-c", "WebflowScrollSnap", "--verbose")
    & $ExePath $Args
    
    $PreviewFile = "$GeneratedDir\WebflowScrollSnap_preview.html"
    if (Test-Path $PreviewFile) {
        Write-Host "SUCCESS: Generated preview at $PreviewFile" -ForegroundColor Green
        # Open it
        Start-Process $PreviewFile
    } else {
        Write-Error "FAILED: Preview file was not generated."
    }
} else {
    Write-Warning "Source file $SourceFile not found. Skipping validation."
}

Write-Host "--- Verification Complete ---" -ForegroundColor Cyan
