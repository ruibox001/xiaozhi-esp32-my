#include "app_webrtc.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>
#include <esp_app_desc.h>

// #include "peer.h"

#define TAG "AppWebrtc"


//下面是类成员函数

AppWebrtc::AppWebrtc() {
    ESP_LOGW(TAG, "AppWebrtc --- create %p", this);
}

AppWebrtc::~AppWebrtc() {
    ESP_LOGW(TAG, "AppWebrtc --- destroy %p", this);
    on_incoming_audio_ = nullptr;
}

void AppWebrtc::StartConnect(OpusEncoderWrapper* encoder, OpusDecoderWrapper* decoder) {
    encoder_ptr_ = encoder;
    decoder_ptr_ = decoder;
    ESP_LOGI(TAG, "StartConnect %p", this);
}

void AppWebrtc::StopConnect() {
    ESP_LOGI(TAG, "StopConnect %p", this);
}

void AppWebrtc::StartAudio() {
    if (webrtc_is_runing) {
        return;
    }
    webrtc_is_runing = true;

    ESP_LOGI(TAG, "StartAudio %p", this);
    std::lock_guard<std::mutex> lock(audio_encode_mutex_);
    audio_encode_queue_.clear();
}

void AppWebrtc::StopAudio() {
    if (!webrtc_is_runing) {
        return;
    }
    webrtc_is_runing = false;
    ESP_LOGI(TAG, "StopAudio %p", this);
}

void AppWebrtc::SendAudioData(const std::vector<uint8_t>& data) {
    // if (gDataChannelOpened == 0) {
    //     return;
    // }
    // peer_connection_send_audio(g_pc, data.data(), data.size());
    ESP_LOGI(TAG, "SendAudioData %p", this);
}

// 把接受的音频数据解码成PCM数据，放到队列中播放
void AppWebrtc::OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback) {
    ESP_LOGI(TAG, "set OnIncomingAudioData = %p", this);
    on_incoming_audio_ = callback;
}

// 播放pcm文件
void AppWebrtc::OnPlayAudioData(std::function<void(std::vector<int16_t> pcm)> callback) {
    ESP_LOGI(TAG, "set OnIncomingAudioData = %p", this);
    on_play_audio_ = callback;
}

void AppWebrtc::WebrtcReadAudioData(std::vector<int16_t>&& audio_data) {
    std::lock_guard<std::mutex> lock(audio_encode_mutex_);
    audio_encode_queue_.emplace_back(std::move(audio_data));
}

bool AppWebrtc::WebrtcEncodeVoiceAndSend(){

    ESP_LOGW(TAG, "webrtc send is create = 00 - %d", webrtc_is_runing);
    if (!webrtc_is_runing) {
        return false;
    }

    ESP_LOGW(TAG, "webrtc send is running = 11");
    std::unique_lock<std::mutex> lock(audio_encode_mutex_);
    if (audio_encode_queue_.empty()) {
        lock.unlock();
        return true;
    }

    auto pcm = std::move(audio_encode_queue_.front());
    audio_encode_queue_.pop_front();
    lock.unlock();

    if (encoder_ptr_){
        encoder_ptr_->Encode(std::move(pcm), [this](std::vector<uint8_t>&& opus) {
            SendAudioData(opus);
        });
    }
    
    return true;
}

void AppWebrtc::WebrtcDecodePVoiceAndPlay(uint8_t *data, size_t size) {
    // auto codec = Board::GetInstance().GetAudioCodec();

    // 1. 将 data 转换为 std::vector<uint8_t>
    std::vector<uint8_t> opus_data(data, data + size);

    // 2. 准备接收 PCM 数据的 vector
    std::vector<int16_t> pcm_data;

    if (decoder_ptr_ && on_play_audio_){
        // 3. 调用解码函数
        if (decoder_ptr_->Decode(std::move(opus_data), pcm_data)) {
            // 解码成功，pcm_data 现在包含解码后的音频数据
            // 可以在这里处理 PCM 数据，比如播放或传递给其他模块
            on_play_audio_(pcm_data);
        }
    }
}