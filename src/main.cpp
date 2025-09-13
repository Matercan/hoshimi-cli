#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/split.hpp> // Include for boost::split

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include <string.h>
#include <sys/ioctl.h>

#include "version.h"

struct winsize getTerminalSize() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w;
}

namespace fs = std::filesystem;
std::vector<std::string> packages;

struct Flag {
  bool present;
  std::vector<std::string> args;
  std::string description;

  Flag(bool on, const std::vector<std::string> &argss, const std::string &desc = "") : present(on), args(argss), description(desc) {}

  // Alternative constructor for initializer list
  Flag(bool on, std::initializer_list<std::string> argss, const std::string &desc = "") : present(on), args(argss), description(desc) {}

  Flag() : present(false), description("") {}
};

void print_help(const std::string &program_name, const std::vector<Flag> &config) {
  std::cout << "Hoshimi - Hyprland Dotfiles Manager\n";
  std::cout << "===================================\n\n";

  std::cout << "USAGE:\n";
  std::cout << "    " << program_name << " <command> [options]\n\n";

  std::cout << "COMMANDS:\n";
  std::cout << "    install       Install dotfiles by cloning repository and "
               "creating symlinks\n";
  std::cout << "    help          Show this help message\n";
  std::cout << "    arch-install  Install all the packages neccessary for this "
               "shell using paru\n";
  std::cout << "    version       Get version information of hoshimi\n";
  std::cout << "    update        Update dotfiles to the most recent master "
               "commit\n\n";

  std::cout << "OPTIONS:\n";

  // Generate options from config vector
  for (const auto &flag : config) {
    if (!flag.args.empty()) {
      std::cout << "    ";

      // Print all aliases for this flag
      for (size_t i = 0; i < flag.args.size(); ++i) {
        std::cout << flag.args[i];
        if (i < flag.args.size() - 1) {
          std::cout << ", ";
        }
      }

      // Add padding and description
      std::cout << "    " << (flag.description.empty() ? "[No description]" : flag.description) << "\n";
    }
  }
  std::cout << "\n";

  std::cout << "EXAMPLES:\n";
  std::cout << "    " << program_name << " install           # Install dotfiles silently\n";

  // Generate examples based on available flags
  for (const auto &flag : config) {
    if (!flag.args.empty() && !flag.args[0].empty()) {
      std::cout << "    " << program_name << " install " << flag.args[0] << "        # Install with "
                << (flag.description.empty() ? "this option" : flag.description.substr(0, flag.description.find(' '))) << "\n";
    }
  }

  std::cout << "    " << program_name << " help              # Show this help\n\n";

  std::cout << "DESCRIPTION:\n";
  std::cout << "    Hoshimi manages Hyprland dotfiles by:\n";
  std::cout << "    1. Cloning the dotfiles repository to ~/.local/share/hoshimi\n";
  std::cout << "    2. Backing up existing dotfiles to ./backup/\n";
  std::cout << "    3. Creating symlinks from the repository to your home "
               "directory\n";
  std::cout << "    4. Enablng config via simiple cli tools / a single json "
               "file \n\n";
}

void print_progress_bar(float progress, size_t current, size_t total) {
  int terminal_width = getTerminalSize().ws_col;

  // Ensure progress is between 0 and 1
  progress = std::max(0.0f, std::min(1.0f, progress));

  // Calculate the text that will appear after the progress bar
  std::ostringstream text_stream;
  text_stream << "] " << std::fixed << std::setprecision(1) << (progress * 100.0f) << "% (" << current << "/" << total << ")";
  std::string suffix_text = text_stream.str();

  // Calculate space needed for: "[" + suffix_text
  int non_bar_width = 1 + suffix_text.length(); // 1 for the opening "["

  // Calculate available width for the actual bar
  int bar_width = terminal_width - non_bar_width;

  // Ensure minimum bar width
  if (bar_width < 10)
    bar_width = 10; // Minimum readable bar width

  int filled_width = static_cast<int>(progress * bar_width);

  std::cout << "\r["; // \r returns cursor to beginning of line

  // Print filled portion
  for (int i = 0; i < filled_width; ++i)
    std::cout << "█";

  // Print empty portion
  for (int i = filled_width; i < bar_width; ++i)
    std::cout << "░";

  // Print the suffix text we calculated earlier
  std::cout << suffix_text;

  std::cout.flush(); // Ensure immediate output

  // Add newline when complete
  if (progress >= 1.0f) {
    std::cout << std::endl;
  }
}

int install_dotfiles(const std::string HOME, const std::string HOSHIMI_HOME, std::vector<Flag> config) {
  // move the current dotfiles into a backup folder
  const fs::path DOTFILES_DIRECTORY = HOSHIMI_HOME + "/dotfiles";
  const fs::path BACKUP_DIRECTORY = fs::current_path() / "backup/";

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

      // Use standard string find instead of boost
      std::string const config_relative_path = fs::relative(dir_entry.path(), DOTFILES_DIRECTORY.string() + ".config");
      bool file_in_packages = false;

      for (int i = 0; i < packages.size(); ++i) {

        if (config_relative_path.find(packages[i]) != std::string::npos) {
          file_in_packages = true;
          break;
        }
      }

      bool file_installed = (config[3].present && file_in_packages) || !config[3].present;

      if (!file_installed)
        continue;

      if (!fs::is_directory(dir_entry)) {
        total_files++;
      }
    }

    if (config[0].present) {
      std::cout << "Found " << total_files << " files to process.\n" << std::endl;
    }

    size_t processed = 0;
    bool progress_bar_active = false;

    for (auto const &dir_entry : fs::recursive_directory_iterator(DOTFILES_DIRECTORY)) {

      // Clear progress bar before verbose output
      if (progress_bar_active && config[0].present) {
        std::cout << "\r" << std::string(getTerminalSize().ws_col, ' ') << "\r";
        progress_bar_active = false;
      }

      if (config[0].present) {
        std::cout << "Processing: " << dir_entry << std::endl;
      }

      fs::path const relative_path = fs::relative(dir_entry.path(), DOTFILES_DIRECTORY);
      fs::path const home_equivalent = HOME / relative_path;
      fs::path const backup_path = BACKUP_DIRECTORY / relative_path;
      std::string const config_relative_path = fs::relative(dir_entry.path(), DOTFILES_DIRECTORY.string() + ".config");

      bool file_in_packages = false;

      for (int i = 0; i < packages.size(); ++i) {

        // Use standard string find instead of boost
        if (config_relative_path.find(packages[i]) != std::string::npos) {
          file_in_packages = true;
          break;
        }
      }

      bool file_installed = (config[3].present && file_in_packages) || !config[3].present;

      if (!file_installed)
        continue;

      // First, handle backup of existing files/directories
      if (fs::exists(home_equivalent)) {
        // Create parent directories in backup if needed
        fs::create_directories(backup_path.parent_path());

        if (fs::is_directory(home_equivalent)) {
          if (config[0].present) {
            std::cout << "Backing up directory: " << home_equivalent << " to " << backup_path << std::endl;
          }
          // For directories, only backup if backup doesn't exist
          if (!fs::exists(backup_path)) {
            fs::create_directory(backup_path);
          }
        } else {
          if (config[0].present) {
            std::cout << "Backing up file: " << home_equivalent << " to " << backup_path << std::endl;
          }
          fs::rename(home_equivalent, backup_path);
        }
      }

      // Now create symlinks/directories
      if (fs::is_directory(dir_entry)) {
        if (!fs::exists(home_equivalent)) {
          if (config[0].present) {
            std::cout << "Creating directory: " << home_equivalent << std::endl;
          }
          fs::create_directories(home_equivalent);
        }
      } else {
        // For files, create symlink (file should be backed up by now)
        if (config[0].present) {
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

        if (config[0].present) {
          // Verbose mode: show simple progress
          std::cout << "Progress: " << processed << "/" << total_files << " (" << int((float)processed / total_files * 100.0) << "%)" << std::endl;
        } else {
          // Non-verbose mode: show progress bar
          float progress = (float)processed / total_files;
          print_progress_bar(progress, processed, total_files);
          progress_bar_active = true;
        }
      }
    }

    // Clear progress bar at the end
    if (progress_bar_active) {
      std::cout << "\r" << std::string(getTerminalSize().ws_col, ' ') << "\r";
    }

    return 0;
  } else {
    std::cout << "Dotfiles directory not found: " << DOTFILES_DIRECTORY << std::endl;
    return 1;
  }
}

int main(int argc, char *argv[]) {
  std::vector<Flag> config = {Flag(false, {"-v", "--verbose"}, "Enable verbose output (show detailed operations)"),
                              Flag(false, {"-f", "--force"}, "Force overwrite existing files without backup"),
                              Flag(false, {"-h", "--help"}, "Show this help message"),
                              Flag(false, {"-p", "--packages"},
                                   "Packages only install the packages "
                                   "for "
                                   "example \n\t hypr,fastfetch,starship.toml,../.zshrc"
                                   "\n\t hypr,nvim,btop")};
  // Check if we have enough arguments
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <command>" << std::endl;
    return 1;
  }

  for (int i = 2; i < argc; i++) {
    if (argc == 2)
      break;
    int packagesArgument = 1;

    for (int j = 0; j < config.size(); j++) {
      if (count(config[j].args.begin(), config[j].args.end(), argv[i])) {
        config[j].present = !config[j].present;
        if (j == 3)
          packagesArgument = ++i;
      }
    }

    if (i == packagesArgument) {
      boost::split(packages, argv[i], boost::is_any_of(","), boost::token_compress_on);
      std::cout << packages[0];
    }
  }

  std::string command = argv[1];

  if (command == "help" || config[2].present) {
    print_help(argv[0], config);
  } else if (command == "install") {
    // git clone the repo into ~/.local/share/hoshimi if not already there

    std::string HOSHIMI_HOME;

    // Safely get HOME environment variable
    const char *home_env = getenv("HOME");
    if (!home_env) {
      std::cerr << "HOME environment variable not set" << std::endl;
      return 1;
    }
    const std::string HOME = home_env;

    // Check XDG_DATA_HOME
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
        return 1;
      }
    }

    if (install_dotfiles(HOME, HOSHIMI_HOME, config) != 0)
      return 1;

    // TODO: rewrite the dotfiles in order to satisfy the config in
    // ~/.config/hoshimi

    std::cout << std::endl << "Hoshimi Dotfiles installed ";
    if (!config[1].present)
      std::cout << "and files backed up";
    std::cout << "." << std::endl;
  } else if (command == "update") {

    std::string HOSHIMI_HOME;

    // Safely get HOME environment variable
    const char *home_env = getenv("HOME");
    if (!home_env) {
      std::cerr << "HOME environment variable not set" << std::endl;
      return 1;
    }
    const std::string HOME = home_env;

    // Check XDG_DATA_HOME
    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && fs::exists(xdg_data_home)) {
      HOSHIMI_HOME = std::string(xdg_data_home) + "/hoshimi";
    } else {
      HOSHIMI_HOME = HOME + "/.local/share/hoshimi";
    }

    const std::string UPDATE_COMMAND = "cd " + HOSHIMI_HOME + " && git pull";
    if (system(UPDATE_COMMAND.c_str()) == 0) {
      return 0;
    } else {
      std::cout << "Hoshimi failed to update. Check if the directory " << HOSHIMI_HOME << "exists or not";
      return 2;
    }
  } else if (command == "arch-install") {
    // Loop through all of the lnes in the lines of the file arch-packages

    std::string packagesToInstall = "";

    std::string HOSHIMI_HOME;

    // Safely get HOME environment variable
    const char *home_env = getenv("HOME");
    if (!home_env) {
      std::cerr << "HOME environment variable not set" << std::endl;
      return 1;
    }
    const std::string HOME = home_env;

    // Check XDG_DATA_HOME
    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && fs::exists(xdg_data_home)) {
      HOSHIMI_HOME = std::string(xdg_data_home) + "/hoshimi";
    } else {
      HOSHIMI_HOME = HOME + "/.local/share/hoshimi";
    }

    std::ifstream f(HOSHIMI_HOME + "/archpackages.txt");

    if (!f.is_open()) {
      std::cerr << "Error opening the file!";
      return 3;
    }

    // Add them to a single linie string

    std::string s;
    while (getline(f, s)) {
      packagesToInstall += s + " ";
      if (config[0].present)
        std::cout << "Going to install: " << s << std::endl;
    }

    f.close();

    // Install the packages the packages

    system(("paru -S " + packagesToInstall).c_str());

  } else if (command == "version") {
    std::cout << "hoshimi v" << HOSHIMI_VERSION << std::endl;
    if (config[0].present) {
      std::cout << "Released on " << HOSHIMI_RELEASE_DATE << std::endl;
    }
  }

  return 0;
}
