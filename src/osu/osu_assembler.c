#include <unistd.h>

#include "circles.h"
#include "hitsounds.h"

int main(const int argc, const char **argv) {
  generateCircles(load_config());
  generateSounds(load_config());
}
