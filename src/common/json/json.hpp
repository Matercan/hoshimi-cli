#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cjson/cJSON.h>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <regex>

#include "../colorscheme.hpp"
#include "../utils.hpp"

namespace fs = std::filesystem;

class JsonHandlerBase {
private:
  // Deep merge function for cJSON objects
  // Merges 'override' into 'base', with override values taking precedence
  void deepMergeCJSON(cJSON *base, cJSON *override) {
    if (!base || !override)
      return;
    if (!cJSON_IsObject(base) || !cJSON_IsObject(override))
      return;

    cJSON *overrideItem = NULL;
    cJSON_ArrayForEach(overrideItem, override) {
      if (!overrideItem->string)
        continue;

      cJSON *baseItem =
          cJSON_GetObjectItemCaseSensitive(base, overrideItem->string);

      if (baseItem) {
        // If both are objects, recursively merge
        if (cJSON_IsObject(baseItem) && cJSON_IsObject(overrideItem)) {
          deepMergeCJSON(baseItem, overrideItem);
        }
        // If both are arrays, merge array elements
        else if (cJSON_IsArray(baseItem) && cJSON_IsArray(overrideItem)) {
          // For arrays, we'll replace individual indices

          int overrideSize = cJSON_GetArraySize(overrideItem);
          for (int i = 0; i < overrideSize; i++) {
            cJSON *overrideArrayItem = cJSON_GetArrayItem(overrideItem, i);
            cJSON *baseArrayItem = cJSON_GetArrayItem(baseItem, i);

            if (baseArrayItem && overrideArrayItem) {
              // If both array elements are objects, merge them
              if (cJSON_IsObject(baseArrayItem) &&
                  cJSON_IsObject(overrideArrayItem)) {
                deepMergeCJSON(baseArrayItem, overrideArrayItem);
              } else {
                // Append array item
                cJSON *duplicate = cJSON_Duplicate(overrideArrayItem, true);
                if (duplicate) {
                  cJSON_AddItemToArray(baseItem, duplicate);
                }
              }
            } else if (overrideArrayItem) {
              // Add new array item
              cJSON *duplicate = cJSON_Duplicate(overrideArrayItem, true);
              if (duplicate) {
                cJSON_AddItemToArray(baseItem, duplicate);
                continue;
              }
              continue;
            }
          }
        }
        // Otherwise, replace the value
        else {
          cJSON *duplicate = cJSON_Duplicate(overrideItem, true);
          if (duplicate) {
            cJSON_ReplaceItemInObjectCaseSensitive(base, overrideItem->string,
                                                   duplicate);
          }
        }
      } else {
        // Key doesn't exist in base, add it
        cJSON *duplicate = cJSON_Duplicate(overrideItem, true);
        if (duplicate) {
          cJSON_AddItemToObject(base, overrideItem->string, duplicate);
        }
      }
    }
  }

  // Load theme configuration with base config merging
  cJSON *loadThemeConfig(const char *themeName) {
    // Build paths
    std::string themeDir = std::string(THEMES_PATH) + "/" + themeName;

    std::vector<std::string> themeDirParts;
    std::string defaultDirStr;
    boost::algorithm::split(themeDirParts, themeDir, boost::is_any_of("/\\"));

    for (size_t i = 0; i < themeDirParts.size() - 1; ++i) {
      boost::algorithm::trim(themeDirParts[i]);
      defaultDirStr += themeDirParts[i] + '/';
    }

    defaultDirStr += "*.json";

    std::string themeConfigPath = themeDir + ".json";

    // Start with an empty object
    cJSON *mergedConfig = cJSON_CreateObject();

    // First, load the defaults/*.json file if it exists
    std::string defaultsFile = defaultDirStr;
    cJSON *defaultConfig = getJsonFromFile(defaultsFile.c_str());
    if (defaultConfig) {
      deepMergeCJSON(mergedConfig, defaultConfig);
      cJSON_Delete(defaultConfig);
    }

    // Then, load and merge the theme-specific config
    cJSON *themeConfig = getJsonFromFile(themeConfigPath.c_str());
    if (themeConfig) {
      deepMergeCJSON(mergedConfig, themeConfig);
      cJSON_Delete(themeConfig);
    }

    return mergedConfig;
  }

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
    THEME_CONFIG_JSON = loadThemeConfig(themeName.c_str());
    const char *themeJson = cJSON_Print(THEME_CONFIG_JSON);
    std::ofstream o("tmp/theme.json");
    o << themeJson;
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

  struct CustomWriter {
    std::filesystem::path file;
    int linesDeleted = 0;
    std::vector<std::string> linesAdded;
  };

  struct Config {
    std::string wallpaper;
    std::string osuSkin;
    std::vector<std::string> commands;
    std::vector<CustomWriter> writers;
    // ... any other fields you have in Config
  };

  Config getConfig() {
    Config config;

    auto getObj = [&](cJSON *parent, const char *key) -> cJSON * {
      if (!parent)
        return nullptr;
      return cJSON_GetObjectItemCaseSensitive(parent, key);
    };

    auto getString = [&](cJSON *parent, const char *key) -> std::string {
      cJSON *it = getObj(parent, key);
      if (!it || !cJSON_IsString(it) || !it->valuestring)
        return "";
      const char *s = cJSON_GetStringValue(it);
      return s ? std::string(s) : std::string();
    };

    auto trimTrailingSlashes = [&](std::string &s) {
      while (!s.empty() && s.back() == '/')
        s.pop_back();
    };

    std::string wallpaper = getString(themeConfig, "wallpaper");
    cJSON *globals = getObj(mainConfig, "globals");
    std::string wallpaperDirectory = getString(globals, "wallpaperDirectory");
    trimTrailingSlashes(wallpaperDirectory);

    // expand ~
    const char *home = getenv("HOME");
    if (home) {
      if (wallpaperDirectory.rfind("~/", 0) == 0) {
        wallpaperDirectory = std::string(home) + wallpaperDirectory.substr(1);
      }
    }

    std::vector<std::string> possiblePaths = {
        wallpaperDirectory + wallpaper,
        std::string(home ? home : "") +
            "/.local/share/hoshimi/assets/wallpapers/" + wallpaper,
        wallpaper};

    auto fileExists = [&](const std::string &p) {
      try {
        return fs::exists(p) && fs::is_regular_file(p);
      } catch (...) {
        return false;
      }
    };

    bool found = false;
    for (auto &p : possiblePaths) {
      if (!p.empty() && fileExists(p)) {
        config.wallpaper = p;
        found = true;
        break;
      }
    }
    if (!found)
      config.wallpaper = "";

    // commands (safe)
    cJSON *commandsJson = getObj(THEME_CONFIG_JSON, "commands");
    if (commandsJson && cJSON_IsArray(commandsJson)) {
      for (cJSON *cmd = commandsJson->child; cmd; cmd = cmd->next) {
        if (cJSON_IsString(cmd) && cmd->valuestring) {
          config.commands.emplace_back(cmd->valuestring);
        }
      }
    }

    // writers (safe)
    cJSON *writersJson = getObj(THEME_CONFIG_JSON, "writers");
    if (writersJson && cJSON_IsArray(writersJson)) {
      for (cJSON *writer = writersJson->child; writer; writer = writer->next) {
        if (!cJSON_IsObject(writer))
          continue;
        CustomWriter cw;

        std::string file = getString(writer, "file");
        trimTrailingSlashes(file);
        // expand ~ for writer.file too if desired:
        if (home && file.rfind("~/", 0) == 0)
          file = std::string(home) + file.substr(1);
        cw.file = std::filesystem::path(file);

        cJSON *linesDeletedItem = cJSON_GetObjectItem(writer, "linesDeleted");
        cw.linesDeleted = linesDeletedItem ? linesDeletedItem->valueint : 0;

        cJSON *linesArray = cJSON_GetObjectItem(writer, "lines");
        if (linesArray && cJSON_IsArray(linesArray)) {
          for (cJSON *line = linesArray->child; line; line = line->next) {
            if (cJSON_IsString(line) && line->valuestring) {
              cw.linesAdded.emplace_back(line->valuestring);
            }
          }
        }

        config.writers.push_back(std::move(cw));
      }
    }

    // osu skin
    std::string osuPath = getString(globals, "osuSkin");
    trimTrailingSlashes(osuPath);
    if (home && osuPath.rfind("~/", 0) == 0)
      osuPath = std::string(home) + osuPath.substr(1);
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

    // Safer default value handling (note: JSON stores 1-based indexes in
    // theme files)
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
