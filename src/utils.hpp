#pragma once

#include <sys/ioctl.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <string.h>
#include <sys/ioctl.h>
#include <vector>

class Utils {
public:
  struct winsize getTerminalSize() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w;
  }

  void print_progress_bar(float progress, size_t current, size_t total) {
    int terminal_width = getTerminalSize().ws_col;
    progress = std::max(0.0f, std::min(1.0f, progress));

    // Clear the current line first
    std::cout << "\r" << std::string(terminal_width, ' ') << "\r";

    // Calculate the text that will appear after the progress bar
    std::ostringstream text_stream;
    text_stream << "] " << std::fixed << std::setprecision(1) << (progress * 100.0f) << "% (" << current << "/" << total << ")";
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

  std::vector<std::string> COLOR_NAMES = {"backgroundColor", "foregroundColor", "selectedColor", "activeColor",
                                          "iconColor",       "errorColor",      "passwordColor", "borderColor"};
};
