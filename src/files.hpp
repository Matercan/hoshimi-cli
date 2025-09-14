#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex.h>
#include <regex>
#include <string>
#include <vector>

#include "json.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

enum FileType { QS, VALUE_PAIR, DEFAULT_VALUE };

class FilesManager {
private:
  Utils utils;

  std::string HOSHIMI_HOME;
  std::string HOME;
  fs::path DOTFILES_DIRECTORY;
  fs::path BACKUP_DIRECTORY;

  fs::path findDotfilesRelativePath(fs::path dotfile) { return fs::relative(dotfile, DOTFILES_DIRECTORY); }
  fs::path findConfigRelativePath(fs::path dotfile) { return fs::relative(dotfile, DOTFILES_DIRECTORY.string() + ".config"); }

public:
  FilesManager() {
    const char *home_env = getenv("HOME");
    if (!home_env) {
      std::cerr << "HOME environment variable not set" << std::endl;
    }
    HOME = home_env;

    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && fs::exists(xdg_data_home)) {
      HOSHIMI_HOME = std::string(xdg_data_home) + "/hoshimi";
    } else {
      HOSHIMI_HOME = HOME + "/.local/share/hoshimi";
    }

    if (!fs::exists(HOSHIMI_HOME)) {
      const std::string DOWNLOAD_COMMAND = "git clone https://github.com/Matercan/hoshimi-dots.git " + HOSHIMI_HOME;

      std::cout << "Running: " << DOWNLOAD_COMMAND << std::endl;
      int result = system(DOWNLOAD_COMMAND.c_str());
      if (result != 0) {
        std::cerr << "Git clone failed" << std::endl;
      }
    }

    DOTFILES_DIRECTORY = HOSHIMI_HOME + "/dotfiles";
    BACKUP_DIRECTORY = fs::current_path() / "backup/";
  }

  bool isModifiable(fs::path dotfile) {
    std::ifstream f(dotfile.string());

    if (!f.is_open()) {
      std::cerr << "Error opening file: " << dotfile.filename() << std::endl;
      std::cout << "Full file path:" << dotfile.string() << std::endl;
      return false;
    }

    std::string s;
    std::getline(f, s);
    return s.find("Hoshimi") != std::string::npos; // if Hoshimi is on the top line, it is modifiable
  }

  fs::path getdotfilesDirectory() { return DOTFILES_DIRECTORY; }
  fs::path getQuickshellFolder() { return DOTFILES_DIRECTORY / ".config/quickshell/"; }
  fs::path findHomeEquivilent(fs::path dotfile) { return HOME / findDotfilesRelativePath(dotfile); }

  int install_dotfiles(std::vector<std::string> packages, bool verbose, bool onlyPackages) {
    // move the current dotfiles into a backup folder

    if (fs::exists(DOTFILES_DIRECTORY)) {
      // Ensure backup directory exists
      if (!fs::exists(BACKUP_DIRECTORY)) {
        fs::create_directories(BACKUP_DIRECTORY);
      } else {
        fs::remove_all(BACKUP_DIRECTORY);
        fs::create_directories(BACKUP_DIRECTORY);
      }

      // Count total files for progress
      size_t total_files = 0;
      for (auto const &dir_entry : fs::recursive_directory_iterator(DOTFILES_DIRECTORY)) {

        std::string const config_relative_path = fs::relative(dir_entry.path(), DOTFILES_DIRECTORY.string() + ".config");
        bool file_in_packages = false;

        for (size_t i = 0; i < packages.size(); ++i) {
          if (config_relative_path.find(packages[i]) != std::string::npos) {
            file_in_packages = true;
            break;
          }
        }

        bool file_installed = (onlyPackages && file_in_packages) || !onlyPackages;

        if (!file_installed)
          continue;

        if (!fs::is_directory(dir_entry)) {
          total_files++;
        }
      }

      if (verbose) {
        std::cout << "Found " << total_files << " files to process.\n" << std::endl;
      }

      size_t processed = 0;
      bool progress_bar_active = false;

      for (auto const &dir_entry : fs::recursive_directory_iterator(DOTFILES_DIRECTORY)) {

        // Clear progress bar before verbose output
        if (progress_bar_active && verbose) {
          std::cout << "\r" << std::string(utils.getTerminalSize().ws_col, ' ') << "\r";
          progress_bar_active = false;
        }
        if (verbose) {
          std::cout << "Processing: " << dir_entry << std::endl;
        }

        fs::path const relative_path = findDotfilesRelativePath(dir_entry.path());
        fs::path const home_equivalent = findHomeEquivilent(dir_entry.path());
        std::string const config_relative_path = findConfigRelativePath(dir_entry.path());
        fs::path const backup_path = BACKUP_DIRECTORY / relative_path;

        bool file_in_packages = false;

        for (size_t i = 0; i < packages.size(); ++i) {

          // Use standard string find instead of boost
          if (config_relative_path.find(packages[i]) != std::string::npos) {
            file_in_packages = true;
            break;
          }
        }

        bool file_installed = (onlyPackages && file_in_packages) || !onlyPackages;

        if (!file_installed)
          continue;

        // First, handle backup of existing files/directories
        if (fs::exists(home_equivalent)) {
          // Create parent directories in backup if needed
          fs::create_directories(backup_path.parent_path());

          if (fs::is_directory(home_equivalent)) {
            if (verbose) {
              std::cout << "Backing up directory: " << home_equivalent << " to " << backup_path << std::endl;
            }
            // For directories, only backup if backup doesn't exist
            if (!fs::exists(backup_path)) {
              fs::create_directory(backup_path);
            }
          } else {
            if (verbose) {
              std::cout << "Backing up file: " << home_equivalent << " to " << backup_path << std::endl;
            }
            fs::rename(home_equivalent, backup_path);
          }
        }

        // Now create symlinks/directories
        if (fs::is_directory(dir_entry)) {
          if (!fs::exists(home_equivalent)) {
            if (verbose) {
              std::cout << "Creating directory: " << home_equivalent << std::endl;
            }
            fs::create_directories(home_equivalent);
          }
        } else {
          // For files, create symlink (file should be backed up by now)
          if (verbose) {
            std::cout << "Creating symlink: " << dir_entry << " -> " << home_equivalent << std::endl;
          }

          // Extra safety check
          if (fs::exists(home_equivalent)) {
            fs::remove(home_equivalent);
          }

          // Create parent directory if it doesn't exist
          fs::create_directories(home_equivalent.parent_path());

          if (isModifiable(dir_entry.path())) {
            std::cout << "\033[1;32mFile " << dir_entry << " modifiable by Hoshimi, symlink will not be created\033[0m" << std::endl;
            fs::copy(dir_entry, home_equivalent);
          } else if (!fs::is_symlink(dir_entry))
            fs::create_symlink(dir_entry, home_equivalent);
          else {
            fs::remove(home_equivalent);
          }
        }

        // Update progress (only for files)
        if (!fs::is_directory(dir_entry)) {
          processed++;

          if (verbose) {
            // Verbose mode: show simple progress
            std::cout << "Progress: " << processed << "/" << total_files << " (" << int((float)processed / total_files * 100.0) << "%)" << std::endl;
          } else {
            // Non-verbose mode: show progress bar
            float progress = (float)processed / total_files;
            utils.print_progress_bar(progress, processed, total_files);
            progress_bar_active = true;
          }
        }
      }

      // Clear progress bar at the end
      if (progress_bar_active) {
        std::cout << "\r" << std::string(utils.getTerminalSize().ws_col, ' ') << "\r";
      }

      return 0;
    } else {
      std::cout << "Dotfiles directory not found: " << DOTFILES_DIRECTORY << std::endl;
      return 1;
    }
  }
};

class WriterBase {
public:
  WriterBase(fs::path writingFile) {
    file = writingFile;

    std::ifstream f(file.string());

    if (!f.is_open()) {
      std::cerr << "Error opening the file: " << file.filename() << std::endl;
      std::cout << "Full file path: " << file.string() << std::endl;
    }

    std::string s;
    while (std::getline(f, s)) {
      fileContents += s + "\n";
    }

    f.close();
    newContents = fileContents;
    filetype = FileType::DEFAULT_VALUE;
  }
  WriterBase() { filetype = FileType::DEFAULT_VALUE; };
  WriterBase(FileType ft) { filetype = ft; }
  WriterBase(fs::path writingFile, FileType ft) {
    file = writingFile;

    std::ifstream f(file.string());

    if (!f.is_open()) {
      std::cerr << "Error opening the file: " << file.filename() << std::endl;
      std::cout << "Full file path: " << file.string() << std::endl;
    }

    std::string s;
    while (std::getline(f, s)) {
      fileContents += s + "\n";
    }

    f.close();
    newContents = fileContents;
    filetype = ft;
  }

  bool writeToFile() {
    std::ofstream o(file.string());

    if (!o.is_open()) {
      std::cerr << "file could not open: " << file.filename() << std::endl;
      std::cout << "Full path: " << file.string() << std::endl;
      return false;
    }

    o << newContents;

    o.close();
    return true;
  }

  void revert() {
    std::ofstream o(file.string());

    if (!o.is_open()) {
      std::cerr << "file could not open: " << file.filename() << std::endl;
      std::cout << "Full path: " << file.string() << std::endl;
      return;
    }

    o << fileContents;

    o.close();
  }

  fs::path getFile() { return file; }

  bool replaceValue(std::string key, std::string value, FileType *fileType) {
    std::regex regex;
    std::string regexReplacement;

    if (filetype != FileType::DEFAULT_VALUE)
      fileType = &filetype;

    if (*fileType == FileType::QS) {
      std::string pattern = "((?:property\\s+\\w+\\s+)?" + key + "\\s*:\\s*)\"[^\"]*\"";
      regex = std::regex(pattern);
      regexReplacement = "$1\"" + value + "\"";
    }
    if (*fileType == FileType::VALUE_PAIR) {
      std::string pattern = "(^|\\n)(" + key + "\\s*=\\s*)([^\\n]*)";
      regex = std::regex(pattern);
      regexReplacement = "$1$2" + value;
    }

    std::string oldContents = newContents;
    newContents = std::regex_replace(newContents, regex, regexReplacement);
    return newContents != oldContents;
  }

  bool replaceWithChecking(std::string key, std::string value) {
    bool exit_code = replaceValue(key, value, nullptr);
    if (!exit_code) {
      std::cerr << "\033[1;31mError writing value " << value << " to " << key << std::endl;
      std::cout << "Make sure you aren't sourcing the same theme as before\033[m" << std::endl;
    }
    return exit_code;
  }

private:
  fs::path file;
  std::string newContents;
  std::string fileContents;
  FileType filetype;
};

class QuickshellWriter {
private:
  Colorscheme colors;
  WriterBase colorsWriter;
  FilesManager files;

public:
  QuickshellWriter() {
    colors = ColorsHandler().getColors();
    colorsWriter = WriterBase(files.findHomeEquivilent(files.getQuickshellFolder() / "functions/Colors.qml"), FileType::QS);
  }

  bool writeColors() {
    bool exitCode = true;

    for (size_t i = 0; i < colors.palette.size(); ++i) {
      colorsWriter.replaceWithChecking("paletteColor" + std::to_string(i + 1), colors.palette[i].toHex());
      std::cout << "paletteColor" + std::to_string(i + 1) << std::endl;
      std::cout << colors.palette[i].toHex() << std::endl;
    }
    colorsWriter.replaceWithChecking("backgroundColor", colors.backgroundColor.toHex());
    colorsWriter.replaceWithChecking("foregroundColor", colors.foregroundColor.toHex());

    for (size_t i = 2; i < colors.main.size(); ++i) {
      auto it = find(colors.palette.begin(), colors.palette.end(), colors.main[i]) - colors.palette.begin();
      colorsWriter.replaceValue(Utils().COLOR_NAMES[i], "paletteColor" + std::to_string(it),
                                nullptr); // Don't check this one because it will be similar each run
    }

    if (!colorsWriter.writeToFile()) {
      exitCode = false;
      std::cerr << "Error writing to file: " << colorsWriter.getFile().filename() << std::endl;
    }

    // if (!exitCode)
    //  colorsWriter.revert();

    return exitCode;
  }
};

class GhosttyWriter {
private:
  Colorscheme colors;
  WriterBase writer;
  FilesManager files;

public:
  GhosttyWriter() {
    colors = ColorsHandler().getColors();
    writer = WriterBase(files.findHomeEquivilent(files.getdotfilesDirectory() / ".config/ghostty/themes/hoshimi"), FileType::VALUE_PAIR);
  }

  bool writeConfig() {
    bool exitCode = true;

    writer.replaceWithChecking("background", colors.backgroundColor.toHex());
    writer.replaceWithChecking("foreground", colors.foregroundColor.toHex());
    writer.replaceWithChecking("cursor-color", colors.selectedColor.toHex());
    writer.replaceWithChecking("cursor-text", colors.selectedColor.toHex());
    writer.replaceWithChecking("selection-background", colors.activeColor.toHex());
    writer.replaceWithChecking("selection-foreground", colors.activeColor.toHex());

    for (int i = 0; i < 16; ++i) {
      writer.replaceValue("palette = " + std::to_string(i), colors.palette[i].toHex(), nullptr);
    }

    if (!writer.writeToFile()) {
      exitCode = false;

      std::cerr << "Error writing to file: " << writer.getFile().filename() << std::endl;
    }

    // if (!exitCode)
    // writer.revert();

    return exitCode;
  }
};
