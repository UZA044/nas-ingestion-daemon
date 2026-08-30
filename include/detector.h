#ifndef DETECTOR_H
#define DETECTOR_H


typedef enum {
    FILE_UNKNOWN = 0, FILE_JPEG, FILE_PDF, FILE_PNG, FILE_ZIP, FILE_MP4, FILE_HEIC
} FileType;

FileType detector_identify(const char *path);

const char* map_file_type_dir(FileType type, const Config *cfg);

#endif // DETECTOR_H