#include "logger.h"
#include <stdbool.h>
#ifndef CONFIG_H
#define CONFIG_H

typedef enum {
      SECTION_NONE,
      SECTION_PATHS,
      SECTION_IMAGE,
      SECTION_DAEMON,
      SECTION_DB
  } Section;

  
typedef struct{
    char *watch_dir;
    char *photos_dir;
    char *docs_dir;
    char *quarantine_dir;

} PathsConfig;

typedef struct{
    float quality_threshold;
    int phash_distance;
} ImageConfig;

typedef struct{
    LogLevel log_level;
    int metrics_port;
    char *health_socket;
} DaemonConfig;

typedef struct{
    char *sqlite_path;
} DatabaseConfig;

typedef struct {
      PathsConfig paths;
      ImageConfig image;
      DaemonConfig daemon;
      DatabaseConfig database;
} Config;

Config *config_load(const char *path);

const Config *config_get(void);

void config_free(void);

const Config *config_reload(void);

bool is_empty_or_spaces(const char *s);


#endif // CONFIG_H