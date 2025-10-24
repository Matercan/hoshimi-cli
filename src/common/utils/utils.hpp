#pragma once

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

// Logging helpers: use like `HLOG("Install") << "Found " << n << " files" <<
// std::endl;`
#define HLOG(tag)                                                              \
  std::cout << "\033[1;39m[LOG]\033[0m " << "\033[2m[" << tag << "]\033[0m "
#define HERR(tag)                                                              \
  std::cerr << "\033[1;31m[Error]\033[0m " << "\033[2m[" << tag << "]\033[0m "

#ifdef DEBUG
#include <iostream>
#define HDBG(tag)                                                              \
  std::cout << "\033[1;36m[DBG]\033[0m " << "\033[2m[" << tag << "]\033[0m "
#else
// No-op version for release builds
class DumbNull {
public:
  template <typename T> DumbNull &operator<<(const T &) { return *this; }
  DumbNull &operator<<(std::ostream &(*)(std::ostream &)) { return *this; }
};
#define HDBG(tag) DumbNull()

#endif // DEBUG

class Utils {
public:
  static struct winsize getTerminalSize() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w;
  }

  static void print_progress_bar(float progress, size_t current, size_t total) {
    int terminal_width = getTerminalSize().ws_col;
    progress = std::max(0.0f, std::min(1.0f, progress));

    // Clear the current line first
    std::cout << "\r" << std::string(terminal_width, ' ') << "\r";

    // Calculate the text that will appear after the progress bar
    std::ostringstream text_stream;
    text_stream << "] " << std::fixed << std::setprecision(1)
                << (progress * 100.0f) << "% (" << current << "/" << total
                << ")";
    std::string suffix_text = text_stream.str();

    // Calculate space needed for: "[" + suffix_text
    int non_bar_width = 1 + suffix_text.length(); // 1 for the opening "["

    // Calculate available width for the actual bar
    int bar_width = std::max(10, terminal_width - non_bar_width);

    int filled_width = static_cast<int>(progress * bar_width);

    std::cout << "[";

    // Print filled portion
    for (int i = 0; i < filled_width; ++i)
      std::cout << "█";

    // Print empty portion
    for (int i = filled_width; i < bar_width; ++i)
      std::cout << "░";

    std::cout << suffix_text;

    // Add newline when complete
    if (progress >= 1.0f) {
      std::cout << std::endl;
    }

    std::cout.flush();
    fflush(stdout); // Force flush the output
  }

  static bool endsWith(const std::string &fullString,
                       const std::string &ending) {
    // Check if the ending string is longer than the full
    // string
    if (ending.size() > fullString.size())
      return false;

    // Compare the ending of the full string with the target
    // ending
    return fullString.compare(fullString.size() - ending.size(), ending.size(),
                              ending) == 0;
  }

  static void destroyOsuDir(int *err) {
    auto osu_path = fs::current_path() / "osu/";

    std::error_code ec;
    fs::remove_all(osu_path, ec);

    if (ec) {
      HERR("Files") << ec.message() << std::endl;
      if (err)
        *err = ec.value();
    }
  }

  std::vector<std::string> COLOR_NAMES = {
      "backgroundColor", "foregroundColor", "selectedColor", "activeColor",
      "iconColor",       "errorColor",      "passwordColor", "borderColor"};
};
