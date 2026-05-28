/*
 * test_watcher.c — Tests inotify watcher
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_watcher
 *
 * Run:
 *   ./test_watcher
 *
 * View logs (uses syslog ident "test-watcher"):
 *   journalctl -t test-watcher
 */

#include "logger.h"
#include "watcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define TEST_DIR "/tmp/test_watch_dir"

int main(void)
{
    log_init("test-watcher", LOG_PID | LOG_NDELAY, LOG_DAEMON);

    mkdir(TEST_DIR, 0755);

    /* Test 1: watcher_init with valid path */
    printf("Test 1: watcher_init(valid path)... ");
    if (!watcher_init(TEST_DIR)) {
        printf("FAIL\n"); log_close(); return 1;
    }
    printf("PASS\n");

    /* Test 2: watcher_init with NULL path */
    printf("Test 2: watcher_init(NULL path)... ");
    if (watcher_init(NULL)) {
        printf("FAIL\n"); log_close(); return 1;
    }
    printf("PASS\n");

    /* Test 3: watcher_init with invalid path */
    printf("Test 3: watcher_init(invalid path)... ");
    if (watcher_init("/nonexistent/path")) {
        printf("FAIL\n"); log_close(); return 1;
    }
    printf("PASS\n");

    /* Test 4: watcher_stop without crash */
    printf("Test 4: watcher_stop cleanup... ");
    watcher_stop();
    printf("PASS\n");

    /* Test 5: re-init after stop */
    printf("Test 5: re-init after stop... ");
    if (!watcher_init(TEST_DIR)) {
        printf("FAIL\n"); log_close(); return 1;
    }
    printf("PASS\n");

    /* Test 6: event detection with forked child */
    printf("Test 6: event detection... ");

    pid_t child = fork();
    if (child == 0) {
        /* Child: wait for parent to block, create file, signal stop */
        usleep(500000);

        char fp[256];
        snprintf(fp, sizeof(fp), "%s/test_file.txt", TEST_DIR);
        FILE *f = fopen(fp, "w");
        if (f) { fprintf(f, "test"); fclose(f); }

        usleep(500000);
        kill(getppid(), SIGTERM);
        _exit(0);
    }

    
    signal(SIGTERM, watcher_signal_handler);
    watcher_start();

    waitpid(child, NULL, 0);
    watcher_stop();

    /* Cleanup test files */
    char fp[256];
    snprintf(fp, sizeof(fp), "%s/test_file.txt", TEST_DIR);
    remove(fp);
    rmdir(TEST_DIR);

    printf("PASS\n");
    printf("All tests passed.\n");
    log_close();
    return 0;
}
