#pragma once

#include "cJSON.h"
#include "esp_http_server.h"
#include "EmbeddedFiles.h"
#include "APIURL.hpp"
#include "./Gate/BaseGate.hpp"
#include "HW/SGHW_HAL.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <vector>

class WebServer
{
    /* Max length a file path can have on storage */
    #define HTTPSERVER_BUFFERSIZE (1024*8)

    private:
    WebServer();

    public:
    // Singleton pattern
    WebServer(WebServer const&) = delete;
    void operator=(WebServer const&) = delete;

    public:
    void init(SGHW_HAL* sghw_hal);

    void start();

    static WebServer& getI()
    {
        static WebServer instance;
        return instance;
    }
    private:
    static esp_err_t fileGetHandler(httpd_req_t* req);
    static esp_err_t otaUploadPostHandler(httpd_req_t* req);

    static esp_err_t webAPIGetHandler(httpd_req_t* req);
    static esp_err_t webAPIPostHandler(httpd_req_t* req);
    static esp_err_t gateControlAPIPostHandler(httpd_req_t* req);
    static esp_err_t webSocketHandler(httpd_req_t* req);

    // Get API
    char* getStatus();
    char* getSysInfo();
    char* getAllSoundLists();

    char* getGalaxyInfoJSON(GateGalaxy gate_galaxy);

    static void toHexString(char dst_hex_string[], const uint8_t* data, uint8_t len);

    static esp_err_t setContentTypeFromFile(httpd_req_t* req, const char* filename);

    static const EF_SFile* getFile(const char* filename);

    // WebSocket management
    void addWebSocketClient(int fd);
    void removeWebSocketClient(int fd);

    // Variable
    httpd_handle_t m_server;
    httpd_config_t m_config;

    SGHW_HAL* m_sghw_hal = nullptr;

    uint8_t m_buffers[HTTPSERVER_BUFFERSIZE];

    httpd_uri_t m_http_ui;
    httpd_uri_t m_http_get_api;
    httpd_uri_t m_http_post_api;
    // httpd_uri_t m_gate_control_api_post;

    httpd_uri_t m_http_ota_upload_post;
    httpd_uri_t m_http_websocket;

    // WebSocket clients
    std::vector<int> m_websocket_clients;
    SemaphoreHandle_t m_websocket_mutex;
    StaticSemaphore_t m_websocket_mutex_buffer;
};

