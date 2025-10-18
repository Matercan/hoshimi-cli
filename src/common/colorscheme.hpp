#ifndef COLORS_H
#define COLORS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <inttypes.h>
#include <iomanip>
#include <ios>
#include <string>
#include <utility>
#include <vector>

#define BLACK Color("#0")
#define WHITE Color("#ffffff")

class Color {
public:
  int r;
  int g;
  int b;

  enum FLAGS {
    NOFLAGS = 0,
    NHASH = 1 << 0,
    WQUOT = 1 << 2,
    RGB = 1 << 3,
    SPCSEP = 1 << 4
  };

  Color() : r(0), g(0), b(0) {}
  Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
  Color(std::string hex) {
    if (hex[0] == '#') {
      hex.erase(0, 1);
    }

    while (hex.length() < 6)
      hex += '0';

    for (std::string::size_type i = 0; i < hex.length(); i += 2) {
      std::string hedec = {hex[i], hex[i + 1]};
      int val = std::stoi(hedec, nullptr, 16);
      switch (i) {
      case 0:
        r = val;
        break;
      case 2:
        g = val;
        break;
      case 4:
        b = val;
        break;
      }
    }
  }
  Color(const char *hexS) {
    std::string hex(hexS);
    if (hex[0] == '#') {
      hex.erase(0, 1);
    }

    while (hex.length() < 6)
      hex += '0';

    for (std::string::size_type i = 0; i < hex.length(); i += 2) {
      std::string hedec = {hex[i], hex[i + 1]};
      int val = std::stoi(hedec, nullptr, 16);
      switch (i) {
      case 0:
        r = val;
        break;
      case 2:
        g = val;
        break;
      case 4:
        b = val;
        break;
      }
    }
  }

  // Convert to hex string
  std::string toHex(const int &flag = FLAGS::NOFLAGS) const {
    std::stringstream ss;

    // If WQUOT, open with a quote
    if (flag & FLAGS::WQUOT)
      ss << '"';

    if (flag & FLAGS::RGB) {
      ss << r;
      if (flag & FLAGS::SPCSEP)
        ss << ", ";
      else
        ss << ",";
      ss << g;
      if (flag & FLAGS::SPCSEP)
        ss << ", ";
      else
        ss << ",";
      ss << b;

    } else { // If not NHASH, include the leading '#'
      if (!(flag & FLAGS::NHASH))
        ss << '#';

      // Ensure we print two hex digits per color component with leading zeros
      ss << std::hex << std::setfill('0') << std::nouppercase;
      ss << std::setw(2) << (static_cast<int>(r) & 0xFF);
      if (flag & FLAGS::SPCSEP)
        ss << ' ';
      ss << std::setw(2) << (static_cast<int>(g) & 0xFF);
      if (flag & FLAGS::SPCSEP)
        ss << ' ';
      ss << std::setw(2) << (static_cast<int>(b) & 0xFF);
    }

    // Close quote if requested
    if (flag & FLAGS::WQUOT)
      ss << '"';

    return ss.str();
  }

  Color mix(const Color &other, const float &percentage) const {
    uint8_t nR = this->r + (other.r - this->r) * percentage;
    uint8_t nG = this->g + (other.g - this->g) * percentage;
    uint8_t nB = this->b + (other.b - this->b) * percentage;

    return Color(std::max(std::min(255, (int)nR), 0),
                 std::max(std::min(255, (int)nG), 0),
                 std::max(std::min(255, (int)nB), 0));
  }

  Color lighten(const float &percentage) const {
    return mix(WHITE, percentage);
  }
  Color darken(const float &percentage) const { return mix(BLACK, percentage); }

  bool operator==(const Color &other) const {
    return this->toHex() == other.toHex();
  }
  bool operator!=(const Color &other) const { return !(*this == other); }
  bool operator>(const Color other) const {
    return (this->r + this->g + this->b) > (other.r + other.g + other.b);
  }
  bool operator<(const Color other) const {
    return (this->r + this->g + this->b) < (other.r + other.g + other.b);
  }

  float saturation() const {
    return (float)(this->r + this->g + this->b) / (255 * 3);
  }
  float brightness() const {
    return 0.2126 * this->r + 0.7152 * this->g + 0.0722 * this->b;
  }
  float hue() const {
    float R = (float)this->r / UINT8_MAX;
    float G = (float)this->g / UINT8_MAX;
    float B = (float)this->b / UINT8_MAX;

    uint8_t maxColor = std::max(R, std::max(G, B));
    uint8_t minColor = std::min(R, std::min(G, B));
    if (maxColor == R)
      return ((float)(G - B) / (maxColor - minColor)) * 60;
    else if (maxColor == G)
      return (2 + (float)(B - R) / (maxColor - minColor)) * 60;
    else
      return (4 + (float)(R - G) / (maxColor - minColor)) * 60;
  }
  bool light() const { return (this->r + this->g + this->b) > 384; }
};

class Colorscheme {
public:
  Color backgroundColor;
  Color foregroundColor;
  Color selectedColor;
  Color activeColor;
  Color iconColor;
  Color errorColor;
  Color passwordColor;
  Color borderColor;
  Color highlightColor;

  std::vector<Color> palette;
  std::vector<Color> main;

  Colorscheme() {}
  Colorscheme(Color mainColors[9], std::vector<Color> paletteColors) {
    palette = paletteColors;

    backgroundColor = mainColors[0];
    foregroundColor = mainColors[1];
    selectedColor = mainColors[2];
    activeColor = mainColors[3];
    iconColor = mainColors[4];
    errorColor = mainColors[5];
    passwordColor = mainColors[6];
    borderColor = mainColors[7];

    // Treat a zero-initialized Color (0,0,0) as 'not provided' for the
    // highlight, otherwise use the provided value.
    if (!(mainColors[8].r == 0 && mainColors[8].g == 0 && mainColors[8].b == 0))
      highlightColor = mainColors[8];
    else {

      // the highlight color needs to be similar to purple but still be
      // lumiscent and saturated if dark mode, or still saturated but less
      // luminescent on light mode

      if (!backgroundColor.light()) {
        highlightColor = Color(std::min(paletteColors[13].r * (int)1.2, 255),
                               paletteColors[13].g / 2,
                               std::min(paletteColors[13].b * (int)1.3, 255));
      } else {
        constexpr int targetPurple = 270;

        auto score = [&](const Color &color) -> int {
          const float CR = color > backgroundColor
                               ? (color.brightness() + 0.05) /
                                     (backgroundColor.brightness() + 0.05)
                               : (backgroundColor.brightness() + 0.05) /
                                     (color.brightness() + 0.05);

          const float CS = std::min((float)1, CR / 7);

          const float hueDiff =
              std::min(std::abs(color.hue() - targetPurple),
                       (float)360 - std::abs((color.hue(), targetPurple)));

          const float hueScore = 1 - (hueDiff / 60);

          const float vibrancyScore = std::exp(
              -((color.saturation() - 0.7) * (color.saturation() - 0.7) / 0.1));

          return 0.5 * CS + 0.3 * hueScore + 0.2 * vibrancyScore;
        };

        typedef std::pair<Color, float> scoredcolor_t;

        scoredcolor_t bestColor = {Color("#0"), 0};

        for (size_t i = 0; i < paletteColors.size(); ++i) {
          if (score(paletteColors[i]) > bestColor.second) {
            bestColor = {paletteColors[i], score(paletteColors[i])};
          } else {
            continue;
          }
        }

        highlightColor = bestColor.first;
      }
    }

    for (int i = 0; i < 8; i++)
      main.push_back(mainColors[i]);

    main.push_back(highlightColor);
  }

  Colorscheme(Color mainColors[9]) {
    backgroundColor = mainColors[0];
    foregroundColor = mainColors[1];
    selectedColor = mainColors[2];
    activeColor = mainColors[3];
    iconColor = mainColors[4];
    errorColor = mainColors[5];
    passwordColor = mainColors[6];
    borderColor = mainColors[7];
    if (!(mainColors[8].r == 0 && mainColors[8].g == 0 && mainColors[8].b == 0))
      highlightColor = mainColors[8];
    else
      highlightColor = selectedColor.lighten(0.2);

    palette[0] = Color("#0");
    palette[1] = Color("#8");
    palette[2] = Color("#008");
    palette[3] = Color("#808");
    palette[4] = Color("#00008");
    palette[5] = Color("#80008");
    palette[6] = Color("#00808");
    palette[7] = Color("#c0c0c");
    palette[8] = Color("#80808");
    palette[9] = Color("#ff");
    palette[10] = Color("#00ff");
    palette[11] = Color("#ffff");
    palette[12] = Color("#0000ff");
    palette[13] = Color("#ff00ff");
    palette[14] = Color("#00ffff");
    palette[15] = Color("#ffffff");
  }
};

#endif // COLORS_H
