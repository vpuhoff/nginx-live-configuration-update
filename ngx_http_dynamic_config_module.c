#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_conf_file.h>
#include <sys/stat.h>

// Максимальный размер тела запроса (1MB)
#define MAX_BODY_SIZE 1048576

typedef struct {
    ngx_str_t  allowed_ips;
    ngx_str_t  auth_basic;
    ngx_str_t  auth_basic_user_file;
    size_t     max_body_size;
} ngx_http_dynamic_config_loc_conf_t;

static void* ngx_http_dynamic_config_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_dynamic_config_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_dynamic_config_handler(ngx_http_request_t *r);
static char* ngx_http_dynamic_config(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t validate_nginx_config(ngx_http_request_t *r, const char* config_path);
static ngx_int_t apply_nginx_config(ngx_http_request_t *r);
static void ngx_http_dynamic_config_post_handler(ngx_http_request_t *r);

// Определение команд модуля
static ngx_command_t ngx_http_dynamic_config_commands[] = {
    {
        ngx_string("dynamic_config"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        ngx_http_dynamic_config,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("dynamic_config_allowed_ips"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_dynamic_config_loc_conf_t, allowed_ips),
        NULL
    },
    {
        ngx_string("dynamic_config_max_body_size"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_dynamic_config_loc_conf_t, max_body_size),
        NULL
    },
    ngx_null_command
};

// Определение контекста модуля
static ngx_http_module_t ngx_http_dynamic_config_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    ngx_http_dynamic_config_create_loc_conf,  /* create location configuration */
    ngx_http_dynamic_config_merge_loc_conf    /* merge location configuration */
};

// Определение модуля
ngx_module_t ngx_http_dynamic_config_module = {
    NGX_MODULE_V1,
    &ngx_http_dynamic_config_module_ctx,     /* module context */
    ngx_http_dynamic_config_commands,        /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};

// Создание конфигурации локации
static void* ngx_http_dynamic_config_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_dynamic_config_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_dynamic_config_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->max_body_size = NGX_CONF_UNSET_SIZE;
    
    return conf;
}

// Слияние конфигураций
static char* ngx_http_dynamic_config_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_dynamic_config_loc_conf_t *prev = parent;
    ngx_http_dynamic_config_loc_conf_t *conf = child;

    ngx_conf_merge_str_value(conf->allowed_ips, prev->allowed_ips, "");
    ngx_conf_merge_size_value(conf->max_body_size, prev->max_body_size, MAX_BODY_SIZE);

    return NGX_CONF_OK;
}

// Обработчик POST запроса
static void ngx_http_dynamic_config_post_handler(ngx_http_request_t *r) {
    ssize_t                             n;
    int                                fd;
    char                               temp_config_path[] = "/tmp/nginx_temp_config_XXXXXX";

    if (r->request_body == NULL || r->request_body->bufs == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }

    // Создаем временный файл
    fd = mkstemp(temp_config_path);
    if (fd == -1) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to create temporary config file");
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    // Записываем конфигурацию
    ngx_buf_t *body = r->request_body->bufs->buf;
    n = write(fd, body->pos, body->last - body->pos);
    if (n != body->last - body->pos) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to write configuration to temporary file");
        close(fd);
        unlink(temp_config_path);
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    close(fd);

    // Проверяем конфигурацию
    if (validate_nginx_config(r, temp_config_path) != NGX_OK) {
        unlink(temp_config_path);
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }

    // Применяем конфигурацию
    if (apply_nginx_config(r) != NGX_OK) {
        unlink(temp_config_path);
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    // Удаляем временный файл
    unlink(temp_config_path);

    // Отправляем успешный ответ
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = 0;
    r->header_only = 1;

    ngx_http_finalize_request(r, ngx_http_send_header(r));
}

// Основной обработчик HTTP-запросов
static ngx_int_t ngx_http_dynamic_config_handler(ngx_http_request_t *r) {
    ngx_http_dynamic_config_loc_conf_t  *dlcf;
    
    dlcf = ngx_http_get_module_loc_conf(r, ngx_http_dynamic_config_module);

    // Проверяем метод
    if (!(r->method & NGX_HTTP_POST)) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    // Проверяем размер тела
    if (r->headers_in.content_length_n > (off_t)dlcf->max_body_size) {
        return NGX_HTTP_REQUEST_ENTITY_TOO_LARGE;
    }

    // Читаем тело запроса
    ngx_int_t rc = ngx_http_read_client_request_body(r, ngx_http_dynamic_config_post_handler);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }

    return NGX_DONE;
}

// Валидация конфигурации
static ngx_int_t validate_nginx_config(ngx_http_request_t *r, const char* config_path) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nginx -t -c %s", config_path);
    
    if (system(cmd) != 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Invalid nginx configuration");
        return NGX_ERROR;
    }
    
    return NGX_OK;
}

// Применение конфигурации
static ngx_int_t apply_nginx_config(ngx_http_request_t *r) {
    if (kill(ngx_pid, SIGHUP) == -1) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to send SIGHUP to nginx master process");
        return NGX_ERROR;
    }
    
    return NGX_OK;
}

// Инициализация обработчика
static char* ngx_http_dynamic_config(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_dynamic_config_handler;

    return NGX_CONF_OK;
}