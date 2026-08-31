#ifndef DETECTOR_H
#define DETECTOR_H


typedef enum {
    FILE_UNKNOWN = 0, FILE_JPEG, FILE_PDF, FILE_PNG, FILE_ZIP, FILE_MP4, FILE_HEIC
} FileType;

FileType detector_identify(const char *path);
const char* detector_get_type_string(FileType type);

#endif // DETECTOR_H
