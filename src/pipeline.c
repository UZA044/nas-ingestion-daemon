#include "pipeline.h"
#include "router.h"
#include "logger.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

void pipeline_process(const char *filepath, const Config *config){
    FileType file_type = detector_identify(filepath);
    const char *dest_dir = dest_dir_for_type(file_type, config);
    router_ensure_dir(dest_dir);
    bool success = router_move_file(filepath, dest_dir);

    if (success) {
        const char *type_str = detector_get_type_string(file_type);

        // Use a copy for basename as it can modify the string
        char *path_copy = strdup(filepath);
        char *fname = basename(path_copy);

        db_insert_file(fname, type_str, dest_dir);
        free(path_copy);

        log_write(LOG_INFO, "Routed %s (%s) → %s", filepath, type_str, dest_dir);
    } else {
        log_write(LOG_ERR, "Failed to route %s", filepath);
    }
}

const char* dest_dir_for_type(FileType type, const Config *cfg){
    switch (type) {
        case FILE_JPEG:
        case FILE_PNG:
        case FILE_HEIC:
            return cfg->paths.photos_dir;

        case FILE_PDF:
        case FILE_ZIP:
            return cfg->paths.docs_dir;

        case FILE_MP4:
        case FILE_UNKNOWN:
        default:
            return cfg->paths.quarantine_dir;
    }
}