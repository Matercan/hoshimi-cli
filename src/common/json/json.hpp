#pragma once

#include <boost/algorithm/string.hpp>
#include <cjson/cJSON.h>
#include <filesystem>
#include <fstream>
#include <regex>

#include "../colorscheme.hpp"
#include "../utils.hpp"

namespace fs = std::filesystem;

class JsonHandlerBase {
public:
  cJSON *getJsonFromFile(const char *filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
      HERR("JSON") << "Unable to open file for reading: " << filePath << "."
                   << std::endl;
      return nullptr;
    }

    std::string content((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
    input.close();

    cJSON *json = cJSON_Parse(content.c_str());
    if (json == NULL) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL) {
        HERR("JSON") << "Error parsing JSON at: " << &error_ptr << "."
                     << std::endl;
      }
      return nullptr;
    }

    return json;
  }

  JsonHandlerBase() {
    // Get data path
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg_config_home)
      CONFIG_DIRECTORY_PATH =
          fs::path((std::string)xdg_config_home + "/hoshimi/");
    else
      CONFIG_DIRECTORY_PATH = fs::path((std::string)home + "/.config/hoshimi/");
    THEMES_PATH = CONFIG_DIRECTORY_PATH / "themes/";
    MAIN_CONFIG_PATH = CONFIG_DIRECTORY_PATH / "config.json";

    MAIN_CONFIG_JSON = getJsonFromFile(MAIN_CONFIG_PATH.c_str());

    // Helper to safely get a string property from a cJSON object
    auto getStringOrEmpty = [&](cJSON *parent, const char *key) -> std::string {
      if (!parent)
        return std::string();
      cJSON *it = cJSON_GetObjectItemCaseSensitive(parent, key);
      if (!it)
        return std::string();
      if (cJSON_IsString(it) && it->valuestring)
        return std::string(it->valuestring);
      return std::string();
    };

    std::string themeName = getStringOrEmpty(MAIN_CONFIG_JSON, "config");
    if (themeName.empty()) {
      HERR("json " + MAIN_CONFIG_PATH.string())
          << "Warning: 'config' key missing or not a string in main config."
          << std::endl;
      themeName = "default"; // fallback theme name
    }

    THEME_CONFIG_FILE = THEMES_PATH / (themeName + ".json");
    THEME_CONFIG_JSON = getJsonFromFile(THEME_CONFIG_FILE.c_str());
  }

  std::string getThemePath() { return THEME_CONFIG_FILE.string(); }

protected:
  fs::path CONFIG_DIRECTORY_PATH;
  fs::path THEMES_PATH;
  fs::path MAIN_CONFIG_PATH;
  cJSON *MAIN_CONFIG_JSON;
  fs::path THEME_CONFIG_FILE;
  cJSON *THEME_CONFIG_JSON;
};

class ShellHandler : public JsonHandlerBase {
private:
  cJSON *themeConfig;
  cJSON *mainConfig;

public:
  ShellHandler() {
    themeConfig = THEME_CONFIG_JSON;
    mainConfig = MAIN_CONFIG_JSON;
  }

  struct Config {
    fs::path wallpaper;
    char **commands = nullptr;
    fs::path osuSkin;
  };

  Config getConfig() {
    Config config;
    // Safe accessors

    auto getObj = [&](cJSON *parent, const char *key) -> cJSON * {
      if (!parent)
        return nullptr;
      return cJSON_GetObjectItemCaseSensitive(parent, key);
    };

    auto getString = [&](cJSON *parent, const char *key) -> std::string {
      cJSON *it = getObj(parent, key);
      if (!it || !cJSON_IsString(it) || !it->valuestring)
        return std::string();
      return std::string(it->valuestring);
    };

    std::string wallpaper = getString(themeConfig, "wallpaper");
    cJSON *globals = getObj(mainConfig, "globals");
    std::string wallpaperDirectory = getString(globals, "wallpaperDirectory");

    // Helper function to check if file exists
    auto fileExists = [](const std::string &path) {
      return fs::exists(path) && fs::is_regular_file(path);
    };

    // Try different wallpaper paths in order of preference
    std::vector<std::string> possiblePaths = {
        wallpaperDirectory + wallpaper, // wallpaperDirectory + filename
        "~/.local/share/hoshimi/assets/wallpapers/" +
            wallpaper, // default hoshimi assets
        wallpaper      // absolute path or relative to current dir
    };

    // Expand ~ to home directory
    const char *home = getenv("HOME");
    if (home) {
      for (auto &path : possiblePaths) {
        if (path.find("~/", 0) == 0) {
          path = std::string(home) + path.substr(1);
        }
      }
    }

    // Find the first existing file
    bool found = false;
    for (const auto &path : possiblePaths) {
      if (fileExists(path)) {
        config.wallpaper = path;
        found = true;
        break;
      }
    }

    if (!found) {
      HERR("JSON") << "Warning: Wallpaper not found. Tried:" << std::endl;
      for (const auto &path : possiblePaths) {
        HLOG("JSON") << "\t" << path << std::endl;
      }
      config.wallpaper = ""; // or set a default wallpaper
    }

    // Get commands array
    cJSON *commandsJson = getObj(THEME_CONFIG_JSON, "commands");
    if (!cJSON_IsArray(commandsJson)) {
      HERR("JSON " + THEME_CONFIG_FILE.string())
          << "Warning: 'commands' key missing or not an array in main config."
          << std::endl;
      return config;
    }
    for (auto *cmd = commandsJson->child; cmd != NULL; cmd = cmd->next) {
      if (cJSON_IsString(cmd) && cmd->valuestring) {
        config.commands =
            (char **)realloc(config.commands, sizeof(char *) * (1 + 1));
        config.commands[0] = strdup(cmd->valuestring);
        config.commands[1] = NULL; // Null-terminate the array
      }
    }
    std::string osuPath = getString(globals, "osuSkin");
    if (home) {
      if (osuPath.find("~/", 0) == 0) {
        osuPath = std::string(home) + osuPath.substr(1);
      }
    }
    config.osuSkin = osuPath;

    return config;
  }
};

class ColorsHandler : public JsonHandlerBase {
private:
  cJSON *colors;

public:
  ColorsHandler() {
    colors = cJSON_GetObjectItemCaseSensitive(THEME_CONFIG_JSON, "colors");
    if (!colors) {
      throw std::runtime_error("Nonexistant 'colors' object");
    }
  }

  Colorscheme getColors() {

    // Use safer access methods
    auto getObj = [&](cJSON *parent, const char *key) -> cJSON * {
      if (!parent)
        return nullptr;
      return cJSON_GetObjectItemCaseSensitive(parent, key);
    };

    auto getString = [&](cJSON *parent, const char *key) -> std::string {
      cJSON *it = getObj(parent, key);
      if (!it || !cJSON_IsString(it) || !it->valuestring)
        return std::string();
      return std::string(it->valuestring);
    };

    auto getInt = [&](cJSON *parent, const char *key, int def = 0) -> int {
      cJSON *it = getObj(parent, key);
      if (!it)
        return def;
      if (cJSON_IsNumber(it))
        return it->valueint;
      if (cJSON_IsString(it) && it->valuestring)
        return atoi(it->valuestring);
      return def;
    };

    std::string bgStr = getString(colors, "backgroundColor");
    std::string fgStr = getString(colors, "foregroundColor");
    Color backgroundColor = bgStr.empty() ? Color("#000000") : Color(bgStr);
    Color foregroundColor = fgStr.empty() ? Color("#ffffff") : Color(fgStr);

    // Safer default value handling (note: JSON stores 1-based indexes in theme
    // files)
    int activeColorIdx = getInt(colors, "activeColor", 1) - 1;
    int selectedColorIdx = getInt(colors, "selectedColor", 2) - 1;
    int iconColorIdx = getInt(colors, "iconColor", 3) - 1;
    int errorColorIdx = getInt(colors, "errorColor", 4) - 1;
    int passwordColorIdx = getInt(colors, "passwordColor", 5) - 1;
    int borderColorIdx = getInt(colors, "borderColor", 6) - 1;

    std::vector<Color> paletteColors(16, Color("#000000"));

    std::regex paletteColorRegex("^paletteColor(1[0-6]|[1-9])$");

    cJSON *child = colors->child;
    while (child) {
      if (child->string && cJSON_IsString(child) && child->valuestring) {
        std::string key = child->string;
        std::smatch match;
        if (std::regex_match(key, match, paletteColorRegex)) {
          int paletteIndex = std::stoi(match[1].str()) - 1;
          if (paletteIndex >= 0 && paletteIndex < 16) {
            paletteColors[paletteIndex] = Color(child->valuestring);
          }
        }
      }
      child = child->next;
    }

    auto inBounds = [&](int idx) { return idx >= 0 && idx < 16; };
    if (!inBounds(activeColorIdx) || !inBounds(selectedColorIdx) ||
        !inBounds(iconColorIdx) || !inBounds(errorColorIdx) ||
        !inBounds(passwordColorIdx) || !inBounds(borderColorIdx)) {
      throw std::runtime_error("Palette color index out of bounds");
    }

    Color activeColorValue = paletteColors[activeColorIdx];
    Color selectedColorValue = paletteColors[selectedColorIdx];
    Color iconColorValue = paletteColors[iconColorIdx];
    Color errorColorValue = paletteColors[errorColorIdx];
    Color passwordColorValue = paletteColors[passwordColorIdx];
    Color borderColorValue = paletteColors[borderColorIdx];

    Color mainColors[8] = {backgroundColor,    foregroundColor,
                           activeColorValue,   selectedColorValue,
                           iconColorValue,     errorColorValue,
                           passwordColorValue, borderColorValue};

    return Colorscheme(mainColors, paletteColors);
  }
};
