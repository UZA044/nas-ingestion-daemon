/*
 * test_config_parser.c — Tests config file parsing
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_config_parser
 *
 * Run:
 *   ./test_config_parser
 *
 * View logs (uses syslog ident "test-config"):
 *   journalctl -t test-config
 */

#include "logger.h"
#include "config.h"
#include <stdio.h>
#include <syslog.h>

int main(void)
{
    log_init("test-config", LOG_PID | LOG_NDELAY, LOG_DAEMON);

    Config *cfg = config_load("../config/nas-ingestion.conf");
    if (cfg == NULL) {
        log_write(LOG_ERR, "Failed to load config");
        log_close();
        return 1;
    }

    printf("watch_dir:      %s\n", cfg->paths.watch_dir);
    printf("photos_dir:     %s\n", cfg->paths.photos_dir);
    printf("docs_dir:       %s\n", cfg->paths.docs_dir);
    printf("quarantine_dir: %s\n", cfg->paths.quarantine_dir);
    printf("quality_thresh: %f\n", cfg->image.quality_threshold);
    printf("phash_distance: %d\n", cfg->image.phash_distance);
    printf("metrics_port:   %d\n", cfg->daemon.metrics_port);
    printf("health_socket:  %s\n", cfg->daemon.health_socket);
    printf("sqlite_path:    %s\n", cfg->database.sqlite_path);

    config_free();
    log_close();
    return 0;
}
