#include "detector.h"
#include "logger.h"
#include <stdio.h>



FileType detector_identify(const char *path){
    if (path == NULL) {
        log_write(LOG_ERR, "path argument passed into detecting the file type was NULL");
        return FILE_UNKNOWN;
    }
    FILE *fptr = fopen(path, "rb");

    if (fptr == NULL){
        log_write(LOG_ERR,  "Error opening the file for detecting file type.");
        return FILE_UNKNOWN;
    }

    unsigned char buffer[16];

    size_t bytes_read = fread(buffer, 1, 16, fptr);
    fclose(fptr);

    if (bytes_read < 3) {
          return FILE_UNKNOWN;
    }

    // JPEG: FF D8 FF
    if (buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF) {
        return FILE_JPEG;
    }

    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (bytes_read >= 8 &&
        buffer[0] == 0x89 && buffer[1] == 0x50 &&
        buffer[2] == 0x4E && buffer[3] == 0x47 &&
        buffer[4] == 0x0D && buffer[5] == 0x0A &&
        buffer[6] == 0x1A && buffer[7] == 0x0A) {
        return FILE_PNG;
    }

    // PDF: 25 50 44 46 (%PDF)
    if (buffer[0] == 0x25 && buffer[1] == 0x50 &&
        buffer[2] == 0x44 && buffer[3] == 0x46) {
        return FILE_PDF;
    }

    // ZIP/DOCX: 50 4B 03 04
    if (buffer[0] == 0x50 && buffer[1] == 0x4B &&
        buffer[2] == 0x03 && buffer[3] == 0x04) {
        return FILE_ZIP;
    }

    // MP4: offset 4, 66 74 79 70 (ftyp)
    if (bytes_read >= 8 &&
        buffer[4] == 0x66 && buffer[5] == 0x74 &&
        buffer[6] == 0x79 && buffer[7] == 0x70) {
        // Could be MP4 or HEIC — check further
        if (bytes_read >= 12 &&
            buffer[8] == 0x68 && buffer[9] == 0x65 &&
            buffer[10] == 0x69 && buffer[11] == 0x63) {
            return FILE_HEIC;   
        }
        return FILE_MP4;
    }

    return FILE_UNKNOWN;

}

const char* detector_get_type_string(FileType type) {
    switch (type) {
        case FILE_JPEG: return "JPEG";
        case FILE_PNG:  return "PNG";
        case FILE_HEIC: return "HEIC";
        case FILE_PDF:  return "PDF";
        case FILE_ZIP:  return "ZIP";
        case FILE_MP4:  return "MP4";
        default:        return "UNKNOWN";
    }
}


