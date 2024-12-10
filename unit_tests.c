#include "nginx_test_mocks.h"
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// Прототипы тестируемых функций
ngx_int_t validate_nginx_config(ngx_http_request_t *r, const char* config_path);
ngx_int_t apply_nginx_config(ngx_http_request_t *r);

// Вспомогательные функции для тестов
static char* create_temp_config(const char* content) {
    char template[] = "/tmp/nginx_test_XXXXXX";
    char *filename = NULL;
    int fd;
    ssize_t written;
    size_t content_len;

    // Создаем временный файл
    fd = mkstemp(template);
    if (fd == -1) {
        fprintf(stderr, "Failed to create temp file: %s\n", strerror(errno));
        return NULL;
    }

    // Записываем содержимое
    content_len = strlen(content);
    written = write(fd, content, content_len);
    
    if (written == -1) {
        fprintf(stderr, "Failed to write to temp file: %s\n", strerror(errno));
        close(fd);
        unlink(template);
        return NULL;
    }

    if (written != (ssize_t)content_len) {
        fprintf(stderr, "Incomplete write to temp file\n");
        close(fd);
        unlink(template);
        return NULL;
    }

    close(fd);

    // Копируем имя файла для возврата
    filename = strdup(template);
    if (filename == NULL) {
        fprintf(stderr, "Failed to allocate memory for filename\n");
        unlink(template);
        return NULL;
    }

    return filename;
}

static void cleanup_temp_file(const char* path) {
    if (path) {
        if (unlink(path) == -1) {
            fprintf(stderr, "Warning: Failed to delete temp file %s: %s\n", 
                    path, strerror(errno));
        }
        free((void*)path);
    }
}

void test_validate_config_valid(void) {
    printf("Testing valid configuration...\n");
    const char* valid_config = 
        "worker_processes 1;\n"
        "events {\n"
        "    worker_connections 1024;\n"
        "}\n"
        "http {\n"
        "    server {\n"
        "        listen 80;\n"
        "        location / {\n"
        "            root /usr/share/nginx/html;\n"
        "        }\n"
        "    }\n"
        "}\n";
    
    char* config_path = create_temp_config(valid_config);
    if (config_path == NULL) {
        fprintf(stderr, "Failed to create temporary configuration file\n");
        exit(1);
    }
    
    ngx_http_request_t r;
    mock_init_request(&r);
    
    ngx_int_t result = validate_nginx_config(&r, config_path);
    assert(result == NGX_OK);
    
    cleanup_temp_file(config_path);
    printf("✓ test_validate_config_valid passed\n");
}

void test_validate_config_invalid(void) {
    printf("Testing invalid configuration...\n");
    const char* invalid_config = 
        "invalid_directive;\n"
        "http {\n"
        "    broken_config_here\n"
        "}\n";
    
    char* config_path = create_temp_config(invalid_config);
    if (config_path == NULL) {
        fprintf(stderr, "Failed to create temporary configuration file\n");
        exit(1);
    }
    
    ngx_http_request_t r;
    mock_init_request(&r);
    
    ngx_int_t result = validate_nginx_config(&r, config_path);
    assert(result == NGX_ERROR);
    
    cleanup_temp_file(config_path);
    printf("✓ test_validate_config_invalid passed\n");
}

void test_apply_config(void) {
    printf("Testing config application...\n");
    ngx_http_request_t r;
    mock_init_request(&r);
    
    ngx_pid = getpid();
    
    ngx_int_t result = apply_nginx_config(&r);
    assert(result == NGX_OK);
    
    printf("✓ test_apply_config passed\n");
}

void test_validate_config_nonexistent(void) {
    printf("Testing nonexistent configuration file...\n");
    
    ngx_http_request_t r;
    mock_init_request(&r);
    
    ngx_int_t result = validate_nginx_config(&r, "/nonexistent/config/file");
    assert(result == NGX_ERROR);
    
    printf("✓ test_validate_config_nonexistent passed\n");
}

void test_apply_config_invalid_pid(void) {
    printf("Testing config application with invalid PID...\n");
    
    ngx_http_request_t r;
    mock_init_request(&r);
    
    ngx_pid = -1;  // Устанавливаем неверный PID
    
    ngx_int_t result = apply_nginx_config(&r);
    assert(result == NGX_ERROR);
    
    printf("✓ test_apply_config_invalid_pid passed\n");
}

int main(void) {
    printf("Running unit tests for nginx dynamic config module...\n\n");
    
    // Проверяем права на запись во временную директорию
    if (access("/tmp", W_OK) == -1) {
        fprintf(stderr, "Error: No write access to /tmp directory\n");
        return 1;
    }
    
    test_validate_config_valid();
    test_validate_config_invalid();
    test_validate_config_nonexistent();
    test_apply_config();
    test_apply_config_invalid_pid();
    
    printf("\nAll tests passed successfully!\n");
    return 0;
}