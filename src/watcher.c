#include "logger.h"
#include "watcher.h"
#include <sys/inotify.h>
#include <signal.h>
#include <stdatomic.h>
#include <errno.h>
#include <unistd.h>

static char *default_watch_path = "/nas/incoming";
static int g_file_descriptor = -1;
static int g_watch_descriptor = -1;

static volatile sig_atomic_t g_running = 1;

void watcher_signal_handler(int sig){
    (void)sig;
    g_running = 0;
    log_write(LOG_INFO, " A signal was detected to terminate the program - about to start this. ");

}

bool watcher_init(const char *path){
    if (path == NULL) {
        log_write(LOG_ERR, "path argument passed into initialise watcher was NULL");
        return false;
    }

    g_file_descriptor = inotify_init();

    if (g_file_descriptor == -1){
        log_write(LOG_ERR, "Inotify failed to initialise.");
        return false;
    }

    g_watch_descriptor = inotify_add_watch(g_file_descriptor, path, IN_CLOSE_WRITE | IN_CREATE);

    if (g_watch_descriptor == -1){
        log_write(LOG_ERR, "A new watch was not added.");
        return false;
    }
    return true;
}

void watcher_start(void){
    char buf[BUF_LEN];

    while (g_running){
        int bytes_read = read(g_file_descriptor, buf, BUF_LEN);

        if (bytes_read == -1) {
            if (errno == EINTR) {
                if (!g_running) {
                    break;
                }
                continue;
            }
            log_write(LOG_ERR, "read() failed with errno %d", errno);
            break;
        }

        char *p = buf;

        while(p < buf + bytes_read){
            struct inotify_event *event = (struct inotify_event *)p;

            if ((event->mask & IN_CLOSE_WRITE) && (event->len > 0)){
                log_write(LOG_INFO, "New file: %s", event->name);
            }

            if ((event->mask & IN_CREATE) && (event->mask & IN_ISDIR) && (event->len > 0)){
                log_write(LOG_INFO, "New directory: %s", event->name);
            }

            p += sizeof(struct inotify_event) + event->len;
        }
    }
}

void watcher_stop(void){
    if (g_file_descriptor != -1) {
        close(g_file_descriptor);
        g_file_descriptor = -1;
        g_watch_descriptor = -1;
    }
}
