#ifndef ROUTER_H
#define ROUTER_H
#include <stdbool.h>

bool router_ensure_dir(const char *path);
bool router_move_file(const char *src, const char *dest_dir);

#endif // ROUTER_H
