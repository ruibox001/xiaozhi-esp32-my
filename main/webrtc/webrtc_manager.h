#pragma once
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

class AppWebrtc; // 前向声明

class WebrtcManager {
public:
    // 获取单例实例
    static WebrtcManager& instance();
    
    // 获取实例（如果不存在则自动创建）
    std::unique_ptr<AppWebrtc>& get();
    
    // 销毁实例
    void destroy();
    
    // 检查是否已创建
    bool is_created() const;

private:
    WebrtcManager();
    ~WebrtcManager();
    
    std::unique_ptr<AppWebrtc> app_webrtc_ = nullptr;
    SemaphoreHandle_t mutex_ = nullptr;
    
};