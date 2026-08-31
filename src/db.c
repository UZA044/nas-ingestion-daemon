#include "db.h"
#include "logger.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

static sqlite3 *g_db = NULL;

bool db_init(const Config *config) {
    if (config == NULL) {
        log_write(LOG_ERR, "[DB] Config is NULL, cannot initialize database");
        return false;
    }

    const char *db_path = config->database.sqlite_path;
    if (db_path == NULL) {
        log_write(LOG_ERR, "[DB] SQLite path not found in config");
        return false;
    }

    int rc = sqlite3_open(db_path, &g_db);
    if (rc != SQLITE_OK) {
        log_write(LOG_ERR, "[DB] Cannot open database: %s", sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    // Create the 'files' table if it doesn't exist
    const char *sql = "CREATE TABLE IF NOT EXISTS files ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "filename TEXT NOT NULL, "
                      "file_type TEXT NOT NULL, "
                      "destination TEXT NOT NULL, "
                      "processed_at DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char *err_msg = NULL;
    rc = sqlite3_exec(g_db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        log_write(LOG_ERR, "[DB] Error creating table: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    log_write(LOG_INFO, "[DB] Database initialized successfully at %s", db_path);
    return true;
}

bool db_insert_file(const char *filename, const char *type, const char *destination) {
    if (g_db == NULL) {
        log_write(LOG_ERR, "[DB] Database not initialized");
        return false;
    }

    const char *sql = "INSERT INTO files (filename, file_type, destination) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_write(LOG_ERR, "[DB] Failed to prepare statement: %s", sqlite3_errmsg(g_db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, destination, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        log_write(LOG_ERR, "[DB] Failed to insert record: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

void db_close(void) {
    if (g_db != NULL) {
        sqlite3_close(g_db);
        g_db = NULL;
        log_write(LOG_INFO, "[DB] Database connection closed");
    }
}

int db_count_records(void) {
    if (g_db == NULL) return -1;

    const char *sql = "SELECT count(*) FROM files;";
    sqlite3_stmt *stmt;
    int count = 0;

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return count;
}
