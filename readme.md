# Nginx Dynamic Configuration Module

This module enhances Nginx by enabling dynamic configuration updates without requiring a full server restart. System administrators and DevOps engineers can update their Nginx configuration through HTTP requests while maintaining server stability and ensuring configuration validity. This capability is particularly valuable in environments where continuous deployment is practiced or where configuration changes need to be made with minimal service interruption.

## Features

The module provides a secure and efficient mechanism for updating Nginx configuration with the following capabilities:

- Dynamic configuration updates through HTTP POST requests, allowing remote configuration changes
- Comprehensive configuration validation before application to prevent server misconfigurations
- Graceful configuration reload without service interruption, maintaining active connections
- IP-based access control for configuration updates, ensuring security
- Request size limitations to prevent denial of service attacks
- Detailed error reporting for troubleshooting
- Automatic configuration backup before applying changes

## Prerequisites

The module requires the following software components to be installed:

- Nginx (version 1.24.0 or higher)
- OpenSSL (version 1.1.1w or higher)
- PCRE library (version 8.45 or higher)
- GCC compiler
- Make build system
- wget for downloading dependencies
- perl for OpenSSL compilation

For Debian/Ubuntu-based systems, install the required dependencies by executing:

```bash
sudo make deps-debian
```

This command will install the following packages:
- build-essential
- libpcre3
- libpcre3-dev
- zlib1g
- zlib1g-dev
- libssl-dev
- wget
- make
- gcc
- perl

For CentOS-based systems, install the required dependencies by executing:

```bash
sudo make deps-centos
```

This command will install the following packages:
- Development Tools group
- pcre-devel
- zlib-devel
- openssl-devel
- wget
- make
- gcc
- perl

## Installation

Follow these steps to install the module:

1. Clone the repository:
```bash
git clone https://github.com/yourusername/nginx-live-configuration-update.git
cd nginx-live-configuration-update
```

2. Build and install the module:
```bash
make
```

The make command executes the following sequence:
1. Creates necessary build directories
2. Downloads and extracts Nginx source code
3. Downloads and extracts PCRE library
4. Downloads and extracts OpenSSL
5. Prepares the module structure
6. Configures Nginx with the dynamic configuration module
7. Builds Nginx with all dependencies
8. Installs the compiled version to the specified prefix

By default, the installation uses the following paths:
- Nginx binary: /usr/local/nginx/sbin/nginx
- Configuration file: /usr/local/nginx/conf/nginx.conf
- Access log: /usr/local/nginx/logs/access.log
- Error log: /usr/local/nginx/logs/error.log

## Configuration

To enable the module, add the following directives to your Nginx configuration file:

```nginx
location /update-config {
    dynamic_config;
    dynamic_config_allowed_ips 127.0.0.1;
    dynamic_config_max_body_size 1m;
}
```

Configuration directives explained:

- `dynamic_config`: Enables the dynamic configuration module for the specified location
- `dynamic_config_allowed_ips`: Specifies IP addresses that are allowed to submit configuration updates
- `dynamic_config_max_body_size`: Sets the maximum size for configuration update requests

## Usage

To update the Nginx configuration, send a POST request with the new configuration content. Here's an example using curl:

```bash
curl -X POST http://localhost/update-config \
     -H "Content-Type: text/plain" \
     --data-binary @new-nginx.conf
```

The module processes the request as follows:

1. Validates the request source IP against allowed IPs
2. Checks the request size against the configured maximum
3. Creates a temporary file with the new configuration
4. Validates the new configuration syntax
5. Creates a backup of the current configuration
6. Applies the new configuration if valid
7. Returns an appropriate HTTP status code:
   - 200: Configuration successfully updated
   - 400: Invalid configuration syntax
   - 403: Access denied (IP not allowed)
   - 413: Request entity too large
   - 500: Internal server error

## Development and Testing

The project includes a comprehensive test suite that verifies the module's functionality. To run the tests:

```bash
make test
```

The test suite includes:
- Unit tests for configuration validation
- Tests for configuration application
- Error handling tests
- Mock implementations for Nginx dependencies

Development utilities available:

```bash
# Check the current build status
make status

# Display component versions
make versions

# Clean build artifacts
make clean

# Remove all artifacts including tests
make clean-all
```

## Project Structure

The project contains the following files:

```
.
├── Makefile                    # Main build system for module compilation
├── Makefile.test               # Build system for test compilation
├── ngx_http_dynamic_config_module.c    # Core module implementation
├── nginx_test_mocks.h          # Test mock object declarations
├── nginx_test_mocks_impl.c     # Test mock object implementations
├── test_module_impl.c          # Test function implementations
├── unit_tests.c                # Test suite
└── README.md                   # Documentation
```

## Configuration Examples and Usage

### Basic Configuration

The module requires configuration both at the Nginx level and for individual configuration updates. Here's a detailed guide on how to set up and use the module effectively.

### Nginx Server Configuration

Basic configuration example for enabling the module:

```nginx
http {
    # Global configuration
    client_max_body_size 1m;  # Ensure this is large enough for your config files

    server {
        listen 8080;
        server_name localhost;

        # Module configuration endpoint
        location /update-config {
            # Enable the dynamic configuration module
            dynamic_config;
            
            # Allow only specific IP addresses
            dynamic_config_allowed_ips 127.0.0.1 192.168.1.100;
            
            # Set maximum configuration file size
            dynamic_config_max_body_size 1m;
        }
    }
}
```

### Advanced Configuration Example

For production environments with enhanced security:

```nginx
http {
    # Main server configuration
    server {
        listen 443 ssl;
        server_name config-management.example.com;

        # SSL configuration
        ssl_certificate     /path/to/cert.pem;
        ssl_certificate_key /path/to/cert.key;

        # Configuration management endpoint with authentication
        location /update-config {
            # Enable the module
            dynamic_config;
            
            # Restrict access to internal networks
            dynamic_config_allowed_ips 10.0.0.0/8 172.16.0.0/12 192.168.0.0/16;
            
            # Set reasonable size limit
            dynamic_config_max_body_size 2m;

            # Add basic authentication
            auth_basic "Configuration Management";
            auth_basic_user_file /etc/nginx/.htpasswd;

            # Add request rate limiting
            limit_req zone=config_updates burst=5;
        }
    }
}
```

### Sending Configuration Updates

Here are examples of how to send configuration updates to the module using different methods:

#### Using curl

Basic configuration update:

```bash
curl -X POST http://localhost:8080/update-config \
     -H "Content-Type: text/plain" \
     --data-binary @new-nginx.conf
```

Secure configuration update with authentication:

```bash
curl -X POST https://config-management.example.com/update-config \
     -H "Content-Type: text/plain" \
     --data-binary @new-nginx.conf \
     -u admin:password \
     --cacert /path/to/ca.crt
```

#### Example Configuration Payload

Here's an example of a configuration file you might send:

```nginx
# new-nginx.conf
worker_processes auto;
events {
    worker_connections 1024;
}

http {
    include       mime.types;
    default_type  application/octet-stream;

    # Updated logging format
    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';

    access_log  logs/access.log  main;

    sendfile        on;
    keepalive_timeout  65;

    # Updated server configuration
    server {
        listen       80;
        server_name  example.com;

        location / {
            root   html;
            index  index.html index.htm;
        }

        # New caching configuration
        location /static {
            root   /var/www/static;
            expires 30d;
        }
    }
}
```

#### Using Python

Example script for updating configuration:

```python
import requests
import os

def update_nginx_config(config_file, endpoint, auth=None):
    """
    Update Nginx configuration using the dynamic config module.
    
    Args:
        config_file: Path to the new configuration file
        endpoint: URL of the update-config endpoint
        auth: Tuple of (username, password) if authentication is required
    """
    with open(config_file, 'r') as f:
        config_content = f.read()
    
    headers = {'Content-Type': 'text/plain'}
    
    response = requests.post(
        endpoint,
        data=config_content,
        headers=headers,
        auth=auth,
        verify=True  # For HTTPS
    )
    
    if response.status_code == 200:
        print("Configuration updated successfully")
    else:
        print(f"Error updating configuration: {response.status_code}")
        print(response.text)

# Example usage
update_nginx_config(
    'new-nginx.conf',
    'https://config-management.example.com/update-config',
    auth=('admin', 'password')
)
```

### Verifying Configuration Updates

After sending a configuration update, you can verify its application:

1. Check the Nginx process status:
```bash
ps aux | grep nginx
```

2. Verify the configuration syntax:
```bash
nginx -t
```

3. Check Nginx error logs for confirmation:
```bash
tail -f /usr/local/nginx/logs/error.log
```

The module will automatically validate the configuration before applying it, but these steps provide additional verification that the changes were applied successfully.

### Best Practices

When updating Nginx configuration dynamically, follow these practices:

1. Always test new configurations in a staging environment first.
2. Keep a backup of working configurations.
3. Implement proper access controls through IP restrictions and authentication.
4. Use HTTPS for configuration updates in production environments.
5. Monitor Nginx logs during and after configuration updates.
6. Implement a rollback strategy for failed configuration updates.

These examples and practices provide a foundation for safely managing Nginx configuration updates in various environments, from development to production.

## Troubleshooting

Common issues and their solutions:

Configuration Update Failures:
- Verify that the configuration syntax is correct using `nginx -t`
- Check the Nginx error logs at /usr/local/nginx/logs/error.log
- Ensure the process has proper permissions to read and write configuration files
- Verify that the client IP is in the allowed IP list

Build Issues:
- Ensure all dependencies are properly installed
- Verify sufficient disk space is available
- Check for appropriate file permissions in the build directory
- Review the build logs for specific error messages

## Contributing

Contributions to improve the module are welcome. Please follow these guidelines:

1. Fork the repository to your own GitHub account
2. Create a feature branch from the main branch
3. Make your changes, following the existing code style
4. Add or update tests to cover your changes
5. Submit a pull request with a clear description of the changes

Ensure that all code:
- Follows the existing style conventions
- Includes appropriate tests
- Is properly documented
- Does not break existing functionality

## License

This project is licensed under the MIT License. See the LICENSE file for complete details about your rights and obligations under this license.

## Support

For assistance with the module:

1. Review existing GitHub issues before creating a new one
2. Provide detailed information when reporting problems:
   - Nginx version
   - Module version
   - Operating system details
   - Error messages
   - Steps to reproduce the issue

## Acknowledgments

This project benefits from the contributions and support of:
- The Nginx development team
- Contributors to the project
- The open source community

For the latest updates and documentation, visit the project repository at:
https://github.com/yourusername/nginx-live-configuration-update