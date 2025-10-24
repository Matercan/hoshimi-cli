#ifndef UTILS_H_DEF
#define UTILS_H_DEF
#include "utils.h"

#include "dirent.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool dir_exists(const char *path, int *err) {
  DIR *dir = opendir(path);
  if (dir) {
    closedir(dir);
    return true;
  } else if (!err) {
    return false;
  } else if (ENOENT == errno) {
    return false;
    *err = 2;
  } else {
    return false;
    *err = -1;
  }
}

void hoshimi_error_init(hoshimi_error_t *error, int code, const char *source) {
  error->hosh_err = code;
  error->sys_err = code;
  error->source = strdup(source);

  switch (code) {
  case 0:
    error->string = strdup("No error");
    break;
  case 1:
    error->string = strdup("Hoshimi home does not exist");
    break;
  case 2:
    error->string = strdup("File not found");
    break;
  case 3:
    error->string = strdup("File not changed");
    break;
  case 4:
    error->string = strdup("Filesystem error");
    break;
  case -1:
    error->string = strdup("Unknown error");
    break;
  case -2:
    error->string = strdup("Failed to run command");
    break;
  }
}

hoshimi_error_t *init_err(const int code, const char *source) {
  hoshimi_error_t *err = (hoshimi_error_t *)malloc(sizeof(hoshimi_error_t));
  hoshimi_error_init(err, code, source);
  return err;
}

char *hoshimi_error_strerror(hoshimi_error_t *error) {
  if (error == NULL) {
    return strdup("Error: NULL error object");
  }

  char value[128];
  const char *source = error->source ? error->source : "unknown";
  const char *string = error->string ? error->string : "unknown error";

  snprintf(value, sizeof(value), "%s: %s", source, string);
  return strdup(value);
}

int free_hoshimi_error(hoshimi_error_t *error) {

  if (!error)
    return -1;

  if (error->string)
    free(error->string);
  if (error->source)
    free(error->source);

  free(error);

  return 0;
}

int mkdir_recursive(const char *path) {
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

char *getHoshimiHome(int *err) {
  int local_err = 0;                     // Local error tracking
  int *err_ptr = err ? err : &local_err; // Use provided err or local

  const char *home_env = getenv("HOME");
  if (!home_env) {
    *err_ptr = 1;
    return NULL; // Return NULL on error
  }

  const char *xdg_data_home = getenv("XDG_DATA_HOME");
  char hoshimi_home[512]; // Increased buffer size for safety

  if (xdg_data_home && dir_exists(xdg_data_home, NULL)) {
    snprintf(hoshimi_home, sizeof(hoshimi_home), "%s/hoshimi", xdg_data_home);
  } else {
    snprintf(hoshimi_home, sizeof(hoshimi_home), "%s/.local/share/hoshimi",
             home_env);
  }

  return strdup(hoshimi_home);
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag,
              struct FTW *ftwbuf) {
  int rv = remove(fpath);
  if (rv)
    perror(fpath);
  return rv;
}

int rmrf(char *path) { return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS); }

#endif

#ifdef __cplusplus

#define UTILS_HPP_DEF
#include "utils.hpp"

#endif
