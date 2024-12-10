# Makefile для сборки модуля
NGINX_VERSION = 1.24.0
PCRE_VERSION = 8.45
OPENSSL_VERSION = 1.1.1w
NGINX_SRC = nginx-$(NGINX_VERSION)
PCRE_SRC = pcre-$(PCRE_VERSION)
OPENSSL_SRC = openssl-$(OPENSSL_VERSION)
MODULE_NAME = ngx_http_dynamic_config_module
MODULE_SRC = $(MODULE_NAME).c
CURR_DIR = $(shell pwd)

# Пути по умолчанию
PREFIX ?= /usr/local/nginx
SBIN_PATH ?= $(PREFIX)/sbin/nginx
CONF_PATH ?= $(PREFIX)/conf/nginx.conf
HTTP_LOG_PATH ?= $(PREFIX)/logs/access.log
ERROR_LOG_PATH ?= $(PREFIX)/logs/error.log

# Флаги компиляции
CFLAGS = -pipe -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g

.PHONY: all clean download configure build install prepare download-deps \
        download-pcre download-openssl download-nginx deps-debian deps-centos \
        versions status test clean-all

all: check-dirs download-deps prepare configure build install

check-dirs:
	@echo "Creating necessary directories..."
	@mkdir -p $(CURR_DIR)/build
	@mkdir -p $(CURR_DIR)/downloads

download-deps: check-dirs download-nginx download-pcre download-openssl

download-pcre:
	@echo "Downloading and extracting PCRE..."
	@cd $(CURR_DIR)/downloads && \
	wget -N https://downloads.sourceforge.net/project/pcre/pcre/$(PCRE_VERSION)/$(PCRE_SRC).tar.gz && \
	tar xzf $(PCRE_SRC).tar.gz -C $(CURR_DIR)/build/
	@echo "PCRE extracted to build/$(PCRE_SRC)"

download-openssl:
	@echo "Downloading and extracting OpenSSL..."
	@cd $(CURR_DIR)/downloads && \
	wget -N https://www.openssl.org/source/$(OPENSSL_SRC).tar.gz && \
	tar xzf $(OPENSSL_SRC).tar.gz -C $(CURR_DIR)/build/
	@echo "OpenSSL extracted to build/$(OPENSSL_SRC)"

download-nginx:
	@echo "Downloading and extracting Nginx..."
	@cd $(CURR_DIR)/downloads && \
	wget -N http://nginx.org/download/$(NGINX_SRC).tar.gz && \
	tar xzf $(NGINX_SRC).tar.gz -C $(CURR_DIR)/build/
	@echo "Nginx extracted to build/$(NGINX_SRC)"

prepare:
	@echo "Preparing module structure..."
	@mkdir -p $(CURR_DIR)/build/$(MODULE_NAME)
	@cp $(MODULE_SRC) $(CURR_DIR)/build/$(MODULE_NAME)/
	@echo "ngx_addon_name=$(MODULE_NAME)" > $(CURR_DIR)/build/$(MODULE_NAME)/config
	@echo "HTTP_MODULES=\"\$$HTTP_MODULES $(MODULE_NAME)\"" >> $(CURR_DIR)/build/$(MODULE_NAME)/config
	@echo "NGX_ADDON_SRCS=\"\$$NGX_ADDON_SRCS \$$ngx_addon_dir/$(MODULE_SRC)\"" >> $(CURR_DIR)/build/$(MODULE_NAME)/config
	@echo "Module structure prepared in build/$(MODULE_NAME)"

configure:
	@echo "Configuring Nginx with dynamic config module..."
	cd $(CURR_DIR)/build/$(NGINX_SRC) && ./configure \
		--prefix=$(PREFIX) \
		--sbin-path=$(SBIN_PATH) \
		--conf-path=$(CONF_PATH) \
		--http-log-path=$(HTTP_LOG_PATH) \
		--error-log-path=$(ERROR_LOG_PATH) \
		--with-http_ssl_module \
		--with-http_v2_module \
		--with-http_realip_module \
		--with-http_addition_module \
		--with-http_sub_module \
		--with-http_gunzip_module \
		--with-http_gzip_static_module \
		--with-threads \
		--with-file-aio \
		--with-pcre=$(CURR_DIR)/build/$(PCRE_SRC) \
		--with-openssl=$(CURR_DIR)/build/$(OPENSSL_SRC) \
		--add-module=$(CURR_DIR)/build/$(MODULE_NAME)

build: 
	@echo "Building Nginx with module..."
	cd $(CURR_DIR)/build/$(NGINX_SRC) && make

install: 
	@echo "Installing Nginx..."
	cd $(CURR_DIR)/build/$(NGINX_SRC) && sudo make install

clean:
	@echo "Cleaning build directory..."
	rm -rf $(CURR_DIR)/build
	rm -rf $(CURR_DIR)/downloads

clean-all: clean
	@echo "Cleaning all artifacts..."
	$(MAKE) -f Makefile.test clean

deps-debian:
	@echo "Installing dependencies for Debian/Ubuntu..."
	sudo apt-get update
	sudo apt-get install -y build-essential libpcre3 libpcre3-dev \
		zlib1g zlib1g-dev libssl-dev wget make gcc perl

deps-centos:
	@echo "Installing dependencies for CentOS..."
	sudo yum groupinstall -y "Development Tools"
	sudo yum install -y pcre-devel zlib-devel openssl-devel wget make gcc perl

versions:
	@echo "Nginx version: $(NGINX_VERSION)"
	@echo "PCRE version: $(PCRE_VERSION)"
	@echo "OpenSSL version: $(OPENSSL_VERSION)"

status:
	@echo "Checking build status..."
	@echo "Build directory:"
	@ls -l $(CURR_DIR)/build
	@echo "\nDownloads directory:"
	@ls -l $(CURR_DIR)/downloads

test:
	$(MAKE) -f Makefile.test test