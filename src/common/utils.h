#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int mkdir_recursive(const char *path) {
  char tmp[1024];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);

  if (tmp[len - 1] == '/')
    tmp[len - 1] = 0;

  for (p = tmp + 1; *p; p++) {
    if (*p == '/' || *p == '\\') {
      *p = 0;
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif
