#ifndef DATABASE_H
#define DATABASE_H

#include <stdint.h>
#include <sqlite3.h>

//==============================================================================
// DATABASE GLOBALS
//==============================================================================
extern sqlite3 *db;

//==============================================================================
// DATABASE FUNCTIONS
//==============================================================================

int db_init(void);
int db_register_user(const char *username, const char *password_hash, const char *display_name);
int db_authenticate_user(const char *username, const char *password_hash, 
                         uint32_t *user_id, char *display_name, uint32_t *balance);
void db_close(void);

#endif // DATABASE_H