/*
 * test_db.c — Tests the SQLite database layer
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_db
 *
 * Run:
 *   ./test_db
 */

#include "db.h"
#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <syslog.h>


#define TEST_DB_PATH "/tmp/test_nas_daemon.db"

int main(void) {
    log_init("test-db", LOG_PID | LOG_NDELAY, LOG_DAEMON);

    // Cleanup old test DB
    unlink(TEST_DB_PATH);

    /* Setup Mock Config */
    Config cfg = {
        .database = {
            .sqlite_path = TEST_DB_PATH
        }
    };

    printf("Test 1: db_init... ");
    if (!db_init(&cfg)) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");

    printf("Test 2: db_insert_file... ");
    if (!db_insert_file("test_image.jpg", "FILE_JPEG", "/nas/photos/2023/01/01")) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");

    printf("Test 3: db_insert_multiple... ");
    if (!db_insert_file("test_doc.pdf", "FILE_PDF", "/nas/docs")) {
        printf("FAIL\n");
        return 1;
    }
    if (!db_insert_file("unknown.txt", "FILE_UNKNOWN", "/nas/quarantine")) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");

    printf("Test 4: db_close... ");
    db_close();
    printf("PASS\n");

    unlink(TEST_DB_PATH);
    log_close();
    printf("\nAll database tests passed!\n");
    return 0;
}
