# Fetch.ps1 - Website Asset Fetcher
# Downloads HTML, CSS, and JS from a website URL

param(
    [Parameter(Mandatory=$true)]
    [string]$Url,
    
    [string]$Output = ".\fetched",
    
    [string]$Name,
    
    [switch]$IncludeImages,
    [switch]$ShowDetails
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " A_WCG Website Fetcher" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "URL: $Url"
Write-Host "Output: $Output"
Write-Host ""

# Parse URL for base and name
$uri = [System.Uri]$Url
$baseName = if ($Name) { $Name } else { $uri.Host -replace '\.', '_' }
$baseUrl = "$($uri.Scheme)://$($uri.Host)"

# Create output directory
if (-not (Test-Path $Output)) {
    New-Item -ItemType Directory -Path $Output -Force | Out-Null
}

# Download HTML
Write-Host "Fetching HTML..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri $Url -UseBasicParsing
    $html = $response.Content
    $htmlPath = Join-Path $Output "$baseName.html"
    $html | Out-File -FilePath $htmlPath -Encoding UTF8
    Write-Host "  Saved: $htmlPath" -ForegroundColor Green
} catch {
    Write-Error "Failed to fetch HTML: $_"
    exit 1
}

# Extract and download CSS
Write-Host "Extracting CSS links..." -ForegroundColor Yellow
$cssLinks = [regex]::Matches($html, '<link[^>]+href=[''"]([^''"]+\.css[^''"]*)[''"]') | 
    ForEach-Object { $_.Groups[1].Value }

$cssContent = ""
$cssCount = 0
foreach ($cssLink in $cssLinks) {
    try {
        # Resolve relative URLs
        if ($cssLink -match '^//') {
            $cssUrl = "$($uri.Scheme):$cssLink"
        } elseif ($cssLink -match '^/') {
            $cssUrl = "$baseUrl$cssLink"
        } elseif ($cssLink -notmatch '^https?://') {
            $cssUrl = "$baseUrl/$cssLink"
        } else {
            $cssUrl = $cssLink
        }
        
        if ($ShowDetails) { Write-Host "  Fetching: $cssUrl" -ForegroundColor Gray }
        $css = (Invoke-WebRequest -Uri $cssUrl -UseBasicParsing).Content
        $cssContent += "/* Source: $cssUrl */`n$css`n`n"
        $cssCount++
    } catch {
        Write-Warning "  Failed to fetch CSS: $cssLink"
    }
}

# Extract inline styles
$inlineStyles = [regex]::Matches($html, '<style[^>]*>([\s\S]*?)</style>') | 
    ForEach-Object { $_.Groups[1].Value }
foreach ($style in $inlineStyles) {
    $cssContent += "/* Inline style */`n$style`n`n"
    $cssCount++
}

if ($cssContent) {
    $cssPath = Join-Path $Output "$baseName.css"
    $cssContent | Out-File -FilePath $cssPath -Encoding UTF8
    Write-Host "  Saved: $cssPath ($cssCount sources)" -ForegroundColor Green
}

# Extract and download JS
Write-Host "Extracting JS scripts..." -ForegroundColor Yellow
$jsLinks = [regex]::Matches($html, '<script[^>]+src=[''"]([^''"]+\.js[^''"]*)[''"]') | 
    ForEach-Object { $_.Groups[1].Value }

$jsContent = ""
$jsCount = 0
foreach ($jsLink in $jsLinks) {
    try {
        # Resolve relative URLs
        if ($jsLink -match '^//') {
            $jsUrl = "$($uri.Scheme):$jsLink"
        } elseif ($jsLink -match '^/') {
            $jsUrl = "$baseUrl$jsLink"
        } elseif ($jsLink -notmatch '^https?://') {
            $jsUrl = "$baseUrl/$jsLink"
        } else {
            $jsUrl = $jsLink
        }
        
        if ($ShowDetails) { Write-Host "  Fetching: $jsUrl" -ForegroundColor Gray }
        $js = (Invoke-WebRequest -Uri $jsUrl -UseBasicParsing).Content
        $jsContent += "/* Source: $jsUrl */`n$js`n`n"
        $jsCount++
    } catch {
        Write-Warning "  Failed to fetch JS: $jsLink"
    }
}

# Extract inline scripts
$inlineScripts = [regex]::Matches($html, '<script(?![^>]+src=)[^>]*>([\s\S]*?)</script>') | 
    ForEach-Object { $_.Groups[1].Value } | Where-Object { $_.Trim() }
foreach ($script in $inlineScripts) {
    $jsContent += "/* Inline script */`n$script`n`n"
    $jsCount++
}

if ($jsContent) {
    $jsPath = Join-Path $Output "$baseName.js"
    $jsContent | Out-File -FilePath $jsPath -Encoding UTF8
    Write-Host "  Saved: $jsPath ($jsCount sources)" -ForegroundColor Green
}

# Optionally download images
if ($IncludeImages) {
    Write-Host "Extracting images..." -ForegroundColor Yellow
    $imgDir = Join-Path $Output "images"
    if (-not (Test-Path $imgDir)) {
        New-Item -ItemType Directory -Path $imgDir -Force | Out-Null
    }
    
    $imgLinks = [regex]::Matches($html, '<img[^>]+src=[''"]([^''"]+)[''"]') | 
        ForEach-Object { $_.Groups[1].Value }
    
    $imgCount = 0
    foreach ($imgLink in $imgLinks) {
        try {
            if ($imgLink -match '^data:') { continue }  # Skip data URIs
            
            # Resolve relative URLs
            if ($imgLink -match '^//') {
                $imgUrl = "$($uri.Scheme):$imgLink"
            } elseif ($imgLink -match '^/') {
                $imgUrl = "$baseUrl$imgLink"
            } elseif ($imgLink -notmatch '^https?://') {
                $imgUrl = "$baseUrl/$imgLink"
            } else {
                $imgUrl = $imgLink
            }
            
            $imgName = Split-Path $imgLink -Leaf
            $imgPath = Join-Path $imgDir $imgName
            
            if ($ShowDetails) { Write-Host "  Fetching: $imgUrl" -ForegroundColor Gray }
            Invoke-WebRequest -Uri $imgUrl -OutFile $imgPath -UseBasicParsing
            $imgCount++
        } catch {
            Write-Warning "  Failed to fetch image: $imgLink"
        }
    }
    Write-Host "  Saved: $imgCount images to $imgDir" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host " Fetch Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Files saved to: $Output"
Write-Host ""
Write-Host "Next step - convert to widget spec:"
Write-Host "  .\Scripts\Convert.ps1 -Source `"$htmlPath`" -ClassName $baseName"
