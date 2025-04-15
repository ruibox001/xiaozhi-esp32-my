#include "webrtc_manager.h"
#include "app_webrtc.h"

#define TAG "WebrtcManager"

WebrtcManager& WebrtcManager::instance() {
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
    webrtc_destroy();
    if (mutex_) {
        vSemaphoreDelete(mutex_);
    }
}

std::unique_ptr<AppWebrtc>& WebrtcManager::webrtc_get() {
    if (app_webrtc_) return app_webrtc_;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return app_webrtc_; // Return null unique_ptr reference
    }

    if (!app_webrtc_) {
        app_webrtc_ = std::make_unique<AppWebrtc>();
    }

    xSemaphoreGive(mutex_);
    ESP_LOGI(TAG, "webrtc instance %p - %p", this, app_webrtc_.get());
    return app_webrtc_;
}

void WebrtcManager::webrtc_destroy() {
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

bool WebrtcManager::webrtc_is_created() const {
    if (!mutex_) return false;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return false;
    }

    bool created = (app_webrtc_ != nullptr);
    xSemaphoreGive(const_cast<SemaphoreHandle_t>(mutex_));
    return created;
}