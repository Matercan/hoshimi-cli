#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif // CPP

#include "dirent.h"
#include "errno.h"
#include "stdbool.h"

typedef struct hoshimi_error {
  int hosh_err;
  int sys_err;
  char *string;
  char *source;
} hoshimi_error_t;

bool dir_exists(const char *path, int *err);
void hoshimi_error_init(hoshimi_error_t *error, int code, const char *source);
hoshimi_error_t *init_err(const int code, const char *source);
char *hoshimi_error_strerror(hoshimi_error_t *error);
int free_hoshimi_error(hoshimi_error_t *error);
int mkdir_recursive(const char *path);
char *getHoshimiHome(int *err);

#ifdef __cplusplus
}
#endif // CPP

#endif // UTILS_H
