# Clean.ps1 - A_WCG Clean Script
# Removes build artifacts

param(
    [switch]$All  # Also remove generated output
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " A_WCG Clean" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Remove build directory
$BuildDir = Join-Path $ProjectDir "build"
if (Test-Path $BuildDir) {
    Write-Host "Removing: $BuildDir" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# Remove generated directory if -All
if ($All) {
    $GeneratedDir = Join-Path $ProjectDir "generated"
    if (Test-Path $GeneratedDir) {
        Write-Host "Removing: $GeneratedDir" -ForegroundColor Yellow
        Remove-Item -Recurse -Force $GeneratedDir
    }
}

Write-Host ""
Write-Host "Clean complete!" -ForegroundColor Green
