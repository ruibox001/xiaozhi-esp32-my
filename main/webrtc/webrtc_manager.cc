#include "webrtc_manager.h"
#include "app_webrtc.h"

#define TAG "WebrtcManager"

WebrtcManager& WebrtcManager::Instance() {
    static WebrtcManager instance;
    return instance;
}

WebrtcManager::WebrtcManager() {
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
    }
}

WebrtcManager::~WebrtcManager() {
    if (app_webrtc_) {
        ESP_LOGI(TAG, "AppWebrtc destroyed = %p", app_webrtc_.get());
        app_webrtc_ = nullptr;
    }
    if (mutex_) {
        vSemaphoreDelete(mutex_);
    }
}

std::unique_ptr<AppWebrtc>& WebrtcManager::WebrtcGet() {
    if (app_webrtc_) return app_webrtc_;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return app_webrtc_; // Return null unique_ptr reference
    }

    if (!app_webrtc_) {
        app_webrtc_ = std::make_unique<AppWebrtc>();
    }

    xSemaphoreGive(mutex_);
    ESP_LOGI(TAG, "webrtc create instance %p", app_webrtc_.get());
    return app_webrtc_;
}

void WebrtcManager::WebrtcDestroy() {
    if (!mutex_ || !app_webrtc_) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return;
    }

    if (app_webrtc_) {
        ESP_LOGI(TAG, "AppWebrtc destroyed = %p", app_webrtc_.get());
        app_webrtc_ = nullptr;
    }

    xSemaphoreGive(mutex_);
    ESP_LOGI(TAG, "webrtc已销毁");
}

bool WebrtcManager::WebrtcIsRuning() const {
    if (!mutex_) return false;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return false;
    }

    bool created = (app_webrtc_ != nullptr);
    xSemaphoreGive(const_cast<SemaphoreHandle_t>(mutex_));
    return created;
}

void WebrtcManager::WebrtcSetState(bool start) {

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return;
    }

    if (start) {
        if (!app_webrtc_) {
            app_webrtc_ = std::make_unique<AppWebrtc>();
            ESP_LOGI(TAG, "AppWebrtc create %p", app_webrtc_.get());
        }
    } else {
        if (app_webrtc_) {
            ESP_LOGI(TAG, "AppWebrtc destroyed = %p", app_webrtc_.get());
            app_webrtc_ = nullptr;
        }
    }

    xSemaphoreGive(const_cast<SemaphoreHandle_t>(mutex_));
}