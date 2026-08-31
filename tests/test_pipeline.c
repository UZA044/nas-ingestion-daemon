/*
 * test_pipeline.c — Tests the ingestion pipeline logic
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_pipeline
 *
 * Run:
 *   ./test_pipeline
 */

#include "pipeline.h"
#include "config.h"
#include "detector.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <syslog.h>


#define TEST_BASE_DIR "/tmp/test_pipeline"
#define TEST_SRC_DIR   "/tmp/test_pipeline/src"
#define TEST_PHOTOS    "/tmp/test_pipeline/photos"
#define TEST_DOCS      "/tmp/test_pipeline/docs"
#define TEST_QUAR     "/tmp/test_pipeline/quarantine"

/* Helper: create a file with specific magic bytes */
static bool create_magic_file(const char *path, const unsigned char *magic, size_t magic_len) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fwrite(magic, 1, magic_len, f);
    fclose(f);
    return true;
}

static bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static void cleanup() {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_BASE_DIR);
    system(cmd);
}

static int test_case(const char *name, bool condition, const char *fail_msg) {
    printf("Test: %-50s ", name);
    if (condition) {
        printf("PASS\n");
        return 0;
    } else {
        printf("FAIL — %s\n", fail_msg);
        return 1;
    }
}

int main(void) {
    log_init("test-pipeline", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    cleanup();

    mkdir(TEST_BASE_DIR, 0755);
    mkdir(TEST_SRC_DIR, 0755);

    /* Setup Mock Config */
    Config cfg = {
        .paths = {
            .photos_dir = TEST_PHOTOS,
            .docs_dir = TEST_DOCS,
            .quarantine_dir = TEST_QUAR
        }
    };

    int failures = 0;

    /* ==================== TESTS FOR dest_dir_for_type ==================== */
    failures += test_case("dest_dir_for_type(FILE_JPEG)",
                          strcmp(dest_dir_for_type(FILE_JPEG, &cfg), TEST_PHOTOS) == 0,
                          "JPEG should go to photos");
    failures += test_case("dest_dir_for_type(FILE_PDF)",
                          strcmp(dest_dir_for_type(FILE_PDF, &cfg), TEST_DOCS) == 0,
                          "PDF should go to docs");
    failures += test_case("dest_dir_for_type(FILE_UNKNOWN)",
                          strcmp(dest_dir_for_type(FILE_UNKNOWN, &cfg), TEST_QUAR) == 0,
                          "Unknown should go to quarantine");

    /* ==================== TESTS FOR pipeline_process (Integration) ==================== */

    // 1. Test JPG routing
    char jpg_path[256];
    snprintf(jpg_path, sizeof(jpg_path), "%s/test.jpg", TEST_SRC_DIR);
    create_magic_file(jpg_path, (unsigned char[]){0xFF, 0xD8, 0xFF}, 3);

    pipeline_process(jpg_path, &cfg);
    char expected_jpg[256];
    snprintf(expected_jpg, sizeof(expected_jpg), "%s/test.jpg", TEST_PHOTOS);
    failures += test_case("pipeline_process(JPG)", file_exists(expected_jpg), "JPG not moved to photos");

    // 2. Test PDF routing
    char pdf_path[256];
    snprintf(pdf_path, sizeof(pdf_path), "%s/test.pdf", TEST_SRC_DIR);
    create_magic_file(pdf_path, (unsigned char[]){0x25, 0x50, 0x44, 0x46}, 4);

    pipeline_process(pdf_path, &cfg);
    char expected_pdf[256];
    snprintf(expected_pdf, sizeof(expected_pdf), "%s/test.pdf", TEST_DOCS);
    failures += test_case("pipeline_process(PDF)", file_exists(expected_pdf), "PDF not moved to docs");

    // 3. Test Unknown routing
    char txt_path[256];
    snprintf(txt_path, sizeof(txt_path), "%s/test.txt", TEST_SRC_DIR);
    create_magic_file(txt_path, (unsigned char[]){'h', 'e', 'l', 'l', 'o'}, 5);

    pipeline_process(txt_path, &cfg);
    char expected_txt[256];
    snprintf(expected_txt, sizeof(expected_txt), "%s/test.txt", TEST_QUAR);
    failures += test_case("pipeline_process(Unknown)", file_exists(expected_txt), "Txt not moved to quarantine");

    cleanup();
    log_close();

    if (failures == 0) {
        printf("\nAll pipeline tests passed!\n");
    } else {
        printf("\n%d pipeline test(s) FAILED.\n", failures);
    }

    return failures;
}
