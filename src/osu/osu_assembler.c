#include <stdlib.h>
#include <unistd.h>

#include "circles.h"
#include "hitsounds.h"

int main(const int argc, const char **argv) {
  Config *config = load_config();
  config->downloadPath = (char *)realloc(config->downloadPath, sizeof("osuGen/"));
  config->downloadPath = "osuGen/";
  mkdir_recursive(config->downloadPath);

  generateCircles(config);
  generateSounds(config);

  rmrf("osu");
}
