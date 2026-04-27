#include "logger.h"

  int main(void)
  {
      /* Use a distinct ident so you can filter easily in journalctl */
      log_init("test-logger", 0 , 0);
      log_write(LOG_INFO,   "This is an info message");
      log_write(LOG_WARNING,"This is a warning");
      log_write(LOG_ERR,  "This is an error");

      log_set_level(LOG_DEBUG);
      log_write(LOG_DEBUG,  "Debug message now appears");

      log_close();
      return 0;
  }
