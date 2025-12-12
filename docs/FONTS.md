# TinyVK Font System

## Overview

TinyVK includes embedded fonts for easy text rendering without requiring external font files. Roboto Medium is the default font.

## File Organization

```
include/tinyvk/assets/
├── fonts.h                  # Font data declarations
└── icons_font_awesome.h     # Font Awesome 6 icon definitions

src/assets/
└── fonts.cpp                # Embedded font binary data
```

## Available Fonts

### Sans-Serif Fonts

- **Roboto Medium** (default) - Clean, modern Google font
- **Lexend** (Regular, Light, SemiBold, Bold, Black, Thin) - Highly readable
- **Quicksand** (Regular, Light, Medium, SemiBold, Bold) - Rounded, friendly
- **Droid Sans** - Android system font

### Monospace Fonts

- **Proggy Clean** - Bitmap programming font
- **Proggy Tiny** - Compact bitmap font

### Icon Fonts

- **Font Awesome 6** - 1000+ icons (Regular & Solid variants)

## Usage

### Using Default Font (Roboto)

```cpp
tvk::App app;
app.Run("My App", 1280, 720);  // Uses Roboto by default
```

### Changing Embedded Font

```cpp
tvk::App app;
tvk::AppConfig config;
config.embeddedFontName = "lexend";  // or "quicksand", "droid"
app.Run(config);
```

### Using Custom Font File

```cpp
tvk::AppConfig config;
config.useEmbeddedFont = false;
config.fontPath = "/path/to/font.ttf";
config.fontSize = 18.0f;
app.Run(config);
```

### Font Scaling

```cpp
tvk::AppConfig config;
config.fontScale = 1.5f;  // 150% size
app.Run(config);
```

## Font Awesome Icons

Include icons in your ImGui code:

```cpp
#include <tinyvk/tinyvk.h>
#include <imgui.h>

void OnUI() override {
    ImGui::Begin("Tools");
    
    // Using icon constants
    if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open")) {
        // Open file dialog
    }
    
    if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save")) {
        // Save file
    }
    
    ImGui::Text(ICON_FA_CIRCLE_INFO " Information");
    ImGui::End();
}
```

### Loading Font Awesome

To use icons, merge Font Awesome with your main font:

```cpp
#include <imgui.h>
#include <tinyvk/assets/fonts.h>
#include <tinyvk/assets/icons_font_awesome.h>

ImGuiIO& io = ImGui::GetIO();

// Main font
io.Fonts->AddFontFromMemoryTTF(
    tvk::roboto_medium, 
    tvk::roboto_medium_size, 
    16.0f
);

// Merge icons
static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
ImFontConfig icons_config;
icons_config.MergeMode = true;
icons_config.PixelSnapH = true;

io.Fonts->AddFontFromFileTTF(
    "assets/fonts/fa-solid-900.ttf",
    16.0f,
    &icons_config,
    icons_ranges
);
```

## Font Data Format

All fonts are embedded as byte arrays:

```cpp
namespace tvk {
    extern unsigned char roboto_medium[];
    extern unsigned int roboto_medium_size;
}
```

Access them directly if needed:

```cpp
ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
    tvk::quicksand_bold,
    tvk::quicksand_bold_size,
    20.0f
);
```

## Naming Conventions

Following TinyVK standards:
- **Namespace**: `tvk` (not `Quasar`)
- **Variables**: `snake_case` (e.g., `roboto_medium`)
- **Headers**: lowercase with underscores (e.g., `fonts.h`, `icons_font_awesome.h`)

## Adding New Fonts

1. Convert font to C array (using tools like `xxd` or `bin2c`)
2. Add declaration to `include/tinyvk/assets/fonts.h`
3. Add data to `src/assets/fonts.cpp`
4. Update font loading in `src/ui/imgui_layer.cpp`

Example:

```bash
# Convert TTF to C array
xxd -i MyFont.ttf > my_font.cpp

# Format as:
namespace tvk {
unsigned char my_font[] = { /* data */ };
unsigned int my_font_size = 12345;
}
```

## Performance

- Fonts are embedded in binary, no file I/O at runtime
- Loaded into GPU memory once during initialization
- Minimal overhead compared to external font files

## License Notes

- **Roboto**: Apache 2.0 (Google)
- **Lexend**: SIL Open Font License
- **Quicksand**: SIL Open Font License
- **Droid Sans**: Apache 2.0
- **Proggy**: Public Domain
- **Font Awesome**: SIL OFL 1.1 (icons), MIT (code)

Ensure compliance with these licenses in your projects.
