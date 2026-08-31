/*
 * test_watcher.c — Tests inotify watcher integration
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_watcher
 *
 * Run:
 *   ./test_watcher
 */

#include "logger.h"
#include "watcher.h"
#include "config.h"
#include "pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <syslog.h>


#define TEST_WATCH_DIR "/tmp/test_watch_in"
#define TEST_DEST_DIR   "/tmp/test_watch_out"

/* Helper to create a mock config */
static Config* create_mock_config() {
    Config *cfg = calloc(1, sizeof(Config));
    if (!cfg) return NULL;

    cfg->paths.photos_dir = TEST_DEST_DIR;
    cfg->paths.docs_dir = TEST_DEST_DIR;
    cfg->paths.quarantine_dir = TEST_DEST_DIR;

    return cfg;
}

static bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static void cleanup() {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s %s", TEST_WATCH_DIR, TEST_DEST_DIR);
    system(cmd);
}

int main(void) {
    log_init("test-watcher", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    cleanup();
    mkdir(TEST_WATCH_DIR, 0755);
    mkdir(TEST_DEST_DIR, 0755);

    Config *mock_cfg = create_mock_config();
    if (!mock_cfg) {
        printf("Failed to create mock config\n");
        return 1;
    }

    /* Test 1: Basic Init and Event Routing */
    printf("Test 1: Integration (File creation -> Routing)... ");

    if (!watcher_init(TEST_WATCH_DIR, mock_cfg)) {
        printf("FAIL (init failed)\n");
        return 1;
    }

    pid_t child = fork();
    if (child == 0) {
        /* Child: create a file and then signal parent to stop */
        usleep(200000); // Give parent time to start watcher_start()

        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/test_image.jpg", TEST_WATCH_DIR);

        int fd = open(file_path, O_WRONLY | O_CREAT, 0644);
        if (fd >= 0) {
            write(fd, "dummy data", 10);
            close(fd);
        }

        usleep(200000); // Give watcher time to process
        kill(getppid(), SIGTERM);
        _exit(0);
    }

    signal(SIGTERM, watcher_signal_handler);
    watcher_start();
    waitpid(child, NULL, 0);

    char expected_path[256];
    snprintf(expected_path, sizeof(expected_path), "%s/test_image.jpg", TEST_DEST_DIR);

    if (file_exists(expected_path)) {
        printf("PASS\n");
    } else {
        printf("FAIL (file not routed to destination)\n");
        return 1;
    }

    watcher_stop();

    /* Test 2: Invalid Inits */
    printf("Test 2: watcher_init(NULL path)... ");
    if (watcher_init(NULL, mock_cfg)) {
        printf("FAIL\n"); return 1;
    }
    printf("PASS\n");

    printf("Test 3: watcher_init(NULL config)... ");
    if (watcher_init(TEST_WATCH_DIR, NULL)) {
        printf("FAIL\n"); return 1;
    }
    printf("PASS\n");

    printf("Test 4: watcher_init(invalid path)... ");
    if (watcher_init("/nonexistent/path_123", mock_cfg)) {
        printf("FAIL\n"); return 1;
    }
    printf("PASS\n");

    cleanup();
    free(mock_cfg);
    log_close();
    printf("\nAll watcher tests passed!\n");
    return 0;
}
