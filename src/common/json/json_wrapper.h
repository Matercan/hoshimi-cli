#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char *wallpaper;
  char **commands;
  char *osuSkin;
} Config;

typedef struct {
  unsigned char r, g, b;
} ColorRGB;

typedef struct {
  ColorRGB background;
  ColorRGB foreground;
  ColorRGB active;
  ColorRGB selected;
  ColorRGB icon;
  ColorRGB error;
  ColorRGB password;
  ColorRGB border;
  ColorRGB palette[16];
} C_ColorScheme;

// C-compatible functions
Config *load_config(void);
void free_config(Config *config);

C_ColorScheme *load_colorscheme(void);
void free_colorscheme(C_ColorScheme *colors);

#ifdef __cplusplus
}
#endif

#endif // JSON_WRAPPER_H
