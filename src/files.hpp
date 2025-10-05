#include "common/json/json.hpp"
#include "common/utils.hpp"
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

namespace fs = std::filesystem;

enum FileType { QS, VALUE_PAIR, DEFAULT_VALUE };

class FilesManager {
public:
  fs::path getdotfilesDirectory() { return DOTFILES_DIRECTORY; }
  fs::path getQuickshellFolder() {
    return DOTFILES_DIRECTORY / ".config/quickshell/";
  }
  fs::path findHomeEquivilent(fs::path dotfile) {
    return HOME / findDotfilesRelativePath(dotfile);
  }

  bool isModifiable(fs::path dotfile) {
    std::ifstream f(dotfile.string());

    if (!f.is_open()) {
      HERR("install " + dotfile.string())
          << "Error opening file" << "." << std::endl;
      return false;
    }

    std::string s;
    std::getline(f, s);
    return s.find("Hoshimi") !=
           std::string::npos; // if Hoshimi is on the top line, it is modifiable
  }

private:
  Utils utils;

  std::string HOSHIMI_HOME;
  std::string HOME;
  fs::path DOTFILES_DIRECTORY;
  fs::path BACKUP_DIRECTORY;

  fs::path findDotfilesRelativePath(const fs::path &dotfile) {
    std::vector<std::string> path;
    boost::algorithm::split(path, dotfile.string(), boost::is_any_of("/"),
                            boost::token_compress_on);

    auto it = std::find(path.begin(), path.end(), "dotfiles");
    if (it == path.end()) {
      // "hoshimi" not found, return empty or handle error
      return fs::path{};
    }

    // Erase everything up to and including "hoshimi"
    path.erase(path.begin(), it + 1);

    std::string pathS;
    for (const auto &component : path) {

      pathS += component;
      if (component != path[path.size() - 1])
        pathS += "/";
    }
    return pathS;
  }

  fs::path findConfigRelativePath(const fs::path &dotfile) {
    std::vector<std::string> path;
    boost::algorithm::split(path, dotfile.string(), boost::is_any_of("/"),
                            boost::token_compress_on);

    auto it = std::find(path.begin(), path.end(), ".config");
    if (it == path.end()) {
      // ".config" not found, return empty or handle error
      return fs::path{};
    }

    // Erase everything up to and including ".config"
    path.erase(path.begin(), it + 1);

    std::string pathS;
    for (const auto &component : path) {
      pathS += component + "/";
    }
    return pathS;
  }

  void install_file(const fs::directory_entry &dir_entry, size_t &processed,
                    size_t &total_files, bool &progress_bar_active,
                    const bool &verbose,
                    const std::vector<std::string> &packages,
                    const std::vector<std::string> &notPackages,
                    const bool &onlyPackages) {
    // Add mutex for thread safety
    static std::mutex progress_mutex;

    // Clear progress bar before verbose output
    if (progress_bar_active && verbose) {
      std::cout << "\r" << std::string(utils.getTerminalSize().ws_col, ' ')
                << "\r";
      progress_bar_active = false;
    }
    if (verbose) {
      HLOG("install") << "Processing: " << dir_entry << "." << std::endl;
    }

    fs::path const relative_path = findDotfilesRelativePath(dir_entry.path());
    fs::path const home_equivalent = findHomeEquivilent(dir_entry.path());
    fs::path const backup_path = BACKUP_DIRECTORY / relative_path;
    std::string const config_relative_path =
        findConfigRelativePath(dir_entry.path());
    if (verbose) {
      HLOG("install") << "Checking path: " << config_relative_path << "."
                      << std::endl;
    }

    // Check if file matches any package in the include list
    bool file_in_packages = false;
    if (!packages.empty()) {
      file_in_packages = std::any_of(
          packages.begin(), packages.end(),
          [&config_relative_path](const std::string &pkg) {
            return config_relative_path.find(pkg) != std::string::npos;
          });
    }

    // Check if file matches any package in the exclude list
    bool file_excluded = false;
    if (!notPackages.empty()) {
      file_excluded = std::any_of(
          notPackages.begin(), notPackages.end(),
          [&config_relative_path](const std::string &pkg) {
            return config_relative_path.find(pkg) != std::string::npos;
          });
    }

    // Determine if file should be installed
    bool file_installed =
        !file_excluded && (!onlyPackages || (onlyPackages && file_in_packages));

    if (verbose) {
      HLOG("install " + dir_entry.path().string())
          << " in_packages: " << file_in_packages
          << " excluded: " << file_excluded
          << " will_install: " << file_installed << "." << std::endl;
    }

    if (!file_installed)
      return;

    // First, handle backup of existing files/directories
    if (fs::exists(home_equivalent)) {
      // Create parent directories in backup if needed
      fs::create_directories(backup_path.parent_path());

      if (fs::is_directory(home_equivalent)) {
        if (verbose) {
          HLOG("install") << "Backing up directory: " << home_equivalent
                          << " to " << backup_path << "." << std::endl;
        }
        // For directories, only backup if backup doesn't exist
        if (!fs::exists(backup_path)) {
          fs::create_directory(backup_path);
        }
      } else {
        if (verbose) {
          HLOG("install") << "Backing up file: " << home_equivalent << " to "
                          << backup_path << "." << std::endl;
        }
        fs::rename(home_equivalent, backup_path);
      }
    }

    // Now create symlinks/directories
    if (fs::is_directory(dir_entry)) {
      if (!fs::exists(home_equivalent)) {
        if (verbose) {
          HLOG("install") << "Creating directory: " << home_equivalent << "."
                          << std::endl;
        }
        fs::create_directories(home_equivalent);
      }
    } else {
      // For files, create symlink (file should be backed up by now)
      if (verbose) {
        HLOG("install") << "Creating symlink: " << dir_entry << " -> "
                        << home_equivalent << "." << std::endl;
      }

      // Extra safety check
      if (fs::exists(home_equivalent)) {
        fs::remove(home_equivalent);
      }

      // Create parent directory if it doesn't exist
      try {
        fs::create_directories(home_equivalent.parent_path());
      } catch (const fs::filesystem_error &e) {
        HERR("install " + dir_entry.path().string())
            << "Filesystem error creating parent directories: " << e.what()
            << std::endl;
        return;
      }

      // Add static mutex for cout synchronization
      static std::mutex cout_mutex;

      if (isModifiable(dir_entry.path())) {
        {
          std::lock_guard<std::mutex> cout_lock(cout_mutex);
          if (processed < total_files)
            HLOG("install " + dir_entry.path().string())
                << "File modifiable by Hoshimi, symlink will not be created."
                << std::endl;
        }
        fs::copy(dir_entry, home_equivalent);
      } else if (!fs::is_symlink(dir_entry)) {
        {
          std::lock_guard<std::mutex> cout_lock(cout_mutex);
          if (verbose)
            HLOG("install")
                << "Creating symlink for: " << dir_entry.path().filename()
                << "." << std::endl;
        }
        fs::create_symlink(dir_entry, home_equivalent);
      } else {
        {
          std::lock_guard<std::mutex> cout_lock(cout_mutex);
          if (verbose)
            HLOG("install")
                << "Removing existing symlink: " << dir_entry.path().filename()
                << "." << std::endl;
        }
        fs::remove(home_equivalent);
      }
    }

    // Update progress (only for files)
    if (!fs::is_directory(dir_entry)) {
      std::lock_guard<std::mutex> lock(progress_mutex);
      processed++;

      if (verbose) {
        HLOG("install") << "Progress: " << processed << "/" << total_files
                        << " (" << int((float)processed / total_files * 100.0)
                        << "%)" << "." << std::endl;
      } else if (processed <= total_files) { // Add bounds check
        float progress = (float)processed / total_files;
        utils.print_progress_bar(progress, processed, total_files);
        progress_bar_active = true;
      }
    }

    return;
  }

  void installDirectory(const fs::directory_entry &dir_entry, size_t &processed,
                        size_t &total_files, bool &progress_bar_active,
                        const bool &verbose,
                        const std::vector<std::string> &packages,
                        const std::vector<std::string> &notPackages,
                        const bool &onlyPackages) {

    static std::mutex threads_mutex;
    std::vector<std::thread> threads;
    const size_t max_threads = std::thread::hardware_concurrency();

    // Add error handling for directory access
    std::error_code ec;
    fs::directory_iterator dir_it(
        dir_entry, fs::directory_options::skip_permission_denied, ec);

    if (ec) {
      if (verbose) {
        std::cerr << "Warning: Could not access directory " << dir_entry.path()
                  << ": " << ec.message() << std::endl;
      }
      return;
    }

    for (auto const &entry : dir_it) {
      try {
        if (fs::is_directory(entry, ec)) {
          if (!ec) {
            installDirectory(entry, processed, total_files, progress_bar_active,
                             verbose, packages, notPackages, onlyPackages);
          }
        } else if (!ec) {
          std::lock_guard<std::mutex> lock(threads_mutex);

          // Wait if we have too many threads
          if (threads.size() >= max_threads) {
            for (auto &t : threads) {
              if (t.joinable())
                t.join();
            }
            threads.clear();
          }

          threads.emplace_back([this, entry, &processed, &total_files,
                                &progress_bar_active, &verbose, &packages,
                                &notPackages, &onlyPackages]() {
            install_file(entry, processed, total_files, progress_bar_active,
                         verbose, packages, notPackages, onlyPackages);
          });
        }
      } catch (const fs::filesystem_error &e) {
        if (verbose) {
          HERR(std::string("install") + dir_entry.path().string())
              << "Filesystem error: " << e.what() << std::endl;
        }
        continue;
      }
    }

    // Wait for remaining threads
    for (auto &t : threads) {
      if (t.joinable())
        t.join();
    }
  }

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
      const std::string DOWNLOAD_COMMAND =
          "git clone https://github.com/Matercan/hoshimi-dots.git " +
          HOSHIMI_HOME;

      HLOG("Install") << "Cloning dotfiles from GitHub to " << HOSHIMI_HOME
                      << "." << std::endl;
      HLOG("Install") << "Running: " << DOWNLOAD_COMMAND << "." << std::endl;
      int result = system(DOWNLOAD_COMMAND.c_str());
      if (result != 0) {
        std::cerr << "Git clone failed" << std::endl;
      }
    }

    DOTFILES_DIRECTORY = HOSHIMI_HOME + "/dotfiles";
    BACKUP_DIRECTORY = HOSHIMI_HOME + "/backup";
  }

  int install_dotfiles(std::vector<std::string> packages,
                       std::vector<std::string> notPackages, bool verbose,
                       bool onlyPackages) {
    if (!fs::exists(DOTFILES_DIRECTORY)) {
      HERR("Install " + DOTFILES_DIRECTORY.string())
          << " Directory not found." << std::endl;
      return 1;
    }

    // Ensure backup directory exists
    if (!fs::exists(BACKUP_DIRECTORY)) {
      fs::create_directories(BACKUP_DIRECTORY);
    } else {
      fs::remove_all(BACKUP_DIRECTORY);
      fs::create_directories(BACKUP_DIRECTORY);
    }

    // Count total files first
    size_t total_files = 0;
    std::mutex count_mutex;

    try {
      for (const auto &dir_entry :
           fs::recursive_directory_iterator(DOTFILES_DIRECTORY)) {
        if (!fs::is_directory(dir_entry)) {
          std::string const config_relative_path = fs::relative(
              dir_entry.path(), DOTFILES_DIRECTORY.string() + ".config");

          // Use same logic as install_file
          bool file_in_packages = false;
          if (!packages.empty()) {
            file_in_packages = std::any_of(
                packages.begin(), packages.end(),
                [&config_relative_path](const std::string &pkg) {
                  return config_relative_path.find(pkg) != std::string::npos;
                });
          }

          bool file_excluded = false;
          if (!notPackages.empty()) {
            file_excluded = std::any_of(
                notPackages.begin(), notPackages.end(),
                [&config_relative_path](const std::string &pkg) {
                  return config_relative_path.find(pkg) != std::string::npos;
                });
          }

          bool will_install =
              !file_excluded &&
              (!onlyPackages || (onlyPackages && file_in_packages));

          if (will_install) {
            std::lock_guard<std::mutex> lock(count_mutex);
            total_files++;
          }
        }
      }
    } catch (const fs::filesystem_error &e) {
      HERR("Install " + DOTFILES_DIRECTORY.string())
      " Filesystem error: " << e.what() << std::endl;
      return 1;
    }

    if (verbose) {
      HLOG("Install") << "Total files to process: " << total_files << "."
                      << std::endl;
    }

    size_t processed = 0;
    bool progress_bar_active = false;

    for (auto const &dir_entry :
         fs::recursive_directory_iterator(DOTFILES_DIRECTORY)) {
      installDirectory(dir_entry, processed, total_files, progress_bar_active,
                       verbose, packages, notPackages, onlyPackages);
    }

    // Clear progress bar at the end
    if (progress_bar_active) {
      std::cout << "\r" << std::string(utils.getTerminalSize().ws_col, ' ')
                << "\r";
      std::cout.flush();
    }

    return 0;
  }
};

class WriterBase {
private:
  fs::path file;
  std::string newContents;
  std::string fileContents;
  FileType filetype;

public:
  std::string Contents() { return newContents; }

  WriterBase(fs::path writingFile) {
    file = writingFile;

    std::ifstream f(file.string());

    if (!f.is_open()) {
      HERR("Config " + file.string())
          << " File opening unsuccessful" << std::endl;
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
      HERR("Config " + file.string())
          << " File opening unsuccessful" << std::endl;
    }

    std::string s;
    while (std::getline(f, s)) {
      fileContents += s + "\n";
    }

    f.close();
    newContents = fileContents;
    filetype = ft;
  }

  bool write() {
    std::ofstream o(file.string());

    if (!o.is_open()) {
      HERR("Config " + file.string())
          << " File opening unsuccessful" << std::endl;
      return false;
    }

    o << newContents;

    o.close();
    return true;
  }
  void write(const std::string &filePath) {
    std::ofstream o(filePath);

    if (!o.is_open()) {
      HERR("Config " + filePath) << " File opening unsuccessful" << std::endl;
      return;
    }

    o << newContents;

    o.close();
    return;
  }

  void revert() {
    std::ofstream o(file.string());

    if (!o.is_open()) {
      HERR("Config " + file.string())
          << " File opening unsuccessful" << std::endl;
      return;
    }

    o << fileContents;

    o.close();
  }

  fs::path getFile() { return file; }

  // Empty file (from given point)
  void empty() { newContents = ""; }
  void empty(const int &line) {
    std::istringstream stream(newContents);
    std::string line_content;
    std::string updated_contents;
    int current_line = 0;

    while (std::getline(stream, line_content)) {
      if (current_line == line)
        break;
      updated_contents += line_content + "\n";
      current_line++;
    }

    newContents = updated_contents;
  }
  void empty(const char *text) {
    std::istringstream stream(newContents);
    std::string line_content;
    std::string updated_contents;

    while (std::getline(stream, line_content)) {
      if (line_content.find(text) != std::string::npos)
        break;
      updated_contents += line_content + "\n";
    }

    newContents = updated_contents;
  }

  // Append contents to file
  void append(std::string text) { newContents += text; }
  void append(const char *text) { newContents += std::string(text); }
  void append(const std::string &text, const int &line) {
    std::istringstream stream(newContents);
    std::string line_content;
    std::string updated_contents;
    int current_line = 0;

    while (std::getline(stream, line_content)) {
      if (current_line == line) {
        updated_contents += text + "\n";
      }
      updated_contents += line_content + "\n";
      current_line++;
    }

    // If the specified line is beyond the current number of lines, append at
    // the end
    if (line >= current_line) {
      updated_contents += text + "\n";
    }

    newContents = updated_contents;
  }
  void appendBeforeLine(const std::string &text, const int &line) {
    std::istringstream stream(newContents);
    std::string line_content;
    std::string updated_contents;
    int current_line = 0;

    while (std::getline(stream, line_content)) {
      if (current_line == line) {
        updated_contents += text + "\n";
      }
      updated_contents += line_content + "\n";
      current_line++;
    }

    // If the specified line is beyond the current number of lines, append at
    // the end
    if (line >= current_line) {
      updated_contents += text + "\n";
    }

    newContents = updated_contents;
  }

  void writeLine(const std::string &text, const int &line) {
    std::istringstream stream(newContents);
    std::string line_content;
    std::string updated_contents;
    int current_line = 0;

    while (std::getline(stream, line_content)) {
      if (current_line == line) {
        updated_contents += text + "\n";
      } else {
        updated_contents += line_content + "\n";
      }
      current_line++;
    }

    // If the specified line is beyond the current number of lines, append at
    // the end
    if (line >= current_line) {
      updated_contents += text + "\n";
    }

    newContents = updated_contents;
  }

  bool replaceValue(const std::string &key, const std::string &value,
                    FileType *fileType) {
    if (filetype != FileType::DEFAULT_VALUE)
      fileType = &filetype;

    std::string updatedContents;
    std::string line;
    std::istringstream newContentsStream(newContents);

    if (*fileType == FileType::QS) {
      bool written = false;
      while (getline(newContentsStream, line)) {
        if (written) {
          updatedContents += line + "\n";
          continue;
        }
        if (boost::algorithm::contains(line, key + ":")) {
          if (line.find(":") == std::string::npos) {
            updatedContents += line + "\n";
            continue;
          }

          std::string newLine = "";
          std::vector<std::string> split;

          boost::algorithm::split(split, line, boost::is_any_of(":"),
                                  boost::token_compress_on);

          newLine += split[0] + ": " + value;
          updatedContents += newLine + "\n";
          written = true;
          continue;
        } else {
          updatedContents += line + "\n";
        }
      }
    }
    if (*fileType == FileType::VALUE_PAIR) {
      while (getline(newContentsStream, line)) {
        bool written = false;
        if (written) {
          updatedContents += line + "\n";
          continue;
        }
        if (boost::algorithm::contains(line, key)) {
          std::string newLine;
          std::vector<std::string> split;
          boost::algorithm::split(split, line, boost::is_any_of("="),
                                  boost::token_compress_on);

          for (size_t i = 0; i < split.size() - 1; ++i)
            newLine += split[i] + "=";

          newLine += value;
          updatedContents += newLine + "\n";
          written = true;
        } else {
          updatedContents += line + "\n";
        }
      }
    }

    std::string oldContents = newContents;
    newContents = updatedContents;
    return newContents != oldContents;
  }
  bool replaceValue(std::string key, std::string value, std::string afterLine) {

    auto fileType = &filetype;
    std::string updatedContents;
    std::string line;
    std::istringstream newContentsStream(newContents);
    bool foundLine = false;

    if (*fileType == FileType::QS) {
      bool written = false;
      while (getline(newContentsStream, line)) {
        if (written) {
          updatedContents += line + "\n";
          continue;
        } else if (!foundLine) {
          if (boost::algorithm::contains(line, afterLine)) {
            foundLine = true;
          }
          updatedContents += line + "\n";
        } else if (boost::algorithm::contains(line, key + ":")) {
          if (line.find(":") == std::string::npos) {
            updatedContents += line + "\n";
            continue;
          }

          std::string newLine = "";
          std::vector<std::string> split;

          boost::algorithm::split(split, line, boost::is_any_of(":"),
                                  boost::token_compress_on);

          newLine += split[0] + ": " + value;
          updatedContents += newLine + "\n";
          written = true;
          continue;
        } else {
          updatedContents += line + "\n";
        }
      }
    } else if (*fileType == FileType::VALUE_PAIR) {
      bool written = false;
      while (getline(newContentsStream, line)) {
        if (written) {
          updatedContents += line + "\n";
          continue;
        } else if (!foundLine) {
          if (boost::algorithm::contains(line, afterLine)) {
            foundLine = true;
          }
          updatedContents += line + "\n";
        } else if (boost::algorithm::contains(line, key)) {
          std::string newLine;
          std::vector<std::string> split;
          boost::algorithm::split(split, line, boost::is_any_of("="),
                                  boost::token_compress_on);

          for (size_t i = 0; i < split.size() - 1; ++i)
            newLine += split[i] + "=";

          newLine += value;
          updatedContents += newLine + "\n";
        } else {
          updatedContents += line + "\n";
        }
      }
    }

    std::string oldContents = newContents;
    newContents = updatedContents;
    return newContents != oldContents;
  }

  bool replaceWithChecking(std::string key, std::string value) {
    bool exit_code = replaceValue(key, value, nullptr);
    if (!exit_code) {
      HERR("Config " + file.string())
          << " Value of " << key << " not changed." << std::endl;
      write("tmp/" + key + file.filename().string());
      HLOG("Config " + file.string()) << " Current contents written to /tmp/" +
                                             key + file.filename().string()
                                      << std::endl;
    }
    return exit_code;
  }
  void replaceWithChecking(std::string key, std::string value, bool &exitCode) {
    if (!replaceValue(key, value, nullptr)) {
      HERR("Config " + file.string())
          << " Value of " << key << " not changed." << std::endl;
      exitCode = false;
      write("tmp/" + key + file.filename().string());
      HLOG("Config " + file.string()) << " Current contents written to tmp/" +
                                             key + file.filename().string()
                                      << std::endl;
    }
  }
  void replaceWithChecking(std::string key, std::string value,
                           std::string afterLine, bool &exitCode) {
    if (!replaceValue(key, value, afterLine)) {
      HERR("Config " + file.string())
          << "Value of " << key << " after line containing " << afterLine
          << " not changed. " << std::endl;
      exitCode = false;
      write("tmp/" + key + file.filename().string());
      HLOG("Config " + file.string()) << " Current contents written to tmp/" +
                                             key + file.filename().string()
                                      << std::endl;
    }
  }
};

class QuickshellWriter {
private:
  Colorscheme colors;
  ShellHandler::Config config;
  WriterBase *colorsWriter;
  WriterBase *shellWriter;
  FilesManager files;

public:
  QuickshellWriter() {
    colors = ColorsHandler().getColors();
    config = ShellHandler().getConfig();
    colorsWriter =
        new WriterBase(files.findHomeEquivilent(files.getQuickshellFolder() /
                                                "functions/Colors.qml"),
                       FileType::QS);
    shellWriter =
        new WriterBase(files.findHomeEquivilent(files.getQuickshellFolder() /
                                                "globals/Variables.qml"),
                       FileType::QS);
  }

  ~QuickshellWriter() {
    delete colorsWriter;
    delete shellWriter;
  }

  bool writeColors() {
    if (!files.isModifiable(colorsWriter->getFile())) {
      HERR("Config " + colorsWriter->getFile().string())
          << "File not modifiable by Hoshimi, skipping." << std::endl;
      colorsWriter->appendBeforeLine(
          "// File not modifiable by Hoshimi, skipping.\n", 0);
    }
    bool exitCode = true;
    if (colors.backgroundColor.light())
      colorsWriter->replaceWithChecking("light", "true");
    else
      colorsWriter->replaceWithChecking("light", "false");

    for (size_t i = 0; i < colors.palette.size(); ++i) {
      colorsWriter->replaceWithChecking(
          "paletteColor" + std::to_string(i + 1),
          colors.palette[i].toHex(Color::FLAGS::WQUOT));
    }
    colorsWriter->replaceWithChecking(
        "backgroundColor", colors.backgroundColor.toHex(Color::FLAGS::WQUOT),
        exitCode);
    colorsWriter->replaceWithChecking(
        "foregroundColor", colors.foregroundColor.toHex(Color::FLAGS::WQUOT),
        exitCode);

    for (size_t i = 2; i < colors.main.size(); ++i) {
      colorsWriter->replaceWithChecking(
          Utils().COLOR_NAMES[i], colors.main[i].toHex(Color::FLAGS::WQUOT),
          exitCode);
    }

    if (!colorsWriter->write()) {
      exitCode = false;
      std::cerr << "Error writing to file: "
                << colorsWriter->getFile().filename() << std::endl;
    }

    if (!exitCode)
      colorsWriter->revert();

    return exitCode;
  }

  bool writeShell() {
    bool exitCode = true;

    shellWriter->replaceWithChecking(
        "wallpaper", "\"" + config.wallpaper.string() + "\"", exitCode);
    shellWriter->replaceValue(
        "osuDirectory",
        "\"" + (config.osuSkin.parent_path() / "osuGen").string() + "\"",
        nullptr);

    if (!shellWriter->write()) {
      exitCode = false;
      HERR("Config " + shellWriter->getFile().string())
          << "Error writing to file." << std::endl;
    }

    if (!exitCode)
      shellWriter->revert();

    return exitCode;
  }
};

class GhosttyWriter {
private:
  Colorscheme colors;
  WriterBase writer;
  FilesManager files;

public:
  void reloadGhostty() {
    system("# Trigger ghostty config reload\n"
           "if pgrep -x ghostty &> /dev/null; then\n"
           "ghostty_addresses=$(hyprctl clients -j | jq -r \'.[] | "
           "select(.class == \"com.mitchellh.ghostty\") | .address\')\n"

           "if [[ -n \"$ghostty_addresses\" ]]; then\n"
           "# Save current active window\n"
           "current_window=$(hyprctl activewindow -j | jq -r '.address')\n"

           "# Focus each ghostty window and send reload key combo\n"
           "while IFS= read -r address; do\n"
           "hyprctl dispatch focuswindow \"address:$address\" > /dev/null &\n"
           "sleep 0.1\n"
           "hyprctl dispatch sendshortcut \"CTRL SHIFT, comma, "
           "address:$address\" > /dev/null &\n"
           "done <<< \"$ghostty_addresses\"\n"

           "# Return focus to original window\n"
           "if [[ -n \"$current_window\" ]]; then\n"
           "hyprctl dispatch focuswindow \"address:$current_window\" > "
           "/dev/null &\n"
           "fi\n"
           "fi\n"
           "fi\n");
  }

  GhosttyWriter() {
    colors = ColorsHandler().getColors();
    writer =
        WriterBase(files.findHomeEquivilent(files.getdotfilesDirectory() /
                                            ".config/ghostty/themes/hoshimi"),
                   FileType::VALUE_PAIR);
  }

  bool writeConfig() {
    bool exitCode = true;

    writer.replaceWithChecking("background", colors.backgroundColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("foreground", colors.foregroundColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("cursor-color", colors.selectedColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("cursor-text", colors.selectedColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("selection-background",
                               colors.activeColor.toHex(), exitCode);
    writer.replaceWithChecking("selection-foreground",
                               colors.activeColor.toHex(), exitCode);

    for (int i = 0; i < 16; ++i) {
      writer.replaceValue("palette = " + std::to_string(i),
                          colors.palette[i].toHex(), nullptr);
    }

    if (!writer.write()) {
      exitCode = false;

      HERR("Config " + writer.getFile().string())
          << "Error writing to file." << std::endl;
    }

    if (!exitCode)
      writer.revert();
    else {
      reloadGhostty();
    }

    return exitCode;
  }
};

// Writer for foot terminal (uses key: value and palette entries)
class FootWriter {
private:
  Colorscheme colors;
  WriterBase writer;
  FilesManager files;

public:
  FootWriter() {
    colors = ColorsHandler().getColors();
    writer = WriterBase(files.findHomeEquivilent(files.getdotfilesDirectory() /
                                                 ".config/foot/foot.ini"),
                        FileType::VALUE_PAIR);
  }

  bool writeConfig() {
    bool exitCode = true;

    writer.replaceWithChecking(
        "background", colors.backgroundColor.toHex(Color::FLAGS::NHASH),
        exitCode);
    writer.replaceWithChecking(
        "foreground", colors.foregroundColor.toHex(Color::FLAGS::NHASH),
        exitCode);
    writer.replaceWithChecking(
        "selection-background",
        colors.foregroundColor.toHex(Color::FLAGS::NHASH), exitCode);
    for (int i = 0; i < 8; ++i) {
      writer.replaceWithChecking("regular" + std::to_string(i),
                                 colors.palette[i].toHex(Color::FLAGS::NHASH),
                                 exitCode);
    }
    writer.append("\n");
    for (int i = 8; i < 16; ++i) {
      writer.replaceWithChecking("bright" + std::to_string(i - 8),
                                 colors.palette[i].toHex(Color::FLAGS::NHASH),
                                 exitCode);
    }

    if (!writer.write()) {
      exitCode = false;
      HERR("Config " + writer.getFile().string())
          << "Error writing to file." << std::endl;
    }

    if (!exitCode)
      writer.revert();

    return exitCode;
  }
};

// Writer for kitty terminal
class KittyWriter {
private:
  Colorscheme colors;
  WriterBase writer;
  FilesManager files;

public:
  KittyWriter() {
    colors = ColorsHandler().getColors();
    writer = WriterBase(files.findHomeEquivilent(files.getdotfilesDirectory() /
                                                 ".config/kitty/hoshimi.conf"),
                        FileType::VALUE_PAIR);
  }

  void reloadKitty() {
    // Send SIGUSR1 to kitty to reload config if running
    system(
        "if pgrep -x kitty > /dev/null; then kill -USR1 $(pgrep -x kitty); fi");
  }

  bool writeConfig() {
    bool exitCode = true;

    writer.replaceWithChecking("background", colors.backgroundColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("foreground", colors.foregroundColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("cursor", colors.selectedColor.toHex(),
                               exitCode);
    writer.replaceWithChecking("selection_background",
                               colors.activeColor.toHex(), exitCode);

    for (int i = 0; i < 16; ++i) {
      // kitty palette entries are usually 'color0 #000000'
      writer.replaceValue("color" + std::to_string(i),
                          colors.palette[i].toHex(), nullptr);
    }

    if (!writer.write()) {
      exitCode = false;
      HERR("Config " + writer.getFile().string())
          << "Error writing to file." << std::endl;
    }

    if (!exitCode)
      writer.revert();
    else
      reloadKitty();

    return exitCode;
  }
};

class AlacrittyWriter {
private:
  Colorscheme colors;
  FilesManager files;
  WriterBase writer;

  fs::path getAlacrittyPath() {
    std::cout << files.findHomeEquivilent(
        files.getdotfilesDirectory() / ".config/alacritty/themes/hoshimi.toml");
    return files.findHomeEquivilent(files.getdotfilesDirectory() /
                                    ".config/alacritty/themes/hoshimi.toml");
  }

public:
  AlacrittyWriter() {
    writer = WriterBase(getAlacrittyPath(), FileType::VALUE_PAIR);
    colors = ColorsHandler().getColors();
  }

  void reloadAlacritty() { return; }

  bool writeConfig() {
    fs::path path = getAlacrittyPath();

    // Build the file contents
    writer.empty();

    writer.append("# Alacritty color scheme generated by Hoshimi\n");
    writer.append("[colors]\n");
    writer.append("transparent_background_colors = true\n\n");

    writer.append("[colors.primary]\n");
    writer.append("background = " +
                  colors.backgroundColor.toHex(Color::FLAGS::WQUOT) + "\n");
    writer.append("foreground = " +
                  colors.foregroundColor.toHex(Color::FLAGS::WQUOT) + "\n\n");

    // Normal colors (standard order)
    static const char *normal_names[8] = {"black", "red",     "green", "yellow",
                                          "blue",  "magenta", "cyan",  "white"};
    writer.append("[colors.normal]\n");
    for (int i = 0; i < 8; ++i) {
      std::string hex = (i < (int)colors.palette.size())
                            ? colors.palette[i].toHex(Color::FLAGS::WQUOT)
                            : std::string("#000000");
      writer.append(std::string(normal_names[i]) + " = " + hex + "\n");
    }
    writer.append("\n");

    // Bright colors (palette[8..15])
    static const char *bright_names[8] = {"black", "red",     "green", "yellow",
                                          "blue",  "magenta", "cyan",  "white"};
    writer.append("[colors.bright]\n");
    for (int i = 0; i < 8; ++i) {
      int idx = 8 + i;
      std::string hex = (idx < (int)colors.palette.size())
                            ? colors.palette[idx].toHex(Color::FLAGS::WQUOT)
                            : colors.palette[i].toHex();
      writer.append(std::string(bright_names[i]) + " = " + hex + "\n");
    }
    writer.append("\n");

    // Write to file
    bool retVal = writer.write();

    if (!retVal) {
      HERR("Config " + path.string()) << "Error writing to file." << std::endl;
      writer.revert();
      return false;
    }
    reloadAlacritty();
    return retVal;
  }
};

class JsonWriter : public JsonHandlerBase {
private:
  FilesManager files;

  /**
   * Set a nested value in JSON using an array of keys
   * @param root - The root cJSON object
   * @param keys - Array of key strings defining the path
   * @param key_count - Number of keys in the array
   * @param value - The string value to set
   * @return 1 on success, 0 on failure
   */
  int set_nested_value(cJSON *root, const char **keys, int key_count,
                       const char *value) {
    if (!root || !keys || key_count <= 0) {
      return 0;
    }

    cJSON *current = root;

    // Navigate/create nested objects for all keys except the last one
    for (int i = 0; i < key_count - 1; i++) {
      cJSON *child = cJSON_GetObjectItem(current, keys[i]);

      if (!child) {
        // Create new object if it doesn't exist
        child = cJSON_CreateObject();
        if (!child) {
          return 0;
        }
        cJSON_AddItemToObject(current, keys[i], child);
      } else if (!cJSON_IsObject(child)) {
        // If existing item is not an object, we can't nest further
        printf("Error: Key '%s' exists but is not an object\n", keys[i]);
        return 0;
      }

      current = child;
    }

    // Set the final value
    const char *final_key = keys[key_count - 1];

    // Remove existing item if it exists
    cJSON_DeleteItemFromObject(current, final_key);

    // Add the new value
    cJSON *new_value = cJSON_CreateString(value);
    if (!new_value) {
      return 0;
    }

    cJSON_AddItemToObject(current, final_key, new_value);
    return 1;
  }

  /**
   * Get a nested value from JSON using an array of keys
   * @param root - The root cJSON object
   * @param keys - Array of key strings defining the path
   * @param key_count - Number of keys in the array
   * @return String value if found, NULL if not found
   */
  const char *get_nested_value(cJSON *root, const char **keys, int key_count) {
    if (!root || !keys || key_count <= 0) {
      return NULL;
    }

    cJSON *current = root;

    // Navigate through all keys
    for (int i = 0; i < key_count; i++) {
      current = cJSON_GetObjectItem(current, keys[i]);
      if (!current) {
        return NULL;
      }
    }

    // Return string value if it's a string
    if (cJSON_IsString(current)) {
      return cJSON_GetStringValue(current);
    }

    return NULL;
  }

  /**
   * Set nested value with different data types
   */
  int set_nested_value_number(cJSON *root, const char **keys, int key_count,
                              double value) {
    if (!root || !keys || key_count <= 0) {
      return 0;
    }

    cJSON *current = root;

    // Navigate/create nested objects for all keys except the last one
    for (int i = 0; i < key_count - 1; i++) {
      cJSON *child = cJSON_GetObjectItem(current, keys[i]);

      if (!child) {
        child = cJSON_CreateObject();
        if (!child)
          return 0;
        cJSON_AddItemToObject(current, keys[i], child);
      }
      current = child;
    }

    // Set the final value as number
    const char *final_key = keys[key_count - 1];
    cJSON_DeleteItemFromObject(current, final_key);

    cJSON *new_value = cJSON_CreateNumber(value);
    if (!new_value)
      return 0;

    cJSON_AddItemToObject(current, final_key, new_value);
    return 1;
  }

  char *formatJson(cJSON *root) {
    char *raw_json = cJSON_Print(root);
    if (!raw_json)
      return NULL;

    size_t raw_len = strlen(raw_json);
    char *formatted = (char *)malloc(raw_len * 2); // Extra space for formatting
    if (!formatted) {
      free(raw_json);
      return NULL;
    }

    size_t out_pos = 0;
    int indent_level = 0;
    int in_string = 0;

    for (size_t i = 0; i < raw_len; i++) {
      char c = raw_json[i];

      // Track if we're inside a string
      if (c == '"' && (i == 0 || raw_json[i - 1] != '\\')) {
        in_string = !in_string;
      }

      if (!in_string) {
        if (c == '{' || c == '[') {
          formatted[out_pos++] = c;
          formatted[out_pos++] = '\n';
          indent_level++;

          // Add indentation for next line
          for (int j = 0; j < indent_level * 2; j++) {
            formatted[out_pos++] = ' ';
          }
          continue;
        } else if (c == '}' || c == ']') {
          // Remove trailing spaces/commas before closing brace
          while (out_pos > 0 && (formatted[out_pos - 1] == ' ' ||
                                 formatted[out_pos - 1] == '\n')) {
            out_pos--;
          }
          formatted[out_pos++] = '\n';
          indent_level--;

          // Add indentation
          for (int j = 0; j < indent_level * 2; j++) {
            formatted[out_pos++] = ' ';
          }
          formatted[out_pos++] = c;
          continue;
        } else if (c == ',') {
          formatted[out_pos++] = c;
          formatted[out_pos++] = '\n';

          // Add indentation for next line
          for (int j = 0; j < indent_level * 2; j++) {
            formatted[out_pos++] = ' ';
          }
          continue;
        } else if (c == ':') {
          formatted[out_pos++] = c;
          formatted[out_pos++] = ' '; // Single space after colon
          continue;
        } else if (c == '\t' || c == '\n' || c == ' ') {
          // Skip whitespace (we control it)
          continue;
        }
      }

      formatted[out_pos++] = c;
    }

    formatted[out_pos] = '\0';
    free(raw_json);
    return formatted;
  }

public:
  JsonWriter() {}

  const char *getJson(const std::vector<std::string> &keys) {
    const bool getTheme = keys[0] == "theme";
    const char *fileToCheck =
        getTheme ? THEME_CONFIG_FILE.c_str() : MAIN_CONFIG_PATH.c_str();
    HLOG("Json") << "Getting info from " << fileToCheck << "." << std::endl;

    // Read from file
    std::ifstream input(fileToCheck, std::ios::binary);
    if (!input) {
      std::cerr << "Unable to open  the file for reading: " << fileToCheck
                << std::endl;
      return NULL;
    }

    std::string content((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
    input.close();

    cJSON *json = cJSON_Parse(content.c_str());
    if (json == NULL) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL) {
        std::cerr << "Error parsing JSON at: " << error_ptr << std::endl;
      }
      return NULL;
    }

    // Get the keys as a vector of C strings
    std::vector<const char *> cKeys;
    for (const auto &key : keys) {
      if (getTheme) {
        if (&key == &keys[0])
          continue;
      }
      cKeys.push_back(key.c_str());
    }

    // Get the value
    const char *value = get_nested_value(json, cKeys.data(), cKeys.size());
    if (value == NULL) {
      std::cerr << "Error getting the value of " << cKeys[cKeys.size() - 1]
                << " in object " << cKeys[cKeys.size() - 2] << std::endl;
      cJSON_Delete(json);
      return NULL;
    }

    return value;
  }

  bool writeJson(const std::vector<std::string> &keys, const char *value) {
    const bool editTheme = keys[0] == "theme";
    const char *fileToEdit =
        editTheme ? THEME_CONFIG_FILE.c_str() : MAIN_CONFIG_PATH.c_str();
    HLOG("Json") << "Editing " << fileToEdit << "." << std::endl;

    cJSON *json = getJsonFromFile(fileToEdit);
    if (json == nullptr) {
      return false;
    }

    // Get the keys as a vector of C strings
    std::vector<const char *> cKeys;
    for (const auto &key : keys) {
      // If editing theme, skip the first key ("theme")
      if (editTheme) {
        if (&key == &keys[0])
          continue;
      }
      cKeys.push_back(key.c_str());
    }

    // Replace the value within the json
    if (!set_nested_value(json, cKeys.data(), cKeys.size(), value)) {
      std::cerr << "Failed to set value in JSON" << std::endl;
      cJSON_Delete(json);
      return false;
    }

    // Format json
    char *json_string = formatJson(json);
    if (!json_string) {
      std::cerr << "Failed to format JSON" << std::endl;
      cJSON_Delete(json);
      return false;
    }

    // Write using streams
    std::ofstream output(fileToEdit);
    if (!output) {
      std::cerr << "Unable to open file for writing: " << fileToEdit
                << std::endl;
      cJSON_free(json_string);
      cJSON_Delete(json);
      return false;
    }

    output << json_string;
    output.close();

    cJSON_free(json_string);
    cJSON_Delete(json);
    return true;
  }
};
