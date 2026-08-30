#include <stdbool.h>
#ifndef ROUTER_H
#define ROUTER_H

#define PATH_MAX 1024

bool router_ensure_dir(const char *path);

bool router_move_file(const char *src, const char *dest);

bool router_construct_dst_path(const char *filename )

#endif // ROUTER_H