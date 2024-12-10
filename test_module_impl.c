#include "nginx_test_mocks.h"
#include <string.h>
#include <signal.h>

// Тестовая реализация функции проверки конфигурации
ngx_int_t validate_nginx_config(ngx_http_request_t *r, const char* config_path) {
    FILE *f = fopen(config_path, "r");
    if (f == NULL) {
        if (r && r->connection && r->connection->log) {
            r->connection->log->log_error(r->connection->log->data, 
                "Failed to open config file: %s", config_path);
        }
        return NGX_ERROR;
    }

    char line[256];
    int valid = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "worker_processes") != NULL) {
            valid = 1;
            break;
        }
    }

    fclose(f);

    if (!valid && r && r->connection && r->connection->log) {
        r->connection->log->log_error(r->connection->log->data, 
            "Invalid configuration: worker_processes directive not found");
    }

    return valid ? NGX_OK : NGX_ERROR;
}

// Тестовая реализация функции применения конфигурации
ngx_int_t apply_nginx_config(ngx_http_request_t *r) {
    if (ngx_pid <= 0) {
        if (r && r->connection && r->connection->log) {
            r->connection->log->log_error(r->connection->log->data, 
                "Invalid PID: %d", (int)ngx_pid);
        }
        return NGX_ERROR;
    }

    if (r && r->connection && r->connection->log) {
        r->connection->log->log_error(r->connection->log->data, 
            "Successfully applied configuration (test mode)");
    }
    return NGX_OK;
}