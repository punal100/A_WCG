# A_WCG User Guide

**Version:** 1.0  
**Last Updated:** December 27, 2025

---

## 1. Overview

A_WCG is a **standalone C++ CLI tool** that converts HTML/CSS to P_MWCS-compatible widget specifications.

```text
[HTML/CSS] → [awcg.exe] → [.h + .json] → [P_MWCS] → [Widget Blueprint]
```

---

## 2. Building

```powershell
cd A_WCG
.\Scripts\Build.ps1 -Configuration Release
```

Output: `out\build\x64-release\bin\Release\awcg.exe`

---

## 3. Scripts Reference

| Script | Purpose |
|--------|---------|
| `Scripts\Build.ps1` | CMake + MSVC build |
| `Scripts\Fetch.ps1` | Download website assets |
| `Scripts\Convert.ps1` | Run conversions |
| `Scripts\Clean.ps1` | Remove artifacts |

```powershell
# Build
.\Scripts\Build.ps1 -Configuration Release

# Fetch website
.\Scripts\Fetch.ps1 -Url "https://example.com" -IncludeImages

# Convert
.\Scripts\Convert.ps1 -Source .\fetched\example_com.html -ClassName MainMenu

# Clean
.\Scripts\Clean.ps1 -All
```

---

## 3. Basic Usage

### Convert HTML to Widget Spec

```powershell
.\awcg.exe --source index.html --output .\generated --class MainMenu --verbose
```

### Output Files

| File | Purpose |
|------|---------|
| `MainMenu.json` | P_MWCS widget specification |
| `MainMenu.h` | C++ header with UPROPERTY bindings |
| `MainMenu.cpp` | C++ implementation stub |

---

## 4. Supported HTML Elements

| HTML Tag | Maps To |
|----------|---------|
| `<div>` | VerticalBox / HorizontalBox / CanvasPanel |
| `<button>` | UButton |
| `<span>`, `<p>`, `<h1-6>` | UTextBlock |
| `<img>` | UImage |
| `<input type="text">` | EditableTextBox |
| `<input type="checkbox">` | CheckBox |
| `<select>` | ComboBoxString |
| `<textarea>` | MultiLineEditableTextBox |

### Container Detection

| CSS Display | Widget Type |
|-------------|-------------|
| `display: flex` | HorizontalBox (row) / VerticalBox (column) |
| `display: block` | VerticalBox |
| `position: absolute` | CanvasPanel |

---

## 5. Supported CSS Properties

### Layout & Positioning

| Property | Maps To | Notes |
|----------|---------|-------|
| `position: absolute` | Canvas Slot | Anchors from `top/left/right/bottom` |
| `position: fixed` | Canvas Slot | Same as absolute |
| `display: flex` | Box Slot | `flex-direction` determines H/V |
| `display: block` | Box Slot (Fill) | Block elements fill horizontally by default |
| `width: 100%` | Fill Anchors | Horizontal stretch |
| `height: 100%` | Fill Anchors | Vertical stretch |
| `top/left/right/bottom` | Slot Offsets | For absolute/fixed position |
| `flex-grow` | Size Rule: Fill | Non-zero = fill with weight |
| `align-self` | VAlign | `flex-start`→Top, `center`→Center, `stretch`→Fill |

### Style Properties

| Property | Maps To | Notes |
|----------|---------|-------|
| `color` | ColorAndOpacity | Text color (hex, rgb, rgba, named) |
| `background-color` | Brush.TintColor | Background (ignores transparent) |
| `font-size` | FontSize (inline) | **MWCS requires inline in Hierarchy** |
| `font-weight` | Font.Typeface | 400→Regular, 700→Bold |
| `font-family` | Font.FontObject | Default: Roboto |
| `text-align` | Justification (inline) | **MWCS requires inline in Hierarchy** |
| `opacity` | RenderOpacity | 0.0 - 1.0 |
| `display: none` | Visibility: Collapsed | Element hidden |
| `visibility: hidden` | Visibility: Hidden | Element invisible but takes space |
| `padding` | Widget Padding | |
| `margin` | Slot Padding | |

> **MWCS Format Note**: For TextBlock widgets, `FontSize` and `Justification` are output inline in the Hierarchy JSON (not in Design section). MWCS reads these from the `FMWCS_HierarchyNode` struct.

### Unsupported

- `grid-template-areas` — Use `position: absolute`
- `box-shadow` — Use texture
- `transform` — Use UMG animations
- `filter` — Use post-process

---

## 6. Naming Conventions

### HTML IDs → C++ Names

| HTML ID | C++ Property |
|---------|--------------|
| `id="my-button"` | `UButton* MyButton` |
| `id="submit_form"` | `UButton* SubmitForm` |

### Rules

- Use `snake_case` or `kebab-case` in HTML
- A_WCG converts to `PascalCase`
- Only elements with `id` get UPROPERTY bindings

---

## 7. Error Codes

| Code | Meaning | Fix |
|------|---------|-----|
| E001 | Parse Error | Fix HTML/CSS syntax |
| E002 | Mapping Error | Use supported element |
| E003 | Schema Error | Check P_MWCS spec format |
| E004 | Parity Error | Compare with reference |

---

## 8. The Authority Rule

> **If P_MWCS rejects it, A_WCG is wrong.**

A_WCG is subordinate to P_MWCS. The hierarchy:

1. P_MWCS schema rules — **Absolute**
2. `MWCS_ValidateWidgets` — Authoritative
3. Extracted widget spec — Reference
4. A_WCG output — Under test

---

## 9. Common Issues

### "My styles aren't applying"

1. Check if CSS property is supported (see §5)
2. Ensure selector matches (id vs class)
3. No media queries (not supported)

### "Widget hierarchy is wrong"

1. Check nesting rules (Button can't contain Button)
2. ScrollBox can only have ONE child
3. CanvasPanel children need explicit position

### "Widgets clumped in top-left corner"

1. Root canvas children need `width: 100%` or `height: 100%` in CSS
2. Block elements without explicit width default to fill
3. Check if `display: block` is set on container elements
4. Absolute positioned elements need `top/left/right/bottom` values

### "Generated code doesn't compile"

1. Ensure module name matches your project (`-m YourModule`)
2. Check for missing includes in your UE project
3. Verify API macro (e.g., `YOURMODULE_API`)

---

## 10. Extension Points

| Area | File |
|------|------|
| Element mapping | `src/awcg_element_mapper.cpp` |
| CSS property mapping | `src/awcg_style_mapper.cpp` |
| JSON output format | `src/awcg_json_generator.cpp` |
| C++ output format | `src/awcg_cpp_generator.cpp` |

---

## 11. Why Framework Source Is Unsupported

A_WCG parses **compiled output only** (HTML/CSS/JS).

**Not supported:**
- `.jsx`, `.tsx` (React)
- `.vue` (Vue)
- `.svelte` (Svelte)

**Solution:** Run `npm run build` first, then point A_WCG at the `dist/` folder.

---

## 12. Why Visual Parity Is Not Guaranteed

A_WCG guarantees **schema correctness**, not visual accuracy.

| Guaranteed | Not Guaranteed |
|------------|----------------|
| Widget hierarchy | Pixel-perfect rendering |
| Widget types | Font rendering |
| Slot configuration | Shadow effects |
| Design properties | Animation smoothness |

**Reason:** UMG uses Slate, not browser rendering.
 
 ### JavaScript Support
 **Zero Support.** A_WCG ignores all `<script>` tags. Dynamic behavior (onClick, content loading) must be reimplemented in Unreal C++/Blueprints.
 · **Ignored:** `onclick="..."`, `<script>...</script>`
 · **Removed:** All script content is stripped from the preview to prevent raw code rendering.
 
 ### CSS Support
 **Partial.** A_WCG maps ~60% of common CSS to UMG properties.
 · **Working:** Colors, Fonts, Sizes (px/rem), Padding, Margin
 · **Not Working:** Complex Layouts (Grid), Pseudo-selectors (:hover, :before), Animations, Transitions
 · **Goal:** Structural fidelity, not visual perfection.

---

## 13. Debugging & Verification

### Spec-to-HTML Preview

A_WCG automatically generates a **Preview HTML file** (`[ClassName]_preview.html`) alongside the JSON and C++ files.

**Purpose:**
This preview file visualizes how A_WCG "interprets" the source HTML. It uses the **exact same mapping logic** as the widget generator but outputs HTML/CSS instead of UMG JSON.

**How to use:**
1. Open the original HTML in a browser.
2. Open the generated `_preview.html` in another tab.
3. **Compare them side-by-side.**

| If you see... | It means... |
|---------------|-------------|
| **Identical layout** | A_WCG correctly mapped the structure and styles. |
| **Missing elements** | The elements were ignored or mapped to `display: none`. |
| **Wrong positioning** | The layout relies on unsupported CSS (e.g., Grid) which A_WCG approximated. |
| **Missing colors/fonts** | The CSS parser missed the properties or they are unsupported. |

> **Note:** The preview is NOT a pixel-perfect render of the UMG widget. It is a visualization of the *data* A_WCG extracted. If the preview looks wrong, the UMG widget will definitely look wrong.

### Troubleshooting Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| **Preview shows only "A_WCG PREVIEW MODE" overlay** | Ad-blocker CSS pollution | A_WCG auto-filters style blocks with class `abn_style`, `adblock`. Rebuild with latest version. |
| **All widgets have `Visibility: Collapsed`** | Same as above | The display:none from ad-blocker CSS was applied. Ensure style filtering is active. |
| **Orange/yellow colors appear in preview** | Ad-blocker detection styles | These are telltale signs of ad-blocker CSS pollution. Update A_WCG. |
| **Missing text in TextBlock widgets** | Text inside nested elements | A_WCG now recursively extracts text from nested spans, divs, etc. |
| **Wrong font sizes (all same size)** | Missing browser-default styles | A_WCG now applies tag-specific CSS (h1=2em bold, h2=1.5em, h6=0.67em). |
| **Divs with text showing empty** | Div mapped to container instead of text | Divs with only text content are now auto-detected and mapped to TextBlock. |
| **Wrong positioning** | Unsupported CSS layouts (Grid, complex Flexbox) | A_WCG approximates to block/inline; some layout adjustments may be needed. |
| **Widgets clumped in corner** | Elements not filling parent | Block elements without explicit width now default to fill. Add `width: 100%` in CSS. |

### Preview Features

The preview HTML now includes:

- **Browser-default styling** for h1-h6, p, a elements (proper font sizes, margins, bold)
- **Tag-tracking classes** (e.g., `tag-h1`, `tag-p`) for accurate element styling
- **Recursive text extraction** for nested elements like `<h1><span>Title</span></h1>`
- **Dark theme** (#1a1a1a background) for visibility on all content
- **Image placeholders** (100x100 dashed boxes) for `<img>` elements

---

## See Also

- [README.md](./README.md) — Project overview and build instructions
