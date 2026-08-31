#include "router.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>

#define DISK_THRESHOLD_BYTES (1024L * 1024L * 1024L) // 1 GB safety limit
#include <sys/stat.h>

bool router_ensure_dir(const char *path){
    if (path == NULL) {
        log_write(LOG_ERR, "[Router] path argument passed into ensuring the dir was NULL");
        return false;
    }

    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len == 0) return false;

    // Remove trailing slash
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    // Iterate through the path and create directories one by one
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                log_write(LOG_ERR, "[Router] mkdir failed for %s: %s", tmp, strerror(errno));
                return false;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        log_write(LOG_ERR, "[Router] final mkdir failed for %s: %s", tmp, strerror(errno));
        return false;
    }

    return true;
}

static int copy_file(const char *src_path, const char *dest_path) {
    if (src_path == NULL || dest_path == NULL) {
        log_write(LOG_ERR, "[Router] copy_file: NULL path argument");
        return -1;
    }

    int src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        log_write(LOG_ERR, "[Router] copy_file: open src '%s' failed: %s", src_path, strerror(errno));
        return -1;
    }

    int dst_fd = open(dest_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dst_fd < 0) {
        log_write(LOG_ERR, "[Router] copy_file: open dest '%s' failed: %s", dest_path, strerror(errno));
        close(src_fd);
        return -1;
    }

    char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        char *buf_ptr = buffer;
        ssize_t bytes_left = bytes_read;

        while (bytes_left > 0) {
            ssize_t bytes_written = write(dst_fd, buf_ptr, bytes_left);
            if (bytes_written < 0) {
                if (errno == EINTR) continue;  // retry on signal
                log_write(LOG_ERR, "[Router] copy_file: write failed: %s", strerror(errno));
                close(src_fd);
                close(dst_fd);
                return -1;
            }
            buf_ptr += bytes_written;
            bytes_left -= bytes_written;
        }
    }

    if (bytes_read < 0) {
        log_write(LOG_ERR, "[Router] copy_file: read failed: %s", strerror(errno));
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    if (fsync(dst_fd) < 0) {
        log_write(LOG_WARNING, "[Router] copy_file: fsync failed: %s", strerror(errno));
    }

    close(src_fd);
    close(dst_fd);
    return 0;
}


bool router_move_file(const char *src, const char *dest_dir) {
    if (src == NULL || dest_dir == NULL) {
        log_write(LOG_ERR, "[Router] router_move_file: NULL path argument");
        return false;
    }

    // --- Disk Space Guard ---
    struct statvfs vfs;
    if (statvfs(dest_dir, &vfs) == 0) {
        unsigned long long free_space = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
        if (free_space < DISK_THRESHOLD_BYTES) {
            log_write(LOG_ERR, "[Router] CRITICAL: Disk space too low (%llu bytes left). Aborting move.", free_space);
            return false;
        }
    } else {
        log_write(LOG_WARNING, "[Router] Could not check disk space: %s", strerror(errno));
        // We continue anyway, but log the warning
    }

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
        if (chmod(dest_path, 0664) != 0) {
            log_write(LOG_ERR, "Failed to set permissions on %s: %s", dest_path, strerror(errno));
        }
        free(src_copy);
        return true;
    }

    if (errno == EXDEV) {
        if (copy_file(src, dest_path) == 0) {
            unlink(src);
            log_write(LOG_INFO, "[Router] Copied %s → %s (cross-device)", src, dest_path);
            if (chmod(dest_path, 0664) != 0) {
                log_write(LOG_ERR, "Failed to set permissions on %s: %s", dest_path, strerror(errno));
            }
            free(src_copy);
            return true;
        }
    }

    log_write(LOG_ERR, "[Router] Failed to move %s → %s: %s", src, dest_path, strerror(errno));
    free(src_copy);
    return false;
}
