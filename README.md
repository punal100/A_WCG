# A_WCG

**Atomic Web Component Generator**

### 🚀 Tagline

_From Browser to Blueprint: Transforming Web Interfaces into P_MWCS-Compatible Assets._

---

## 📝 Project Description

**A_WCG** is a standalone C++ CLI tool that bridges modern web development and Unreal Engine 5.

It functions as the ingestion engine for **P_MWCS** (Modular Widget Creation System). Instead of manually recreating UI layouts in Unreal, A_WCG takes a standard website (HTML/CSS/JS) and programmatically deconstructs it, generating:

- **C++ Class Headers** — Logic and variable bindings (`UFUNCTION`, `UPROPERTY`)
- **JSON Design Schemas** — P_MWCS-compatible widget specifications

> **Important**: A_WCG is a **standalone executable** with no Unreal Engine dependencies. Widget Blueprint creation is exclusively owned by P_MWCS.
> 
> **Current Status (v1.3.3):**
> - ✅ **HTML Structure**: Fully supported (structure & content)
> - ✅ **List Layout**: Ordered/unordered lists display vertically with content wrappers
> - ✅ **Text Rendering**: Black text by default, blue for links (improved preview parity)
> - ✅ **Auto-Wrap**: Enabled for all content text blocks
> - ✅ **Images**: Auto-sized to prevent stretching
> - ✅ **List Markers**: Numbered/bullet markers visible via Design section
> - ⚠️ **CSS Support**: Partial/Experimental (colors, fonts, basic layout)
> - ❌ **JavaScript**: **Not Supported** (Strictly ignored)
> 
> *A_WCG focuses on structural conversion. Visual parity is "best effort" via CSS mapping.*

---

## 🔧 Building from Source

### Requirements

- CMake 3.16+
- MSVC (Visual Studio 2019 or later)
- C++17 support

### Build Steps (Windows + MSVC)

```powershell
cd A_WCG
.\DevTools\scripts\Build.ps1 -Configuration Release
```

Output: `out\build\x64-release\bin\Release\awcg.exe`

---

## 🛠️ Scripts

| Script | Purpose |
|--------|---------|
| `DevTools\scripts\Build.ps1` | Build with MSVC (Debug/Release) |
| `DevTools\scripts\Fetch.ps1` | Download HTML/CSS/JS from a URL |
| `DevTools\scripts\RunPreview.ps1` | Build & Generate Preview (AV-Safe) |
| `DevTools\scripts\Convert.ps1` | Run HTML→Widget conversion |
| `DevTools\scripts\Clean.ps1` | Remove build artifacts |

```powershell
# Build
.\DevTools\scripts\Build.ps1 -Configuration Release

# Fetch website assets (Download only)
.\DevTools\scripts\Fetch.ps1 -Url "https://example.com" -Output .\fetched

# Generate & Preview (No network access - AV Safe)
.\DevTools\scripts\RunPreview.ps1 -Name example_com

# Convert HTML to widget spec
.\DevTools\scripts\Convert.ps1 -Source .\fetched\example_com.html -ClassName MainMenu

# Clean build artifacts
.\DevTools\scripts\Clean.ps1
```

---

## 📊 CLI Usage

### Basic Conversion

```powershell
.\build\bin\Release\awcg.exe --source index.html --output .\generated --class MainMenu
```

### Command-Line Options

| Option | Short | Description |
|--------|-------|-------------|
| `--source` | `-s` | Source HTML file (required) |
| `--css` | | CSS file (auto-detected if same name as HTML) |
| `--output` | `-o` | Output directory (default: `./generated`) |
| `--class` | `-c` | Widget class name (default: from filename) |
| `--module` | `-m` | Module name for C++ API macro |
| `--no-header` | | Skip C++ header generation |
| `--no-json` | | Skip JSON spec generation |
| `--no-preview` | | Skip HTML preview generation |
| `--verbose` | `-v` | Verbose output |
| `--help` | `-h` | Show help |

### Example Workflow

```powershell
# 1. Convert HTML to MWCS spec
.\awcg.exe -s .\WebUI\Menu.html -o .\Generated -c MainMenu -v

# 2. Copy files to UE project
Copy-Item .\Generated\MainMenu.* .\MyProject\Source\MyProject\UI\

# 3. Use P_MWCS to create widget
UnrealEditor-Cmd.exe Project.uproject -run=MWCS_CreateWidgets -unattended -NullRHI
```

---

## ❌ Non-Goals

A_WCG explicitly does **NOT**:

| ❌ | Non-Goal |
|---|----------|
| ❌ | Create or modify Widget Blueprints |
| ❌ | Execute runtime UI logic |
| ❌ | Replace or bypass P_MWCS |
| ❌ | Parse JSX, TSX, Vue SFC, or Svelte source files |
| ❌ | Perform live DOM or browser execution |

---

## 🔑 Key Features

- **Zero UE Dependencies** — Runs anywhere, outputs standard files
- **Automated C++ Binding** — Generates headers with `UFUNCTION` and `UPROPERTY`
- **JSON Layout Schema** — Converts DOM structures into P_MWCS JSON
- **MWCS Inline Properties** — TextBlock widgets include `FontSize` and `Justification` inline in Hierarchy (required by MWCS)
- **List Styling**- Lists (`<ul>`, `<ol>`) automatically receive left padding (40px) to visually distinguish them.
- **Button Child Wrapping**: Since UMG Buttons only support a single child, A_WCG automatically wraps multiple children (e.g. `<li><a href>Icon + Text</a></li>`) into a `HorizontalBox` container.

#### Validation & Parity
- **Type Compatibility**: `TransparentButton` is now strictly validated as a `UButton` subclass, preventing type mismatch errors in P_MWCS.
- **Hierarchy Validation**: Generated widgets are structure-checked against UMG constraints (e.g. ScrollBox child count, Button child count) during generation.
- **HTML Preview** — Browser-accurate preview with proper font sizes, headings, and text extraction
- **CSS Filtering** — Automatically filters ad-blocker CSS pollution
- **Recursive Text** — Extracts text from nested elements (spans inside headings)
- **P_MWCS Optimized** — 100% compatibility with Modular Widget Creation System
- **Stack Agnostic** — Works with raw HTML/CSS or compiled React/Vue/Svelte

---

## 🛡️ CSS Filtering

A_WCG automatically filters problematic CSS that can corrupt widget generation:

### Filtered Style Blocks
Style elements with class names containing the following patterns are **skipped**:
- `abn` (anti-adblock notation)
- `adblock` / `Adblock`

### Why This Matters
Many websites include large ad-blocker CSS blocks (sometimes 200KB+) that contain rules like:
```css
#ad_right,.ad-box { display: none !important; }
```

Without filtering, these styles can incorrectly apply `display: none` to content elements, causing:
- Empty preview HTML (only "A_WCG PREVIEW MODE" visible)
- `Visibility: Collapsed` on all generated UMG widgets
- Corrupted colors from ad-blocker detection styles

---

## 🏛️ P_MWCS Authority

When validating A_WCG output:

| Priority | Source | Authority |
|----------|--------|-----------|
| 1 | P_MWCS schema rules | **Absolute** |
| 2 | `MWCS_ValidateWidgets` output | Authoritative |
| 3 | Extracted widget spec | Reference |
| 4 | A_WCG output | Under test |

**If a mismatch occurs: A_WCG is wrong. P_MWCS is never modified.**

---

## 🚨 Error Codes

| Code | Category | Description |
|------|----------|-------------|
| E001 | Parse Error | HTML/CSS/JS invalid syntax |
| E002 | Mapping Error | Unsupported element/property |
| E003 | Schema Error | JSON rejected by P_MWCS |
| E004 | Parity Error | Round-trip mismatch |

---

## 📁 Project Structure

```
A_WCG/
├── CMakeLists.txt        # CMake build config
├── src/
│   ├── main.cpp          # CLI entry point
│   ├── awcg_types.h      # Core data types (SlotConfig, Color, Anchors)
│   ├── awcg_html_parser.h/cpp
│   ├── awcg_css_parser.h/cpp
│   ├── awcg_element_mapper.h/cpp  # HTML→UMG widget mapping
│   ├── awcg_style_mapper.h/cpp    # CSS→Slot/Design conversion
│   ├── awcg_json_generator.h/cpp  # P_MWCS JSON generation
│   ├── awcg_cpp_generator.h/cpp   # C++ header/source generation
│   └── awcg_html_preview.h/cpp    # Preview HTML generation
├── DevTools/
│   ├── ci/               # CI configuration
│   ├── output/           # Build/test output
│   └── scripts/
│       ├── Build.ps1     # Build script
│       ├── Fetch.ps1     # Website fetcher
│       ├── Convert.ps1   # Conversion script
│       ├── RunPreview.ps1 # AV-safe preview
│       └── Clean.ps1     # Cleanup script
├── README.md
├── GUIDE.md
└── PLAN.md
```

---

## 📚 Documentation

- [GUIDE.md](./GUIDE.md) — Usage guide, supported patterns, troubleshooting
- [P_MWCS README](../README.md) — Parent plugin documentation

---

## 👤 Author

Punal Manalan
