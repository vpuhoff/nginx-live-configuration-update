#include "nginx_test_mocks.h"

// Определение глобальной переменной PID
ngx_pid_t ngx_pid;

// Функция логирования для тестов
static void test_log_error(void *data, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// Реализация функции инициализации мок-запроса
void mock_init_request(ngx_http_request_t *r) {
    static ngx_log_t log = {0};
    static ngx_connection_t conn = {0};
    
    log.log_error = test_log_error;
    conn.log = &log;
    r->connection = &conn;
}