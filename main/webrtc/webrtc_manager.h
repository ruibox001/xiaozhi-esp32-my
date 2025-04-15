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
    static WebrtcManager& Instance();
    
    // 获取实例（如果不存在则自动创建）
    std::unique_ptr<AppWebrtc>& WebrtcGet();
    
    // webrtc获取运行状态
    bool WebrtcIsRuning() const;

    //设置webrtc状态
    void WebrtcSetState(bool start);

    
private:
    WebrtcManager();
    ~WebrtcManager();

    // 销毁实例，建议外部调用WebrtcSetState(false)替代
    void WebrtcDestroy();
    
    std::unique_ptr<AppWebrtc> app_webrtc_ = nullptr;
    SemaphoreHandle_t mutex_ = nullptr;
    
};