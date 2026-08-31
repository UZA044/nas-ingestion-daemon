#ifndef NAS_DB_H
#define NAS_DB_H

#include <stdbool.h>
#include "config.h"

/*
 * db_init: Opens the database connection and creates the 'files' table if it doesn't exist.
 * Returns true on success, false on failure.
 */
bool db_init(const Config *config);

/*
 * db_insert_file: Records a processed file into the database.
 * Returns true on success, false on failure.
 */
bool db_insert_file(const char *filename, const char *type, const char *destination);

/*
 * db_close: Closes the database connection.
 */
void db_close(void);

/*
 * db_count_records: Returns the total number of records in the 'files' table.
 * Used primarily for testing.
 */
int db_count_records(void);

#endif // NAS_DB_H
