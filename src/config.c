#include "config.h"
#include "logger.h"
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

static Config *config = NULL;
static char *default_config_path = "/config/nas-ingestion.conf";


Config *config_load(const char *path){
    config = calloc(1, sizeof(Config));
    if (config == NULL) {
        log_write(LOG_ERR, "Failed to allocate memory for config");
        return NULL;
    }

    FILE *fptr = fopen(path, "r");

    if (fptr == NULL){
        log_write(LOG_ERR,  "Error opening the conf file.");
        free(config);
        return NULL;
    }

    default_config_path  = strdup(path);

    char buff[512];

    Section current_section = SECTION_NONE;   

    while (fgets(buff, 512, fptr)) {

        buff[strcspn(buff, "\n")] = '\0';

        char *p = buff;

        if (*p == '\0' || *p && is_empty_or_spaces(p) || *p == '#'){
            continue;
        }

        if (*p == '[') {
            char *close = strchr(p, ']');
            if (close!= NULL){
                *close = '\0';
                if (strcmp(p + 1, "paths") == 0)
                    current_section = SECTION_PATHS;
                else if (strcmp(p + 1, "image") == 0)
                    current_section = SECTION_IMAGE;
                else if (strcmp(p + 1, "daemon") == 0)
                    current_section = SECTION_DAEMON;
                else if (strcmp(p + 1, "db") == 0)
                    current_section = SECTION_DB;
            }
            continue;
        }
        
        if (current_section == SECTION_NONE){
            log_write(LOG_ERR,  "In conf file correct configuration was not found.");
            continue;
        }
        
        log_write(LOG_INFO, "Current section: %d", current_section);

        char *equals = strchr(p, '=');
        if (equals == NULL) {
            log_write(LOG_ERR, "Config line malformed, missing '='");
            continue;
        }

        *equals = '\0';
        char *key = p;
        char *value = equals + 1;

        char *end = key + strlen(key) - 1;
        while (end > key && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }

        while (*value && isspace((unsigned char)*value)) {
            value++;
        }

        switch (current_section) {

        case SECTION_PATHS:
            if (strcmp(key, "watch_dir") == 0)
                config->paths.watch_dir = strdup(value);
            else if (strcmp(key, "photos_dir") == 0)
                config->paths.photos_dir = strdup(value);
            else if (strcmp(key, "docs_dir") == 0)
                config->paths.docs_dir = strdup(value);
            else if (strcmp(key, "quarantine_dir") == 0)
                config->paths.quarantine_dir = strdup(value);
            break;

        case SECTION_IMAGE:
            if (strcmp(key, "quality_threshold") == 0)
                config->image.quality_threshold = strtod(value, NULL);
            else if (strcmp(key, "phash_distance") == 0)
                config->image.phash_distance = (int)strtol(value, NULL, 10);
            break;

        case SECTION_DAEMON:
            if (strcmp(key, "log_level") == 0) {
                if (strcmp(value, "DEBUG") == 0)
                    config->daemon.log_level = LOG_DEBUG;
                else if (strcmp(value, "INFO") == 0)
                    config->daemon.log_level = LOG_INFO;
                else if (strcmp(value, "WARNING") == 0)
                    config->daemon.log_level = LOG_WARNING;
                else if (strcmp(value, "ERR") == 0)
                    config->daemon.log_level = LOG_ERR;
            } else if (strcmp(key, "metrics_port") == 0) {
                config->daemon.metrics_port = (int)strtol(value, NULL, 10);
            } else if (strcmp(key, "health_socket") == 0) {
                config->daemon.health_socket = strdup(value);
            }
            break;

        case SECTION_DB:
            if (strcmp(key, "sqlite_path") == 0)
                config->database.sqlite_path = strdup(value);
            break;

        default:
            break;
        }

    }
    fclose(fptr);
    return config;
}

const Config *config_get(void){
    if (config == NULL ){
        log_write(LOG_ERR,  "Config instance is NULL when attempting to get config.");

    }
    return config;
}

void config_free(void){
    if (config == NULL)
        return;

    free(config->paths.watch_dir);
    free(config->paths.photos_dir);
    free(config->paths.docs_dir);
    free(config->paths.quarantine_dir);

    free(config->daemon.health_socket);

    free(config->database.sqlite_path);

    free(config);
    config = NULL;
}

const Config *config_reload(void){
    config_free();

    config_load(default_config_path);
    return config;
}


bool is_empty_or_spaces(const char *s) {
      while (*s) {
          if (!isspace((unsigned char)*s)) {
              return false;
          }
          s++;
      }
      return true;            
  }