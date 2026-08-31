/*
 * test_router.c — Tests file routing (move, directory creation, collision handling)
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_router
 *
 * Run:
 *   ./test_router
 *
 * View logs (uses syslog ident "test-router"):
 *   journalctl -t test-router
 */

#include "logger.h"
#include "router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>

#define TEST_BASE_DIR "/tmp/test_router"
#define TEST_SRC_DIR  "/tmp/test_router/src"
#define TEST_DEST_DIR "/tmp/test_router/dest"

/* Helper: create a test file with content */
static bool create_test_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return false;
    if (content) fprintf(f, "%s", content);
    fclose(f);
    return true;
}

/* Helper: check if file exists */
static bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

/* Helper: check if path is a directory */
static bool is_directory(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Helper: read entire file content */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

/* Helper: run one test case, return 0 on pass, 1 on fail */
static int test_case(const char *name, int condition, const char *fail_msg) {
    printf("Test: %-50s ", name);
    if (condition) {
        printf("PASS\n");
        return 0;
    } else {
        printf("FAIL%s%s\n", fail_msg ? " — " : "", fail_msg ? fail_msg : "");
        return 1;
    }
}

/* Cleanup test directory */
static void cleanup_test_dir(void) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_BASE_DIR);
    system(cmd);
}

int main(void) {
    log_init("test-router", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    cleanup_test_dir();

    int failures = 0;

    /* ==================== TESTS FOR router_ensure_dir ==================== */

    /* Test 1: router_ensure_dir with NULL path */
    failures += test_case("router_ensure_dir(NULL)",
                          !router_ensure_dir(NULL),
                          "should return false for NULL");

    /* Test 2: router_ensure_dir creates new directory */
    char new_dir[256];
    snprintf(new_dir, sizeof(new_dir), "%s/new_dir", TEST_BASE_DIR);
    failures += test_case("router_ensure_dir(new_dir)",
                          router_ensure_dir(new_dir) && is_directory(new_dir),
                          "should create directory and return true");

    /* Test 3: router_ensure_dir on existing directory */
    failures += test_case("router_ensure_dir(existing_dir)",
                          router_ensure_dir(new_dir),
                          "should return true for existing directory");

    /* Test 4: router_ensure_dir creates nested directories */
    char nested_dir[256];
    snprintf(nested_dir, sizeof(nested_dir), "%s/a/b/c/d", TEST_BASE_DIR);
    failures += test_case("router_ensure_dir(nested_dirs)",
                          router_ensure_dir(nested_dir) && is_directory(nested_dir),
                          "should create nested directories");

    /* Test 5: router_ensure_dir with invalid path (permission denied simulation) */
    /* Skip - hard to test portably */

    /* ==================== TESTS FOR router_move_file ==================== */

    /* Setup source directory and files */
    mkdir(TEST_SRC_DIR, 0755);
    mkdir(TEST_DEST_DIR, 0755);

    char src_file[256], dest_file[256];

    /* Test 6: Move file to existing destination directory */
    snprintf(src_file, sizeof(src_file), "%s/test1.txt", TEST_SRC_DIR);
    create_test_file(src_file, "content1");
    failures += test_case("router_move_file(to existing dir)",
                          router_move_file(src_file, TEST_DEST_DIR) &&
                          !file_exists(src_file) &&
                          file_exists("/tmp/test_router/dest/test1.txt"),
                          "should move file to existing dir");

    /* Test 7: Move file to non-existing destination directory (creates it) */
    char new_dest[256];
    snprintf(new_dest, sizeof(new_dest), "%s/new_dest", TEST_BASE_DIR);
    snprintf(src_file, sizeof(src_file), "%s/test2.txt", TEST_SRC_DIR);
    create_test_file(src_file, "content2");
    failures += test_case("router_move_file(creates dest dir)",
                          router_move_file(src_file, new_dest) &&
                          !file_exists(src_file) &&
                          file_exists("/tmp/test_router/new_dest/test2.txt") &&
                          is_directory(new_dest),
                          "should create destination directory and move file");

    /* Test 8: Collision handling - file exists, should create _1 suffix */
    snprintf(src_file, sizeof(src_file), "%s/test3.txt", TEST_SRC_DIR);
    create_test_file(src_file, "content3a");
    /* First move */
    router_move_file(src_file, TEST_DEST_DIR);
    /* Second move with same name - should create test3_1.txt */
    create_test_file(src_file, "content3b");
    failures += test_case("router_move_file(collision -> _1)",
                          router_move_file(src_file, TEST_DEST_DIR) &&
                          file_exists("/tmp/test_router/dest/test3.txt") &&
                          file_exists("/tmp/test_router/dest/test3_1.txt"),
                          "should create _1 suffix on collision");

    /* Test 9: Multiple collisions - _1, _2, _3 */
    for (int i = 0; i < 3; i++) {
        create_test_file(src_file, "content");
        router_move_file(src_file, TEST_DEST_DIR);
    }
    failures += test_case("router_move_file(multiple collisions -> _2, _3)",
                          file_exists("/tmp/test_router/dest/test3_2.txt") &&
                          file_exists("/tmp/test_router/dest/test3_3.txt"),
                          "should create _2, _3 suffixes");

    /* Test 10: File without extension */
    char src_noext[256];
    snprintf(src_noext, sizeof(src_noext), "%s/README", TEST_SRC_DIR);
    create_test_file(src_noext, "no extension");
    failures += test_case("router_move_file(no extension)",
                          router_move_file(src_noext, TEST_DEST_DIR) &&
                          file_exists("/tmp/test_router/dest/README"),
                          "should handle files without extension");

    /* Test 11: Collision on file without extension */
    create_test_file(src_noext, "another");
    failures += test_case("router_move_file(collision no extension -> _1)",
                          router_move_file(src_noext, TEST_DEST_DIR) &&
                          file_exists("/tmp/test_router/dest/README_1"),
                          "should create _1 suffix for files without extension");

    /* Test 12: File with multiple dots (e.g., archive.tar.gz) */
    char src_multidot[256];
    snprintf(src_multidot, sizeof(src_multidot), "%s/archive.tar.gz", TEST_SRC_DIR);
    create_test_file(src_multidot, "multi dot");
    failures += test_case("router_move_file(multiple dots)",
                          router_move_file(src_multidot, TEST_DEST_DIR) &&
                          file_exists("/tmp/test_router/dest/archive.tar.gz"),
                          "should preserve full extension after last dot");

    /* Test 13: Collision on multiple dots */
    create_test_file(src_multidot, "another");
    failures += test_case("router_move_file(collision multiple dots -> _1)",
                          router_move_file(src_multidot, TEST_DEST_DIR) &&
                          file_exists("/tmp/test_router/dest/archive.tar_1.gz"),
                          "should insert _1 before last dot");

    /* Test 14: NULL source path */
    failures += test_case("router_move_file(NULL src)",
                          !router_move_file(NULL, TEST_DEST_DIR),
                          "should return false for NULL src");

    /* Test 15: NULL destination directory */
    snprintf(src_file, sizeof(src_file), "%s/test_null_dest.txt", TEST_SRC_DIR);
    create_test_file(src_file, "content");
    failures += test_case("router_move_file(NULL dest)",
                          !router_move_file(src_file, NULL),
                          "should return false for NULL dest");

    /* Test 16: Non-existent source file */
    failures += test_case("router_move_file(nonexistent src)",
                          !router_move_file("/tmp/does_not_exist.txt", TEST_DEST_DIR),
                          "should return false for nonexistent source");

    /* Test 17: Empty filename (edge case) */
    /* Not easily testable with current API */

    /* Test 18: Move preserves file content */
    char src_content[256];
    snprintf(src_content, sizeof(src_content), "%s/content_test.txt", TEST_SRC_DIR);
    const char *test_content = "Hello, World! This is test content.\nWith multiple lines.";
    create_test_file(src_content, test_content);
    router_move_file(src_content, TEST_DEST_DIR);
    char *moved_content = read_file("/tmp/test_router/dest/content_test.txt");
    failures += test_case("router_move_file(preserves content)",
                          moved_content && strcmp(moved_content, test_content) == 0,
                          "moved file should have identical content");
    free(moved_content);

    /* Test 19: Source file removed after successful move */
    snprintf(src_file, sizeof(src_file), "%s/removed_after_move.txt", TEST_SRC_DIR);
    create_test_file(src_file, "delete me");
    router_move_file(src_file, TEST_DEST_DIR);
    failures += test_case("router_move_file(removes source)",
                          !file_exists(src_file),
                          "source file should be removed after move");

    /* Test 20: Destination directory created with correct permissions */
    char perm_dest[256];
    snprintf(perm_dest, sizeof(perm_dest), "%s/perm_test", TEST_BASE_DIR);
    snprintf(src_file, sizeof(src_file), "%s/perm_file.txt", TEST_SRC_DIR);
    create_test_file(src_file, "x");
    router_move_file(src_file, perm_dest);
    struct stat st;
    stat(perm_dest, &st);
    failures += test_case("router_move_file(dir permissions 0755)",
                          (st.st_mode & 0777) == 0755,
                          "created directory should have 0755 permissions");

    /* ==================== TESTS FOR copy_file (internal) ==================== */
    /* Note: copy_file is static, tested indirectly via EXDEV simulation if possible */

    /* Cleanup */
    cleanup_test_dir();

    printf("\n");
    if (failures == 0) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) FAILED.\n", failures);
    }

    log_close();
    return failures;
}