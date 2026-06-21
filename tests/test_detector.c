/*
 * test_detector.c — Tests file type detection via magic bytes
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_detector
 *
 * Run:
 *   ./test_detector
 *
 * View logs (uses syslog ident "test-detector"):
 *   journalctl -t test-detector
 *
 * Creates temp test files in /tmp/test_detector/ and cleans up after.
 */

#include "logger.h"
#include "detector.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>

#define TEST_DIR "/tmp/test_detector"

/* Helper: write raw bytes to a file */
static bool write_test_file(const char *path, const unsigned char *bytes, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL)
        return false;
    size_t written = fwrite(bytes, 1, len, f);
    fclose(f);
    return (written == len);
}

/* Helper: run one test case */
static int test_case(const char *name, const unsigned char *bytes, size_t len, FileType expected)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", TEST_DIR, name);

    if (!write_test_file(path, bytes, len)) {
        printf("FAIL (could not create %s)\n", name);
        return 1;
    }

    FileType result = detector_identify(path);
    remove(path);

    if (result == expected) {
        printf("PASS (%s → type %d)\n", name, result);
        return 0;
    } else {
        printf("FAIL (expected %d, got %d)\n", expected, result);
        return 1;
    }
}

int main(void)
{
    log_init("test-detector", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    mkdir(TEST_DIR, 0755);

    int failures = 0;

    /* Test 1: JPEG — FF D8 FF */
    {
        unsigned char bytes[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00};
        failures += test_case("test.jpg", bytes, sizeof(bytes), FILE_JPEG);
    }

    /* Test 2: PNG — 89 50 4E 47 0D 0A 1A 0A */
    {
        unsigned char bytes[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        failures += test_case("test.png", bytes, sizeof(bytes), FILE_PNG);
    }

    /* Test 3: PDF — 25 50 44 46 */
    {
        unsigned char bytes[] = {0x25, 0x50, 0x44, 0x46, 0x2D, 0x31, 0x2E, 0x34};
        failures += test_case("test.pdf", bytes, sizeof(bytes), FILE_PDF);
    }

    /* Test 4: ZIP/DOCX — 50 4B 03 04 */
    {
        unsigned char bytes[] = {0x50, 0x4B, 0x03, 0x04, 0x00, 0x00};
        failures += test_case("test.docx", bytes, sizeof(bytes), FILE_ZIP);
    }

    /* Test 5: MP4 — offset 4, 66 74 79 70 */
    {
        unsigned char bytes[] = {0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70};
        failures += test_case("test.mp4", bytes, sizeof(bytes), FILE_MP4);
    }

    /* Test 6: HEIC — offset 4, 66 74 79 70 68 65 69 63 */
    {
        unsigned char bytes[] = {0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70,
                                 0x68, 0x65, 0x69, 0x63};
        failures += test_case("test.heic", bytes, sizeof(bytes), FILE_HEIC);
    }

    /* Test 7: Unknown — random bytes */
    {
        unsigned char bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
        failures += test_case("test.unknown", bytes, sizeof(bytes), FILE_UNKNOWN);
    }

    /* Test 8: Empty file */
    {
        unsigned char bytes[] = {0x00, 0x00};
        failures += test_case("test.empty", bytes, sizeof(bytes), FILE_UNKNOWN);
    }

    /* Test 9: NULL path */
    printf("Test: NULL path... ");
    FileType result = detector_identify(NULL);
    if (result == FILE_UNKNOWN) {
        printf("PASS (returned FILE_UNKNOWN)\n");
    } else {
        printf("FAIL\n");
        failures++;
    }

    /* Test 10: Nonexistent file */
    printf("Test: nonexistent file... ");
    result = detector_identify("/tmp/test_detector_does_not_exist");
    if (result == FILE_UNKNOWN) {
        printf("PASS (returned FILE_UNKNOWN)\n");
    } else {
        printf("FAIL\n");
        failures++;
    }

    /* Cleanup */
    rmdir(TEST_DIR);

    printf("\n");
    if (failures == 0) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) FAILED.\n", failures);
    }

    log_close();
    return failures;
}
