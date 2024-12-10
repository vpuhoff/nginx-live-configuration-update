#ifndef NGINX_TEST_MOCKS_H
#define NGINX_TEST_MOCKS_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Базовые типы Nginx
typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef intptr_t        ngx_flag_t;
typedef pid_t          ngx_pid_t;

#define NGX_OK          0
#define NGX_ERROR     -1

// Объявление глобальной переменной PID
extern ngx_pid_t ngx_pid;

// Мок структуры логирования
typedef struct {
    void *data;
    void (*log_error)(void *data, const char *fmt, ...);
} ngx_log_t;

// Мок структуры соединения
typedef struct {
    ngx_log_t *log;
} ngx_connection_t;

// Мок структуры запроса
typedef struct ngx_http_request_s {
    ngx_connection_t    *connection;
} ngx_http_request_t;

// Объявления функций
void mock_init_request(ngx_http_request_t *r);

#endif // NGINX_TEST_MOCKS_H