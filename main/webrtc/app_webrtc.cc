#include "app_webrtc.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>
#include <esp_app_desc.h>

#include "board.h"
#include "display.h"
#include "system_info.h"
#include "audio_codec.h"
#include "assets/lang_config.h"

// #include "peer.h"

#define TAG "AppWebrtc"


//下面是类成员函数

AppWebrtc::AppWebrtc() {
    
}

AppWebrtc::~AppWebrtc() {
    on_incoming_audio_ = nullptr;
}

void AppWebrtc::StartConnect() {
    
}

void AppWebrtc::StartAudio() {
    ESP_LOGI(TAG, "StartAudio %p", this);
}

void AppWebrtc::StopAudio() {
    ESP_LOGI(TAG, "StopAudio %p", this);
}

void AppWebrtc::WebrtcSendAudioData(const std::vector<uint8_t>& data) {
    // if (gDataChannelOpened == 0) {
    //     return;
    // }
    // peer_connection_send_audio(g_pc, data.data(), data.size());
}

// 把接受的音频数据解码成PCM数据，放到队列中播放
void AppWebrtc::OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback) {
    ESP_LOGI(TAG, "set OnIncomingAudioData %p - %p", this, callback);
    on_incoming_audio_ = callback;
}
