#pragma once

typedef enum {
    DB_OK = 0,
    DB_SUCCESS = 0,

    /* HTTP / network */
    DB_ERR_HTTP,
    DB_ERR_TIMEOUT,
    DB_ERR_UNAUTHORIZED,
    DB_ERR_FORBIDDEN,

    /* Conflict / uniqueness */
    DB_ERR_CONFLICT,

    /* Data */
    DB_ERR_NOT_FOUND,
    DB_ERROR_NOT_FOUND,
    DB_ERR_EMPTY,
    DB_ERR_PARSE,
    DB_ERROR_PARSE,

    /* Logic */
    DB_ERR_INVALID_ARG,
    DB_ERROR_INVALID_PARAM,
    DB_ERROR_INTERNAL,
    DB_ERROR_NOT_IMPLEMENTED,
    DB_ERR_UNKNOWN
} db_error_t;
