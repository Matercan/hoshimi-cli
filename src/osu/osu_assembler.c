#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "circles.h"
#include "hitsounds.h"

int main(const int argc, const char **argv) {
  Config *config = load_config();

  free(config->downloadPath);
  config->downloadPath = strdup("osuGen");
  mkdir_recursive(config->downloadPath);

  generateCircles(config);
  generateSounds(config);

  free_config(config);

  rmrf("osu");
}
