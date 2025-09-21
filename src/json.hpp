#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>

#include "colorscheme.hpp"

namespace fs = std::filesystem;

class JsonHandlerBase {
public:
  JsonHandlerBase() {
    // Get data path
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg_config_home)
      CONFIG_DIRECTORY_PATH = fs::path((std::string)xdg_config_home + "/hoshimi/");
    else
      CONFIG_DIRECTORY_PATH = fs::path((std::string)home + "/.config/hoshimi/");
    THEMES_PATH = CONFIG_DIRECTORY_PATH / "themes/";
    MAIN_CONFIG_PATH = CONFIG_DIRECTORY_PATH / "config.json";

    MAIN_CONFIG_JSON = readJsonFile(MAIN_CONFIG_PATH.string());
    THEME_CONFIG_FILE = THEMES_PATH.string() + MAIN_CONFIG_JSON["config"].asString() + ".json"; // Fixed: use [] instead of find()
    THEME_CONFIG_JSON = readJsonFile(THEME_CONFIG_FILE.string());
  }

  std::string getThemePath() { return THEME_CONFIG_FILE.string(); }

protected:
  fs::path CONFIG_DIRECTORY_PATH;
  fs::path THEMES_PATH;
  fs::path MAIN_CONFIG_PATH;
  Json::Value MAIN_CONFIG_JSON;
  fs::path THEME_CONFIG_FILE;
  Json::Value THEME_CONFIG_JSON;

private:
  Json::Value readJsonFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file: " + filePath);
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;

    if (!Json::parseFromStream(builder, file, &root, &errors)) {
      throw std::runtime_error("Failed to parse JSON: " + errors);
    }

    file.close();
    return root;
  }
};

class ShellHandler : public JsonHandlerBase {
private:
  Json::Value themeConfig;
  Json::Value mainConfig;

public:
  struct Config {
    fs::path wallpaper;
  };

  ShellHandler() {
    themeConfig = THEME_CONFIG_JSON;
    mainConfig = MAIN_CONFIG_JSON;
  }

  Config getConfig() {
    Config config;
    std::string wallpaper = themeConfig["wallpaper"].asString();
    std::string wallpaperDirectory = MAIN_CONFIG_JSON["globals"]["wallpaperDirectory"].asString();

    // Helper function to check if file exists
    auto fileExists = [](const std::string &path) { return fs::exists(path) && fs::is_regular_file(path); };

    // Try different wallpaper paths in order of preference
    std::vector<std::string> possiblePaths = {
        wallpaperDirectory + wallpaper,                          // wallpaperDirectory + filename
        "~/.local/share/hoshimi/assets/wallpapers/" + wallpaper, // default hoshimi assets
        wallpaper                                                // absolute path or relative to current dir
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
      std::cerr << "Warning: Wallpaper not found. Tried:" << std::endl;
      for (const auto &path : possiblePaths) {
        std::cerr << "  " << path << std::endl;
      }
      config.wallpaper = ""; // or set a default wallpaper
    }

    return config;
  }
};

class ColorsHandler : public JsonHandlerBase {
private:
  Json::Value colors;

public:
  ColorsHandler() {

    if (!THEME_CONFIG_JSON.isMember("colors")) {
      throw std::runtime_error("Nonexistant 'colors' object");
    }

    colors = THEME_CONFIG_JSON["colors"];
  }

  Colorscheme getColors() {

    // Use safer access methods
    Color backgroundColor = colors["backgroundColor"].asString();
    Color foregroundColor = colors["foregroundColor"].asString();

    // Safer default value handling
    uint8_t activeColor = colors.get("activeColor", 5).asInt() - 1; // Convert to 0-based index
    uint8_t selectedColor = colors.get("selectedColor", 6).asInt() - 1;
    uint8_t iconColor = colors.get("iconColor", 13).asInt() - 1;
    uint8_t errorColor = colors.get("errorColor", 2).asInt() - 1;
    uint8_t passwordColor = colors.get("passwordColor", 4).asInt() - 1;
    uint8_t borderColor = colors.get("borderColor", 5).asInt() - 1;

    std::vector<Color> paletteColors(16);

    std::regex paletteColorRegex("^paletteColor(1[0-6]|[1-9])$");

    for (Json::Value::const_iterator it = colors.begin(); it != colors.end(); ++it) {
      std::string key = it.key().asString();
      std::smatch match;

      if (std::regex_match(key, match, paletteColorRegex)) {
        int paletteIndex = std::stoi(match[1].str()) - 1; // Convert to 0-based index
        if (paletteIndex >= 0 && paletteIndex < 16) {
          paletteColors[paletteIndex] = Color(it->asString());
        }
      }
    }

    // Bounds checking for palette access
    if (activeColor >= 16 || selectedColor >= 16 || iconColor >= 16 || errorColor >= 16 || passwordColor >= 16 || borderColor >= 16) {
      throw std::runtime_error("Palette color index out of bounds");
    }

    Color activeColorValue = paletteColors[activeColor];
    Color selectedColorValue = paletteColors[selectedColor];
    Color iconColorValue = paletteColors[iconColor];
    Color errorColorValue = paletteColors[errorColor];
    Color passwordColorValue = paletteColors[passwordColor];
    Color borderColorValue = paletteColors[borderColor];

    Color mainColors[8] = {backgroundColor, foregroundColor, activeColorValue,   selectedColorValue,
                           iconColorValue,  errorColorValue, passwordColorValue, borderColorValue};

    return Colorscheme(mainColors, paletteColors);
  }
};
