#include "logger.h"
#include "config.h"
#include <stdbool.h>
#ifndef PIPELINE_H
#define PIPELINE_H

const char* map_file_type_dir(FileType type, const Config *cfg);

#endif // PIPELINE_H