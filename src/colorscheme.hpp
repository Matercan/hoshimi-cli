#include "utils/headers.hpp"

class Color {
public:
  int r;
  int g;
  int b;
  enum FLAGS { NHASH, WQUOT, NOFLAGS };

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
  std::string toHex(const FLAGS &flag = FLAGS::NOFLAGS) const {
    std::stringstream ss;

    // If WQUOT, open with a quote
    if (flag == FLAGS::WQUOT)
      ss << '"';

    // If not NHASH, include the leading '#'
    if (flag != FLAGS::NHASH)
      ss << '#';

    // Ensure we print two hex digits per color component with leading zeros
    ss << std::hex << std::setfill('0') << std::nouppercase;
    ss << std::setw(2) << (static_cast<int>(r) & 0xFF);
    ss << std::setw(2) << (static_cast<int>(g) & 0xFF);
    ss << std::setw(2) << (static_cast<int>(b) & 0xFF);

    // Close quote if requested
    if (flag == FLAGS::WQUOT)
      ss << '"';

    return ss.str();
  }

  bool operator==(const Color &other) const { return this->toHex() == other.toHex(); }
  bool operator!=(const Color &other) const { return !(*this == other); }
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

  std::vector<Color> palette;
  std::vector<Color> main;

  Colorscheme() {}
  Colorscheme(Color mainColors[8], std::vector<Color> paletteColors) {
    palette = paletteColors;

    backgroundColor = mainColors[0];
    foregroundColor = mainColors[1];
    selectedColor = mainColors[2];
    activeColor = mainColors[3];
    iconColor = mainColors[4];
    errorColor = mainColors[5];
    passwordColor = mainColors[6];
    borderColor = mainColors[7];

    main.reserve(8);
    for (int i = 0; i < 8; i++)
      main.push_back(mainColors[i]);
  }

  Colorscheme(Color mainColors[8]) {
    backgroundColor = mainColors[0];
    foregroundColor = mainColors[1];
    selectedColor = mainColors[2];
    activeColor = mainColors[3];
    iconColor = mainColors[4];
    errorColor = mainColors[5];
    passwordColor = mainColors[6];
    borderColor = mainColors[7];

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
