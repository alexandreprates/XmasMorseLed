#pragma once
#include <cstddef>
#include <sys/types.h>
using esp_err_t = int;
constexpr int ESP_OK = 0;
constexpr int ESP_FAIL = -1;
using httpd_handle_t = void*;
enum httpd_method_t { HTTP_GET, HTTP_POST };
struct httpd_req_t { size_t content_len; void* user_ctx; void* test_state; };
struct httpd_uri_t { const char* uri; httpd_method_t method; esp_err_t (*handler)(httpd_req_t*); void* user_ctx; };
using httpd_uri_match_func_t = bool (*)(const char*, const char*, size_t);
struct httpd_config_t { int task_priority; int stack_size; int max_open_sockets; bool lru_purge_enable; int recv_wait_timeout; int send_wait_timeout; httpd_uri_match_func_t uri_match_fn; };
bool httpd_uri_match_wildcard(const char*, const char*, size_t);
#define HTTPD_DEFAULT_CONFIG() httpd_config_t{}
esp_err_t httpd_start(httpd_handle_t*, const httpd_config_t*);
esp_err_t httpd_stop(httpd_handle_t);
esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t*);
esp_err_t httpd_resp_set_status(httpd_req_t*, const char*);
esp_err_t httpd_resp_set_type(httpd_req_t*, const char*);
esp_err_t httpd_resp_set_hdr(httpd_req_t*, const char*, const char*);
esp_err_t httpd_resp_send(httpd_req_t*, const char*, ssize_t);
size_t httpd_req_get_hdr_value_len(httpd_req_t*, const char*);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t*, const char*, char*, size_t);
int httpd_req_recv(httpd_req_t*, char*, size_t);
