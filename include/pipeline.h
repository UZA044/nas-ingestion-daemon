#ifndef PIPELINE_H
#define PIPELINE_H
#include "config.h"
#include "detector.h"

const char* dest_dir_for_type(FileType type, const Config *cfg);
void pipeline_process(const char *filepath, const Config *config);

#endif // PIPELINE_H
