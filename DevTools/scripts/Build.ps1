# Build.ps1 - A_WCG Build Script
# Uses MSVC directly (no GUI) via Developer Command environment

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    
    [switch]$Clean,
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$PresetName = "x64-$($Configuration.ToLower())"
$BuildDir = Join-Path $ProjectDir "out\build\$PresetName"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " A_WCG Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Configuration: $Configuration"
Write-Host ""

# Find Visual Studio
$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswherePath)) {
    Write-Error "Visual Studio Installer not found."
    exit 1
}

$vsPath = & $vswherePath -latest -property installationPath
if (-not $vsPath) {
    Write-Error "Visual Studio not found."
    exit 1
}

# Find VsDevCmd.bat to set up environment
$vsDevCmd = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path $vsDevCmd)) {
    Write-Error "VsDevCmd.bat not found at: $vsDevCmd"
    exit 1
}

Write-Host "Using Visual Studio: $vsPath" -ForegroundColor Gray

# Find CMake in VS
$cmakePath = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmakePath)) {
    $cmakePath = "cmake.exe" # Fallback to PATH
    Write-Warning "CMake not found in VS. using 'cmake.exe' from PATH."
} else {
    Write-Host "Using CMake: $cmakePath" -ForegroundColor Gray
}

# Clean if requested
if ($Clean -or $Rebuild) {
    if (Test-Path $BuildDir) {
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $BuildDir
    }
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

# Build using cmd to properly source VsDevCmd.bat
Write-Host ""
Write-Host "Configuring and building with MSVC..." -ForegroundColor Cyan

# Create a temporary batch file to run cmake with VS environment
$batchContent = @"
@echo off
call "$vsDevCmd" -arch=x64 -no_logo
cd /d "$BuildDir"
"$cmakePath" -G "Visual Studio 17 2022" -A x64 "$ProjectDir"
if errorlevel 1 exit /b 1
"$cmakePath" --build . --config $Configuration
if errorlevel 1 exit /b 1
"@

$batchFile = Join-Path $env:TEMP "awcg_build.bat"
$batchContent | Out-File -FilePath $batchFile -Encoding ASCII

try {
    # Run build directly so output is visible in real-time
    $logFile = Join-Path $BuildDir "build_log.txt"
    & cmd.exe /c "`"$batchFile`"" > $logFile 2>&1
    $exitCode = $LASTEXITCODE
    
    Get-Content $logFile | Write-Host
    
    if ($exitCode -ne 0) {
        Write-Error "Build failed with exit code $exitCode. See log above."
        exit 1
    }
}
finally {
    Remove-Item $batchFile -ErrorAction SilentlyContinue
}

# Report output
$exePath = Join-Path $BuildDir "$Configuration\awcg.exe"
if (Test-Path $exePath) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host " Build Successful!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Executable: $exePath"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  $exePath -s input.html -o .\output -c ClassName"
} else {
    Write-Host ""
    Write-Host "Build completed. Check: $BuildDir" -ForegroundColor Green
}
