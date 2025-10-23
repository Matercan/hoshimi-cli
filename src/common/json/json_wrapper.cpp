#include "json_wrapper.h"
#include "../utils.h"
#include "json.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <zip.h>

extern "C" {

Config *load_config(void) {
  try {
    ShellHandler handler;
    auto cppConfig = handler.getConfig();

    Config *cConfig = (Config *)malloc(sizeof(Config));
    if (!cConfig)
      return nullptr;

    // Copy wallpaper path
    std::string wallpaperStr = cppConfig.wallpaper;
    cConfig->wallpaper = strdup(wallpaperStr.c_str());

    // Copy commands array
    if (!cppConfig.commands.empty()) {
      // Count commands
      int count = cppConfig.commands.size();

      cConfig->commands = (char **)malloc(sizeof(char *) * (count + 1));
      for (int i = 0; i < count; i++) {
        cConfig->commands[i] = strdup(cppConfig.commands[i].c_str());
      }
      cConfig->commands[count] = nullptr;
    } else {
      cConfig->commands = nullptr;
    }

    cConfig->osuSkin = (char *)malloc(5);

    strcpy(cConfig->osuSkin, "osu/");

    char *home = getHoshimiHome(NULL);
    char downloadPath[128];
    snprintf(downloadPath, sizeof(downloadPath), "%s/assets/osuGen", home);

    cConfig->downloadPath = strdup(downloadPath);

    free(home);

    return cConfig;
  } catch (...) {
    return nullptr;
  }
}

void free_config(Config *config) {
  if (!config)
    return;

  if (config->wallpaper) {
    free(config->wallpaper);
  }

  if (config->commands) {
    for (int i = 0; config->commands[i] != nullptr; i++) {
      free(config->commands[i]);
    }
    free(config->commands);
  }

  if (config->downloadPath) {
    free(config->downloadPath);
  }

  if (config->osuSkin) {
    free(config->osuSkin);
  }

  free(config);
}

C_ColorScheme *load_colorscheme(void) {
  try {
    ColorsHandler handler;
    auto cppColors = handler.getColors();

    C_ColorScheme *cColors = (C_ColorScheme *)malloc(sizeof(C_ColorScheme));
    if (!cColors)
      return nullptr;

    // Helper to convert Color to ColorRGB
    auto toRGB = [](const Color &c) -> ColorRGB {
      ColorRGB rgb;
      rgb.r = c.r;
      rgb.g = c.g;
      rgb.b = c.b;
      return rgb;
    };

    cColors->background = toRGB(cppColors.backgroundColor);
    cColors->foreground = toRGB(cppColors.foregroundColor);
    cColors->active = toRGB(cppColors.activeColor);
    cColors->selected = toRGB(cppColors.selectedColor);
    cColors->icon = toRGB(cppColors.iconColor);
    cColors->error = toRGB(cppColors.errorColor);
    cColors->password = toRGB(cppColors.passwordColor);
    cColors->border = toRGB(cppColors.borderColor);

    for (int i = 0; i < 16; i++) {
      cColors->palette[i] = toRGB(cppColors.palette[i]);
    }

    return cColors;
  } catch (...) {
    return nullptr;
  }
}

void free_colorscheme(C_ColorScheme *colors) {
  if (colors) {
    free(colors);
  }
}

} // extern "C"
