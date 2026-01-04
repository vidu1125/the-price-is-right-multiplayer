#pragma once

typedef enum {
    DB_OK = 0,

    /* HTTP / network */
    DB_ERR_HTTP,
    DB_ERR_TIMEOUT,
    DB_ERR_UNAUTHORIZED,
    DB_ERR_FORBIDDEN,

    /* Data */
    DB_ERR_NOT_FOUND,
    DB_ERR_EMPTY,
    DB_ERR_PARSE,

    /* Logic */
    DB_ERR_INVALID_ARG,
    DB_ERR_UNKNOWN
} db_error_t;
