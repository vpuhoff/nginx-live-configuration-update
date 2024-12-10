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