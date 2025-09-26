
#include "files.hpp"
#include "utils/headers.hpp"
#include "version.h"

namespace fs = std::filesystem;
std::vector<std::string> packages;
std::vector<std::string> notPackages;

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
  std::cout << "    update        Update dotfiles to the most recent master commit\n";
  std::cout << "    config        Get or set the config options within your configuration\n";
  std::cout << "    source        Source the current configuration, updating the modifiable dotfiles \n\n";

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
  std::cout << "\nEXAMPLES:\n";
  std::cout << "    " + program_name + " install -p hypr,fastfetch\n";
  std::cout << "    " + program_name + " source -p quickshell\n";
  std::cout << "    " + program_name + " arch-install\n";
  std::cout << "    " + program_name + " -np hypr --no-secondary-commands\n";
}

int main(int argc, char *argv[]) {
  std::vector<Flag> config = {Flag(false, {"-v", "--verbose"}, "Enable verbose output (show detailed operations)"),
                              Flag(false, {"-f", "--force"}, "Force overwrite existing files without backup"),
                              Flag(false, {"-h", "--help"}, "Show this help message"),
                              Flag(false, {"-p", "--packages"}, "Packages that you want to install"),
                              Flag(false, {"-np", "--not-packages"}, "Not-Packages that you want to install"),
                              Flag(false, {"--no-secondary-commands"}, "Don't show the secondnary commands")};

  // Check if we have enough arguments
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <command>" << std::endl;
    return 1;
  }

  for (int i = 2; i < argc; ++i) {
    if (argc == 2)
      break;
    int packagesArgument, noPackagesArgument;

    for (size_t j = 0; j < config.size(); ++j) {
      if (count(config[j].args.begin(), config[j].args.end(), argv[i])) {
        config[j].present = !config[j].present;
        if (j == 3)
          packagesArgument = ++i;
        else if (j == 4)
          noPackagesArgument = ++i;
      }
    }

    if (i == packagesArgument) {
      boost::split(packages, argv[i], boost::is_any_of(","), boost::token_compress_on);
    } else if (i == noPackagesArgument) {
      boost::split(notPackages, argv[i], boost::is_any_of(","), boost::token_compress_on);
    }
  }

  std::string command = argv[1];

  if (command == "help" || config[2].present) {
    print_help(argv[0], config);
  } else if (command == "install") {
    FilesManager filesManager;

    if (filesManager.install_dotfiles(packages, notPackages, config[0].present, config[3].present) != 0)
      return 1;

    std::cout << config[5].present << std::endl;

    // HACK: converting to a c string and then to a standard string.
    if (!config[5].present)
      system(((std::string)argv[0] + " source").c_str());

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
  } else if (command == "source") {
    if (config[3].present) {
      for (size_t i = 0; i < packages.size(); ++i) {
        if (packages[i] == "ghostty") {
          GhosttyWriter gs;
          gs.writeConfig();
        } else if (packages[i] == "quickshell") {
          QuickshellWriter qs;
          qs.writeColors();
        }
      }
    } else {
      GhosttyWriter *gs = new GhosttyWriter();
      gs->writeConfig();
      delete gs;

      QuickshellWriter *qs = new QuickshellWriter();
      qs->writeColors();
      qs->writeShell();
      delete qs;
    }

  } else if (command == "config") {
    std::vector<std::string> vec;
    std::string configArg;
    bool setB;

    // Process arguments until we find "set"
    for (int i = 2; i < argc; ++i) {
      if (strcmp(argv[i], "set") == 0) {
        setB = true;
        if (i + 1 < argc) {
          configArg = argv[i + 1];
          break;
        } else {
          std::cerr << "No value after set" << std::endl;
        }
      } else if (strcmp(argv[i], "get") == 0) {
        setB = false;
        configArg = argv[i];
      } else {
        vec.push_back(argv[i]);
      }
    }

    if (vec.empty() || configArg.empty()) {
      std::cerr << "Error: Invalid config command format" << std::endl;
      std::cout << "Usage: " << argv[0] << " config <key1> <key2> ... get/set <value>" << std::endl;
      return 1;
    }

    JsonWriter js;
    if (setB) {
      if (!js.writeJson(vec, configArg.c_str())) {
        std::cerr << "Unable to handle request" << std::endl;
        std::cout << "Write config options in a list and then set with the value you want to set it to" << std::endl;
        std::cout << "For example: " << argv[0] << " config globals wallpaperDirectory set ~/Pictures/Wallpapers" << std::endl;
        return 1;
      } else {
        if (!config[5].present)
          return system((std::string(argv[0]) + " source").c_str());
      }
    } else
      std::cout << js.getJson(vec) << std::endl;
    return 0;

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
