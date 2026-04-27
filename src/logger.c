#include "logger.h"
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static const char *g_ident = "nas_daemon";
static int g_option = LOG_PID | LOG_NDELAY;
static int g_facility = LOG_DAEMON; 

void log_init(const char *ident, int option, int facility)
  {
    if (ident)
        g_ident = ident;
    if (option >= 0)
        g_option = option;

    if (facility)
        g_facility = facility;

      openlog(g_ident, g_option, g_facility);

      setlogmask(LOG_UPTO(LOG_INFO));

      log_write(LOG_INFO, "STARTED PROGRAM : %s", "test");
  };

void log_write(LogLevel level, const char* format, ...){

    if (format == NULL){
        perror("The format to log was also NULL");
        return;
    }

    va_list vl;
    va_start(vl, format);
    vsyslog(level, format, vl);
    va_end(vl);

};

void log_set_level(LogLevel level){
    setlogmask(LOG_UPTO(level));
}

void log_close(){
    closelog();
}
