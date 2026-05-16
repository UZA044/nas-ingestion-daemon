/*
 * test_logger.c — Tests syslog logging
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make test_logger
 *
 * Run:
 *   ./test_logger
 *
 * View logs (messages go to syslog with ident "test-logger"):
 *   journalctl -t test-logger
 *
 * View all syslog messages:
 *   journalctl -f          (follow/live tail)
 *   journalctl -b          (since last boot)
 */

#include "logger.h"

int main(void)
{
    /* Use a distinct ident so you can filter easily in journalctl */
    log_init("test-logger", 0, 0);
    log_write(LOG_INFO,   "This is an info message");
    log_write(LOG_WARNING,"This is a warning");
    log_write(LOG_ERR,  "This is an error");

    log_set_level(LOG_DEBUG);
    log_write(LOG_DEBUG,  "Debug message now appears");

    log_close();
    return 0;
}
