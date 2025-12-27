# Convert.ps1 - A_WCG Conversion Script
# Runs the A_WCG CLI to convert HTML/CSS to P_MWCS specs

param(
    [Parameter(Mandatory=$true)]
    [string]$Source,
    
    [string]$Output = ".\generated",
    
    [string]$ClassName,
    
    [string]$CSS,
    
    [string]$Module = "MyProject",
    
    [switch]$NoHeader,
    [switch]$NoJson,
    [switch]$ShowDetails
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir

# Find executable
$exePaths = @(
    # CMake Preset paths (VS 2022)
    "$ProjectDir\out\build\x64-release\bin\Release\awcg.exe",
    "$ProjectDir\out\build\x64-debug\bin\Debug\awcg.exe",
    # Legacy paths
    "$ProjectDir\build\bin\Release\awcg.exe",
    "$ProjectDir\build\bin\Debug\awcg.exe"
)

$exePath = $null
foreach ($path in $exePaths) {
    if (Test-Path $path) {
        $exePath = $path
        break
    }
}

if (-not $exePath) {
    Write-Error "awcg.exe not found. Run .\Scripts\Build.ps1 first."
    exit 1
}

# Build arguments
$args = @("--source", $Source, "--output", $Output, "--module", $Module)

if ($ClassName) {
    $args += @("--class", $ClassName)
}

if ($CSS) {
    $args += @("--css", $CSS)
}

if ($NoHeader) {
    $args += "--no-header"
}

if ($NoJson) {
    $args += "--no-json"
}

if ($ShowDetails) {
    $args += "--verbose"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " A_WCG Conversion" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Source: $Source"
Write-Host "Output: $Output"
Write-Host ""

# Run conversion
& $exePath $args
$exitCode = $LASTEXITCODE

if ($exitCode -eq 0) {
    Write-Host ""
    Write-Host "Conversion complete!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Conversion failed with exit code $exitCode" -ForegroundColor Red
}

exit $exitCode
