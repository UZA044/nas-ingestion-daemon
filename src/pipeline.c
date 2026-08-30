#include "pipeline.h"
#include "logger.h"
#include <stdio.h>

void pipeline_process(const char *srcpath, const Config *config){
    FileType file_type = detector_identify(src_path);
    const char *dest_dir = dest_dir_for_type(type, config);

    router_ensure_dir(dest_dir);
    bool success =router_move_file(filepath, dest_dir);

    //db_insert_file(basename(filepath), type, "accepted", dest_dir);

    log_write(LOG_INFO, "Routed %s → %s", filepath, dest_dir);
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