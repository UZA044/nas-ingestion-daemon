#include "router.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

bool router_ensure_dir(const char *path){
    int result;
    if (path == NULL) {
        log_write(LOG_ERR, "[Router] path argument passed into ensuring the dir was NULL");
        return false;
    }
    result = mkdir(path,0755);

    if (result){
        if (errno == EEXIST){
            return true;
        }
        log_write(LOG_ERR, "[Router] mkdir command failed - returning false");
        return false;
    }else {
        return true;
    }
}

bool router_move_file(const char *src, const char *dest_dir) {
    // Get filename from src 
    char *src_copy = strdup(src);
    const char *filename = basename(src_copy); 

    if (!router_ensure_dir(dest_dir)) {
        free(src_copy);
        return false;
    }

    char dest_path[PATH_MAX];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, filename);

    // if EXISTS, find _1, _2, _3...
    if (access(dest_path, F_OK) == 0) {  // EXISTS
        char *dot = strrchr(filename, '.');
        size_t base_len = dot ? (dot - filename) : strlen(filename);
        const char *ext = dot ? dot : "";

        // Formats path: dest_dir/filename_i.ext (%.*s prints filename up to base_len bytes)
        for (int i = 1; i < 1000; i++) {
            snprintf(dest_path, sizeof(dest_path), "%s/%.*s_%d%s",
                     dest_dir, (int)base_len, filename, i, ext);
            if (access(dest_path, F_OK) != 0) break; 
        }
    }

    if (rename(src, dest_path) == 0) {
        log_write(LOG_INFO, "[Router] Moved %s → %s", src, dest_path);
        free(src_copy);
        return true;
    }

    if (errno == EXDEV) {
        if (copy_file(src, dest_path) == 0) {
            unlink(src);
            log_write(LOG_INFO, "[Router] Copied %s → %s (cross-device)", src, dest_path);
            free(src_copy);
            return true;
        }
    }

    log_write(LOG_ERR, "[Router] Failed to move %s → %s: %s", src, dest_path, strerror(errno));
    free(src_copy);
    return false;
}
