#include "files.hpp"
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
  std::cout << "    config        Get or set the config options within your "
               "configuration\n";
  std::cout << "    source        Source the current configuration, updating "
               "the modifiable dotfiles \n";
  std::cout << "    restart       (re)start the shell and reload terminals. \n\n";

  std::cout << "OPTIONS:\n";

  std::cout << "    -h, --help                              Show this help message\n";
  std::cout << "    -v, --verbose                           Enable verbose "
               "output (show detailed operations)\n";
  std::cout << "    -f, --force                             Force overwrite "
               "existing files without backup\n";
  std::cout << "    -p, --packages <pkg1,pkg2,...>          Comma-separated "
               "list of packages to install or source\n";
  std::cout << "    -np, --not-packages <pkg1,pkg2,...>     Comma-separated "
               "list of packages NOT to install or source\n";
  std::cout << "    --no-secondary-commands                 Don't do followup "
               "commands\n";
  std::cout << "    --version                               Show version "
               "information\n\n";

  std::cout << "\nEXAMPLES:\n";
  std::cout << "    " + program_name + " install -p hypr,fastfetch -v\n";
  std::cout << "    " + program_name + " source -p quickshell\n";
  std::cout << "    " + program_name + " arch-install\n";
  std::cout << "    " + program_name + " install -np hypr --no-secondary-commands\n\n";

  std::cout << "Subcommands have their own help" << std::endl;
}

void restart(std::vector<Flag> &config, char *argv[]);

int archInstall(std::vector<Flag> &config, bool &retFlag);

void getPackageInfo(int argc, std::vector<Flag> &config, char *argv[]);

void getConfigArg(int argc, char *argv[], bool &setB, std::string &configArg, std::vector<std::string> &vec);

void sourceConfig(std::vector<Flag> config);

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

  getPackageInfo(argc, config, argv);

  std::string command = argv[1];

  if (command == "install") {
    FilesManager filesManager;

    if (config[2].present) {
      std::cout << argv[0] << " install installs hoshimi dotfiles.";
      std::cout << "Dotfiles modifiable by Hoshimi config will be copied "
                   "instead of symlinked."
                << std::endl;
      std::cout << "The dotfiles' source is located in " << filesManager.getdotfilesDirectory() << std::endl;
      std::cout << "Unless running in force mode, the existing files will be "
                   "backed up to "
                << filesManager.getdotfilesDirectory() / ".backup/" << " before being replaced." << std::endl;
      std::cout << "Use '" << argv[0] << " help' to see all available commands and options." << std::endl;
      return 0;
    }

    if (filesManager.install_dotfiles(packages, notPackages, config[0].present, config[3].present) != 0)
      return 1;

    if (!config[5].present)
      sourceConfig(config);

    std::cout << std::endl << "Hoshimi Dotfiles installed ";
    if (!config[1].present)
      std::cout << "and files backed up";
    std::cout << "." << std::endl;
  } else if (command == "update") {
    FilesManager filesManager;

    if (config[2].present) {
      std::cout << argv[0]
                << " update updates hoshimi dotfiles to the most recent commit "
                   "on master branch."
                << std::endl;
      std::cout << "The dotfiles' source is located in " << filesManager.getdotfilesDirectory() << std::endl;
      std::cout << "Use '" << argv[0] << " help' to see all available commands and options." << std::endl;
      return 0;
    }

    std::filesystem::path HOSHIMI_HOME = filesManager.getdotfilesDirectory().parent_path();

    const std::string UPDATE_COMMAND = "cd " + HOSHIMI_HOME.string() + " && git pull";
    if (system(UPDATE_COMMAND.c_str()) == 0) {
      return 0;
    } else {
      std::cout << "Hoshimi failed to update. Check if the directory " << HOSHIMI_HOME.string() << " exists or not";
      return 2;
    }
  } else if (command == "source") {
    if (config[2].present) {
      std::cout << argv[0]
                << " source sources the current configuration, updating the "
                   "modifiable dotfiles."
                << std::endl;
      std::cout << "You can specify which packages to source with the "
                   "-p/--packages and -np/--not-packages flags."
                << std::endl;
      std::cout << "Use '" << argv[0] << " help' to see all available commands and options." << std::endl;
      return 0;
    }
    sourceConfig(config);

  } else if (command == "config") {
    if (config[2].present) {
      std::cout << argv[0]
                << " config gets or sets the config options within your "
                   "configuration."
                << std::endl;
      std::cout << "Usage: " << argv[0] << " config <key1> <key2> ... get/set <value>" << std::endl;
      std::cout << "For example, to set the wallpaper directory: " << argv[0] << " config globals wallpaperDirectory set ~/Pictures/Wallpapers"
                << std::endl;
      std::cout << "To get the current wallpaper directory: " << argv[0] << " config globals wallpaperDirectory get" << std::endl;
      return 0;
    }

    std::vector<std::string> vec;
    std::string configArg;
    bool setB;

    getConfigArg(argc, argv, setB, configArg, vec);

    if (vec.empty() || configArg.empty()) {
      std::cerr << "Error: Invalid config command format" << std::endl;
      std::cout << "Usage: " << argv[0] << " config <key1> <key2> ... get/set <value>" << std::endl;
      return 1;
    }

    JsonWriter js;
    if (setB) {
      if (!js.writeJson(vec, configArg.c_str())) {
        std::cerr << "Unable to handle request" << std::endl;
        std::cout << "Write config options in a list and then set with the "
                     "value you want to set it to"
                  << std::endl;
        std::cout << "For example: " << argv[0] << " config globals wallpaperDirectory set ~/Pictures/Wallpapers" << std::endl;
        return 1;
      } else {
        if (!config[5].present)
          return sourceConfig(config), 0;
      }
    } else
      std::cout << js.getJson(vec) << std::endl;
    return 0;

  } else if (command == "arch-install") {
    if (config[2].present) {
      std::cout << argv[0]
                << " arch-install installs all the packages neccessary for "
                   "this shell using paru."
                << std::endl;
      std::cout << "Use '" << argv[0] << " help' to see all available commands and options." << std::endl;
      return 0;
    }

    bool retFlag;
    int retVal = archInstall(config, retFlag);
    if (retFlag)
      return retVal;

  } else if (command == "restart") {
    if (config[2].present) {
      std::cout << argv[0] << " restart (re)starts the shell and reloads terminals." << std::endl;
      std::cout << "Use '" << argv[0] << " help' to see all available commands and options." << std::endl;
      return 0;
    }

    restart(config, argv);

  } else if (command == "version") {
    std::cout << "hoshimi v" << HOSHIMI_VERSION << std::endl;
    if (config[0].present) {
      std::cout << "Released on " << HOSHIMI_RELEASE_DATE << std::endl;
    }
  } else if (command == "help" || config[2].present) {
    print_help(argv[0], config);
  } else {
    HERR("main") << "Unknown command: " << command << "Use '" << argv[0] << " help' to see available commands." << std::endl;
    return 1;
  }
}

void sourceConfig(std::vector<Flag> config) {
  if (config[3].present) {
    for (size_t i = 0; i < packages.size(); ++i) {
      if (packages[i] == "ghostty") {
        GhosttyWriter gs;
        gs.writeConfig();
      } else if (packages[i] == "quickshell") {
        QuickshellWriter qs;
        qs.writeColors();
        qs.writeShell();
      } else if (packages[i] == "alacritty") {
        AlacrittyWriter as;
        as.writeConfig();
      } else if (packages[i] == "foot") {
        FootWriter fs;
        fs.writeConfig();
      }
    }
  } else {
    GhosttyWriter *gs = new GhosttyWriter();
    gs->writeConfig();
    delete gs;
    AlacrittyWriter *as = new AlacrittyWriter();
    as->writeConfig();
    delete as;
    FootWriter *fs = new FootWriter();
    fs->writeConfig();
    delete fs;

    QuickshellWriter *qs = new QuickshellWriter();
    qs->writeColors();
    qs->writeShell();
    delete qs;
  }

  if (config[5].present)
    return;
  auto shellConfig = ShellHandler().getConfig();
  for (int i = 0; shellConfig.commands && shellConfig.commands[i]; ++i) {
    if (system(shellConfig.commands[i]) != 0) {
      HERR("main") << "Failed to run command: " << shellConfig.commands[i] << std::endl;
    }
  }
}

void getConfigArg(int argc, char *argv[], bool &setB, std::string &configArg, std::vector<std::string> &vec) {
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
}

void getPackageInfo(int argc, std::vector<Flag> &config, char *argv[]) {
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
}

int archInstall(std::vector<Flag> &config, bool &retFlag) {
  retFlag = true;
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
  retFlag = false;
  return {};
}

void restart(std::vector<Flag> &config, char *argv[]) {
  if (!config[5].present)
    sourceConfig(config);

  GhosttyWriter *gs = new GhosttyWriter();
  gs->reloadGhostty();
  delete gs;

  system("killall qs; \n "
         " qs > /dev/null 2>&1 &");
  HLOG("main") << "Hoshimi restarted" << std::endl;
}
