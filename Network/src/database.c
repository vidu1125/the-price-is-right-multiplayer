#include <stdio.h>
#include <string.h>
#include "../include/database.h"

//==============================================================================
// DATABASE GLOBALS
//==============================================================================
sqlite3 *db = NULL;

//==============================================================================
// DATABASE INITIALIZATION
//==============================================================================

int db_init() {
    int rc = sqlite3_open("game_server.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    // Create users table
    const char *sql_users = 
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "display_name TEXT,"
        "balance INTEGER DEFAULT 1000,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    // Create match history table
    const char *sql_history = 
        "CREATE TABLE IF NOT EXISTS match_history ("
        "match_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "room_id INTEGER,"
        "winner_id INTEGER,"
        "game_mode INTEGER,"
        "player_count INTEGER,"
        "duration INTEGER,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql_users, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    
    rc = sqlite3_exec(db, sql_history, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    
    printf("Database initialized successfully\n");
    return 0;
}

//==============================================================================
// USER MANAGEMENT
//==============================================================================

int db_register_user(const char *username, const char *password_hash, const char *display_name) {
    const char *sql = "INSERT INTO users (username, password_hash, display_name) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, display_name, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_authenticate_user(const char *username, const char *password_hash, 
                         uint32_t *user_id, char *display_name, uint32_t *balance) {
    const char *sql = "SELECT user_id, display_name, balance FROM users WHERE username = ? AND password_hash = ?;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *user_id = sqlite3_column_int(stmt, 0);
        strncpy(display_name, (const char*)sqlite3_column_text(stmt, 1), 31);
        display_name[31] = '\0';
        *balance = sqlite3_column_int(stmt, 2);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

//==============================================================================
// DATABASE CLEANUP
//==============================================================================

void db_close() {
    if (db) {
        sqlite3_close(db);
    }
}