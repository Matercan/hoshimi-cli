#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex.h>
#include <string>
#include <vector>

#include "json.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

class WriterBase {
public:
  std::string newContents;

  WriterBase(fs::path writingFile) {
    file = writingFile;

    std::ifstream f(file.string());

    if (!f.is_open()) {
      std::cerr << "Error opening the file: " << file.filename();
    }

    std::string s;
    while (std::getline(f, s)) {
      fileContents += s + "\n";
    }

    f.close();
  }
  WriterBase() {};

private:
  fs::path file;
  std::string fileContents;
};

class QuickshellWriter {
private:
  Colorscheme colors;
  WriterBase colorsWriter;

public:
  QuickshellWriter() {
    colors = ColorsHandler().getColors();
    colorsWriter = WriterBase(fs::path(""));
  }
};

class FilesManager {
private:
  Utils utils;

  std::string HOSHIMI_HOME;
  std::string HOME;
  fs::path DOTFILES_DIRECTORY;
  fs::path BACKUP_DIRECTORY;

  fs::path findHomeEquivilent(fs::path dotfile) { return HOME / findDotfilesRelativePath(dotfile); }
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

  fs::path getQuickshellFolder() {}

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

        for (int i = 0; i < packages.size(); ++i) {

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

        for (int i = 0; i < packages.size(); ++i) {

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

          if (!fs::is_symlink(dir_entry))
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
