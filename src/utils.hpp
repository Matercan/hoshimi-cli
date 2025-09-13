#pragma once

#include <sys/ioctl.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <string.h>
#include <sys/ioctl.h>

class Utils {
public:
  struct winsize getTerminalSize() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w;
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
};
