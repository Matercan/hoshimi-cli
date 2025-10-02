# hoshimi-cli

Controlling hoshimi dotfiles consistently across every hoshimi dotfile.

<details><summary id="dependencies">External dependencies</summary>

-   [`clang` ](https://github.com/llvm/llvm-project) - Compiling the code
-   [`ninja`](https://github.com/ninja-build/ninja) - Dispatching the compiler
-   [`cmake` ](https://cmake.org/download/) - Telling ninja how to dispatch the compiler
-   [`cJSON`](https://github.com/DaveGamble/cJSON) - JSON parsing for reading your configuration
-   [`boost`](https://github.com/boostorg/boost) - Fast C++ libs for menial tasks

</details>

## Preferred installation

```sh
# clone repository
git clone git@github.com:Matercan/hoshimi-cli.git
cd hoshimi-cli

# configure
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=FALSE \
  -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ \
  --no-warn-unused-cli

# build
cmake -- build build --config Debug --target all --

# (optional)
cp build/bin/hoshimi ~/.local/bin/hoshimi

```

## Usage
```
Hoshimi - Hyprland Dotfiles Manager
===================================

USAGE:
    build/bin/hoshimi <command> [options]

COMMANDS:
    install       Install dotfiles by cloning repository and creating symlinks
    help          Show this help message
    arch-install  Install all the packages neccessary for this shell using paru
    version       Get version information of hoshimi
    update        Update dotfiles to the most recent master commit
    config        Get or set the config options within your configuration
    source        Source the current configuration, updating the modifiable dotfiles 
    restart       (re)start the shell and reload terminals. 

OPTIONS:
    -h, --help                              Show this help message
    -v, --verbose                           Enable verbose output (show detailed operations)
    -f, --force                             Force overwrite existing files without backup
    -p, --packages <pkg1,pkg2,...>          Comma-separated list of packages to install or source
    -np, --not-packages <pkg1,pkg2,...>     Comma-separated list of packages NOT to install or source
    --no-secondary-commands                 Don't do followup commands
    --version                               Show version information


EXAMPLES:
    build/bin/hoshimi install -p hypr,fastfetch -v
    build/bin/hoshimi source -p quickshell
    build/bin/hoshimi arch-install
    build/bin/hoshimi install -np hypr --no-secondary-commands

Subcommands have their own help
```

## Configuring (To be changed)

Configuration taes place in 2 steps. The main config file is located in ~/.config/hoshimi/config.json

The main config file sets up global variables and overrides for the theme, it also tells hoshimi which theme to use.

The themes are located in ~/.config/hoshimi/themes/ and can be pointed to in the main config. Each theme contains the colors, the ordering of the bar as well as the wallpaper fonts etc.

<details><summary>Example main config</summary>

```json
{
  "$schema": "https://raw.githubusercontent.com/Matercan/hoshimi-cli/refs/heads/main/schema/json_schema.json",
  "globals": {
    "iconDirectory": "/usr/share/icons/candy-icons/",
    "wallpaperDirectory": "/home/matercan/Pictures/wallpapers"
  },
  "config": "catppuccin/latte"
}
```

</details>

<details><summary>Example theme config</summary>

```json
{
  "$schema": "https://raw.githubusercontent.com/Matercan/hoshimi-cli/refs/heads/main/schema/theme_schema.json",
  "wallpaper": "akiyama-mizuki.jpg",
  "bar": {
    "globals": {
      "visible": true,
      "position": "left"
    },
    "WorkspaceWidget": {
      "position": 1,
      "visible": true,
      "location": "center"
    },
    "WindowTitle": {
      "position": 0,
      "visible": false,
      "location": "center"
    },
    "Cava": {
      "position": 2,
      "visible": true,
      "location": "bottom"
    },
    "Taskbar": {
      "position": 0,
      "location": "center",
      "visible": false
    },
    "Widgets": {
      "positon": 5,
      "location": "center",
      "visible": true,
      "ClockWidget": {
        "visible": true,
        "location": "center"
      },
      "BatteryWidget": {
        "visible": true,
        "location": "top"
      },
      "VolumeWidget": {
        "visible": true,
        "location": "bottom"
      }
    },
    "TrayWidget": {
      "position": 4,
      "visible": true,
      "location": "bottom"
    },
    "DevButtons": {
      "position": 6,
      "visible": true,
      "location": "bottom"
    }
  },
  "colors": {
    "backgroundColor": "#eff1f5",
    "foregroundColor": "#4c4f69",
    "paletteColor1": "#5c5f77",
    "paletteColor2": "#d20f39",
    "paletteColor3": "#40a02b",
    "paletteColor4": "#df8e1d",
    "paletteColor5": "#1e66f5",
    "paletteColor6": "#ea76cb",
    "paletteColor7": "#179299",
    "paletteColor8": "#acb0be",
    "paletteColor9": "#6c6f85",
    "paletteColor10": "#d20f39",
    "paletteColor11": "40a02b",
    "paletteColor12": "#1e66f5",
    "paletteColor13": "#1e66f5",
    "paletteColor14": "#ea76cb",
    "paletteColor15": "#179299",
    "paletteColor16": "#bcc0cc"
  }
}
```

</details>

