#ifndef NAS_WATCHER_H
#define NAS_WATCHER_H

#include <stdbool.h>
#include <sys/inotify.h>
#include <limits.h>
#include "config.h"

#define BUF_LEN (1024 * (sizeof(struct inotify_event) + NAME_MAX + 1))

bool watcher_init(const char *path, const Config *config);
void watcher_start(void);
void watcher_stop(void);
void watcher_signal_handler(int sig);

#endif // NAS_WATCHER_H
