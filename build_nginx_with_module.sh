#!/bin/bash
set -e

# Параметры по умолчанию
NGINX_VERSION="1.24.0"
INSTALL_PREFIX="/usr/local/nginx"
BUILD_DIR="/tmp/nginx-build"
MODULE_PATH="$(pwd)/ngx_http_dynamic_config_module.c"

# Функция для вывода сообщений
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Функция для проверки ошибок
check_error() {
    if [ $? -ne 0 ]; then
        log "Error: $1"
        exit 1
    fi
}

# Определение типа системы
detect_os() {
    if [ -f /etc/debian_version ]; then
        echo "debian"
    elif [ -f /etc/redhat-release ]; then
        echo "centos"
    else
        echo "unknown"
    fi
}

# Установка зависимостей
install_dependencies() {
    log "Installing dependencies..."
    OS_TYPE=$(detect_os)
    
    case $OS_TYPE in
        debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                libpcre3 \
                libpcre3-dev \
                zlib1g \
                zlib1g-dev \
                libssl-dev \
                wget
            ;;
        centos)
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                pcre-devel \
                zlib-devel \
                openssl-devel \
                wget
            ;;
        *)
            log "Unsupported OS type"
            exit 1
            ;;
    esac
    
    check_error "Failed to install dependencies"
}

# Подготовка рабочей директории
prepare_build_dir() {
    log "Preparing build directory..."
    rm -rf $BUILD_DIR
    mkdir -p $BUILD_DIR
    check_error "Failed to create build directory"
}

# Загрузка исходников Nginx
download_nginx() {
    log "Downloading Nginx sources..."
    cd $BUILD_DIR
    wget http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz
    check_error "Failed to download Nginx"
    
    tar xzf nginx-${NGINX_VERSION}.tar.gz
    check_error "Failed to extract Nginx sources"
}

# Сборка Nginx с модулем
build_nginx() {
    log "Building Nginx with dynamic config module..."
    cd $BUILD_DIR/nginx-${NGINX_VERSION}
    
    ./configure \
        --prefix=$INSTALL_PREFIX \
        --with-http_ssl_module \
        --with-http_v2_module \
        --with-http_realip_module \
        --with-http_addition_module \
        --with-http_sub_module \
        --with-http_gunzip_module \
        --with-http_gzip_static_module \
        --with-threads \
        --with-file-aio \
        --add-module=$MODULE_PATH
    check_error "Configuration failed"
    
    make
    check_error "Build failed"
    
    sudo make install
    check_error "Installation failed"
}

# Проверка установки
verify_installation() {
    log "Verifying installation..."
    $INSTALL_PREFIX/sbin/nginx -t
    check_error "Nginx test failed"
}

# Очистка
cleanup() {
    log "Cleaning up..."
    rm -rf $BUILD_DIR
}

# Основной процесс
main() {
    log "Starting Nginx build process..."
    
    install_dependencies
    prepare_build_dir
    download_nginx
    build_nginx
    verify_installation
    cleanup
    
    log "Nginx installation completed successfully!"
}

# Запуск скрипта
main "$@"