# Fish completion for hoshimi - Hyprland Dotfiles Manager

# Commands
complete -c hoshimi -f -n __fish_use_subcommand -a install -d "Install dotfiles by cloning repository and creating symlinks"
complete -c hoshimi -f -n __fish_use_subcommand -a help -d "Show help message"
complete -c hoshimi -f -n __fish_use_subcommand -a arch-install -d "Install all packages necessary for this shell using paru"
complete -c hoshimi -f -n __fish_use_subcommand -a version -d "Get version information of hoshimi"
complete -c hoshimi -f -n __fish_use_subcommand -a update -d "Update dotfiles to the most recent master commit"
complete -c hoshimi -f -n __fish_use_subcommand -a config -d "Get or set config options within your configuration"
complete -c hoshimi -f -n __fish_use_subcommand -a source -d "Source current configuration, updating modifiable dotfiles"
complete -c hoshimi -f -n __fish_use_subcommand -a restart -d "(re)start the shell and reload terminals"
complete -c hoshimi -f -n __fish_use_subcommand -a osu-generate -d "Generate osu items needed for the race"

# Global options (available for all commands)
complete -c hoshimi -s h -l help -d "Show help message"
complete -c hoshimi -s v -l verbose -d "Enable verbose output (show detailed operations)"
complete -c hoshimi -s f -l force -d "Force overwrite existing files without backup"
complete -c hoshimi -s p -l packages -d "Comma-separated list of packages to install or source" -r
complete -c hoshimi -s n -l not-packages -d "Comma-separated list of packages NOT to install or source" -r
complete -c hoshimi -l no-secondary-commands -d "Don't do followup commands"
complete -c hoshimi -l -maximum-followup-commands -d "Maximum number of followups a "
complete -c hoshimi -l version -d "Show version information"

# Package name completions (common Hyprland-related packages)
set -l packages hypr,quickshell,fastfetch,ghostty,fish,foot,alacritty

# Completions for -p/--packages option
complete -c hoshimi -n "__fish_seen_subcommand_from install source" -s p -l packages -d "Packages to include" -a "(string join , $packages)"

# Completions for -np/--not-packages option
complete -c hoshimi -n "__fish_seen_subcommand_from install source" -l not-packages -d "Packages to exclude" -a "(string join , $packages)"
