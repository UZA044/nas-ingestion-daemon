#include "config.h"
#include "logger.h"
#include "db.h"
#include <signal.h>
#include "watcher.h"

int main(){
    log_init("nas-daemon", LOG_PID | LOG_NDELAY, LOG_DAEMON);

    Config *cfg = config_load("../config/nas-ingestion.conf");
    if (cfg == NULL) {
        log_write(LOG_ERR, "Failed to load config");
        log_close();
        return 1;
    }

    if (!db_init(cfg)) {
        log_write(LOG_ERR, "Failed to initialize database");
        config_free();
        log_close();
        return 1;
    }

    signal(SIGTERM, watcher_signal_handler);

    if (!watcher_init(cfg->paths.watch_dir, cfg)) {
        log_write(LOG_ERR, "Failed to start watcher");
        config_free();
        log_close();
        return 1;
    }
    log_write(LOG_INFO, "Daemon started, watching %s", cfg->paths.watch_dir);

    watcher_start();

    watcher_stop();
    db_close();
    config_free();
    log_close();
    return 0;
};