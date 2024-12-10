import unittest
import requests
import subprocess
import time
import os
import json
from typing import Optional

class NginxDynamicConfigTests(unittest.TestCase):
    NGINX_BIN = "/usr/local/nginx/sbin/nginx"
    CONFIG_ENDPOINT = "http://localhost:8080/update-config"
    
    @classmethod
    def setUpClass(cls):
        """Подготовка тестового окружения"""
        # Создаем тестовую конфигурацию Nginx
        cls.create_test_config()
        
        # Запускаем Nginx
        subprocess.run([cls.NGINX_BIN, "-c", cls.test_config_path])
        time.sleep(1)  # Даем время на запуск
    
    @classmethod
    def tearDownClass(cls):
        """Очистка после тестов"""
        # Останавливаем Nginx
        subprocess.run([cls.NGINX_BIN, "-s", "stop"])
        
        # Удаляем тестовые файлы
        if os.path.exists(cls.test_config_path):
            os.remove(cls.test_config_path)
    
    @classmethod
    def create_test_config(cls):
        """Создание тестовой конфигурации Nginx"""
        config = """
        worker_processes 1;
        events {
            worker_connections 1024;
        }
        http {
            server {
                listen 8080;
                
                location /update-config {
                    dynamic_config;
                    dynamic_config_allowed_ips 127.0.0.1;
                    dynamic_config_max_body_size 1m;
                }
            }
        }
        """
        cls.test_config_path = "/tmp/nginx_test.conf"
        with open(cls.test_config_path, "w") as f:
            f.write(config)
    
    def send_config(self, config: str) -> requests.Response:
        """Отправка конфигурации на endpoint"""
        return requests.post(
            self.CONFIG_ENDPOINT,
            data=config,
            headers={"Content-Type": "text/plain"}
        )
    
    def test_valid_config(self):
        """Тест отправки валидной конфигурации"""
        config = """
        worker_processes 2;
        events {
            worker_connections 2048;
        }
        http {
            server {
                listen 8080;
                location /update-config {
                    dynamic_config;
                }
            }
        }
        """
        response = self.send_config(config)
        self.assertEqual(response.status_code, 200)
    
    def test_invalid_config(self):
        """Тест отправки неверной конфигурации"""
        config = """
        worker_processes 1;
        invalid_directive here;
        """
        response = self.send_config(config)
        self.assertEqual(response.status_code, 400)
    
    def test_large_config(self):
        """Тест отправки слишком большой конфигурации"""
        config = "x" * (2 * 1024 * 1024)  # 2MB
        response = self.send_config(config)
        self.assertEqual(response.status_code, 413)
    
    def test_wrong_method(self):
        """Тест использования неверного HTTP метода"""
        response = requests.get(self.CONFIG_ENDPOINT)
        self.assertEqual(response.status_code, 405)
    
    def test_config_apply(self):
        """Тест применения конфигурации"""
        # Отправляем новую конфигурацию
        config = """
        worker_processes 1;
        events {
            worker_connections 1024;
        }
        http {
            server {
                listen 8080;
                location /update-config {
                    dynamic_config;
                }
                location /test {
                    return 200 'test';
                }
            }
        }
        """
        response = self.send_config(config)
        self.assertEqual(response.status_code, 200)
        
        # Даем время на применение конфигурации
        time.sleep(1)
        
        # Проверяем, что новый location работает
        response = requests.get("http://localhost:8080/test")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.text, 'test')

    def test_concurrent_requests(self):
        """Тест параллельных запросов"""
        import concurrent.futures
        
        config = """
        worker_processes 1;
        events {
            worker_connections 1024;
        }
        http {
            server {
                listen 8080;
                location /update-config {
                    dynamic_config;
                }
            }
        }
        """
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            futures = [
                executor.submit(self.send_config, config)
                for _ in range(5)
            ]
            
            responses = [f.result() for f in futures]
            
            # Проверяем, что хотя бы один запрос успешен
            success = any(r.status_code == 200 for r in responses)
            self.assertTrue(success)

if __name__ == '__main__':
    unittest.main(verbosity=2)