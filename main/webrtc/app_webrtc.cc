#include "app_webrtc.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>
#include <esp_app_desc.h>

#define TAG "AppWebrtc"


void oniceconnectionstatechange(PeerConnectionState state, void *userdata) {
    ESP_LOGW(TAG, "PeerConnection State = %d", state);
    AppWebrtc* self = static_cast<AppWebrtc*>(userdata);
    self->eState = state;
    if (state != PEER_CONNECTION_COMPLETED) {
        self->gDataChannelOpened = 0;
    }
    if (self->on_webrtc_status_change_){
        self->on_webrtc_status_change_(state);
    }
}
  
void onmessage(char *msg, size_t len, void *userdata, uint16_t sid) {
    ESP_LOGI(TAG, "Datachannel message: %.*s", len, msg);
}

// Define the callback function to handle incoming audio data
void on_audio_track_callback(uint8_t *data, size_t size, void *userdata) {
    // Here you can process the incoming audio data
    // For demonstration, we'll just print the size of the data received
    // ESP_LOGI(TAG, "Received audio data of size: %zu\n", size);
    AppWebrtc* self = static_cast<AppWebrtc*>(userdata);
    if (self && data) {
        // 4. 调用真正的成员函数
        self->on_incoming_audio_(std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + size));
    }
}
  
void onopen(void *userdata) {
    ESP_LOGI(TAG, "Datachannel opened");
    AppWebrtc* self = static_cast<AppWebrtc*>(userdata);
    self->gDataChannelOpened = 1;
    
    // 网络连接后移除信令服务器
    if (self->peer_signaling_task_handle_ != nullptr) {
        vTaskDelete(self->peer_signaling_task_handle_);     // 删除指定任务
        self->peer_signaling_task_handle_ = nullptr;        // 清除句柄，避免重复删除
    }
}
  
void onclose(void *userdata) {
    ESP_LOGI(TAG, "Datachannel closed");
}

//下面是类成员函数

AppWebrtc::AppWebrtc() {
    ESP_LOGW(TAG, "AppWebrtc --- create %p", this);
    xSemaphore = xSemaphoreCreateMutex();
}

AppWebrtc::~AppWebrtc() {
    ESP_LOGW(TAG, "AppWebrtc --- destroy %p", this);

    if (g_pc) {
        g_pc = nullptr;
    }

    if (encoder_ptr_) {
        encoder_ptr_ = nullptr;
    }
    if (decoder_ptr_) {
        decoder_ptr_ = nullptr;
    }

    if (on_play_audio_) {
        on_play_audio_ = nullptr;
    }

    if (on_incoming_audio_){
        on_incoming_audio_ = nullptr;
    }
}

void AppWebrtc::StartConnect(OpusEncoderWrapper* encoder, OpusDecoderWrapper* decoder, const char *mac) {
    if (encoder_ptr_){
        return;
    }

    encoder_ptr_ = encoder;
    decoder_ptr_ = decoder;
    ESP_LOGI(TAG, "StartConnect %p", this);

    //开始初始化webrtc
    static char deviceid[32] = {0};

    PeerConfiguration config = {
        .ice_servers = {
            { .urls = "stun:stun.l.google.com:19302" }
        },
        .audio_codec = CODEC_OPUS,
        .datachannel = DATA_CHANNEL_BINARY,
    };
    config.onaudiotrack = on_audio_track_callback;
    // 2. 传递 this 指针，以便回调时能访问对象
    config.user_data = this;

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    strcpy(deviceid, mac);
    ESP_LOGI(TAG, "Device ID: %s", deviceid);

    peer_init();

    g_pc = peer_connection_create(&config);
    peer_connection_oniceconnectionstatechange(g_pc, oniceconnectionstatechange);
    peer_connection_ondatachannel(g_pc, onmessage, onopen, onclose);

    ServiceConfiguration service_config = SERVICE_CONFIG_DEFAULT();
    service_config.client_id = deviceid;
    service_config.pc = g_pc;
    service_config.mqtt_url = "broker.emqx.io";
    peer_signaling_set_config(&service_config);
    peer_signaling_join_channel();

    // if (peer_connection_task_stack_ == nullptr) {
    //     peer_connection_task_stack_ = (StackType_t*)heap_caps_malloc(4096 * 6, MALLOC_CAP_SPIRAM);
    // }
    // peer_connection_task_handle_ = xTaskCreateStatic([](void* arg) {
    //     auto this_ = (AppWebrtc*)arg;
    //     this_->PeerConnectionTask();
    //     vTaskDelete(NULL);
    // }, "peer_connection", 4096 * 6, this, 15, peer_connection_task_stack_, &peer_connection_task_buffer_);

    // if (peer_signaling_task_stack_ == nullptr) {
    //     peer_signaling_task_stack_ = (StackType_t*)heap_caps_malloc(4096 * 8, MALLOC_CAP_SPIRAM);
    // }
    // peer_signaling_task_handle_ = xTaskCreateStatic([](void* arg) {
    //     auto this_ = (AppWebrtc*)arg;
    //     this_->PeerSignalingTask();
    //     vTaskDelete(NULL);
    // }, "peer_signaling", 4096 * 8, this, 6, peer_signaling_task_stack_, &peer_signaling_task_buffer_);


    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "open https://sepfy.github.io/webrtc?deviceId=%s", deviceid);

    xTaskCreatePinnedToCore([](void* arg) {
        AppWebrtc* appwebrtc = (AppWebrtc*)arg;
        appwebrtc->PeerConnectionTask();
        vTaskDelete(NULL);
    }, "peer_connection", 4096 * 6, this, 10, &peer_connection_task_handle_, 0);

    xTaskCreatePinnedToCore([](void* arg) {
        AppWebrtc* appwebrtc = (AppWebrtc*)arg;
        appwebrtc->PeerSignalingTask();
        vTaskDelete(NULL);
    }, "peer_signaling", 4096 * 2, this, 6, &peer_signaling_task_handle_, 1);
}

void AppWebrtc::PeerConnectionTask() {
    ESP_LOGW(TAG, "PeerConnectionTask started");
    while (true) {
        // ESP_LOGW(TAG, "PeerConnectionTask runing ");
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
            if(!WebrtcEncodeVoiceAndSend()) {
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            peer_connection_loop(g_pc);
            xSemaphoreGive(xSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(6));
    }
}

void AppWebrtc::PeerSignalingTask() {
    ESP_LOGW(TAG, "PeerSignalingTask started");
    while (true) {
        // ESP_LOGW(TAG, "PeerSignalingTask runing ");
        peer_signaling_loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void AppWebrtc::StartAudio() {
    if (webrtc_is_runing) {
        return;
    }

    ESP_LOGI(TAG, "StartAudio %p", this);
    std::lock_guard<std::mutex> lock(audio_encode_mutex_);
    audio_encode_queue_.clear();
    webrtc_is_runing = true;

}

void AppWebrtc::StopAudio() {
    if (!webrtc_is_runing) {
        return;
    }

    webrtc_is_runing = false;
    ESP_LOGI(TAG, "StopAudio %p", this);

    peer_signaling_leave_channel();
    if (g_pc) {
        peer_connection_close(g_pc);
    }

    if (peer_connection_task_handle_ != nullptr) {
        vTaskDelete(peer_connection_task_handle_);     // 删除指定任务
        peer_connection_task_handle_ = nullptr;        // 清除句柄，避免重复删除
    }

    if (peer_signaling_task_handle_ != nullptr) {
        vTaskDelete(peer_signaling_task_handle_);     // 删除指定任务
        peer_signaling_task_handle_ = nullptr;        // 清除句柄，避免重复删除
    }

    if (xSemaphore) {
        vSemaphoreDelete(xSemaphore);   // 销毁 Mutex
        xSemaphore = nullptr;           // 将指针设为 NULL，避免误用
    }
   
    std::lock_guard<std::mutex> lock(audio_encode_mutex_);
    audio_encode_queue_.clear();

    if (g_pc) {
        peer_connection_destroy(g_pc);
        g_pc = nullptr;
    }
}

void AppWebrtc::SendAudioData(const std::vector<uint8_t>& data) {
    if (gDataChannelOpened == 0 || data.empty()) {
        return;
    }
    peer_connection_send_audio(g_pc, data.data(), data.size());
}

void AppWebrtc::SendText(const std::string& text) {
    if (gDataChannelOpened == 0) {
        return;
    }
    peer_connection_datachannel_send(g_pc, const_cast<char*>(text.c_str()), text.length());
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

// 监听状态
void AppWebrtc::OnWebrtcStatusChange(std::function<void(int status)> callback) {
    ESP_LOGI(TAG, "set OnWebrtcStatusChange = %p", this);
    on_webrtc_status_change_ = callback;
}

void AppWebrtc::WebrtcReadAudioData(const std::vector<uint8_t>& audio_data) {
    std::lock_guard<std::mutex> lock(audio_encode_mutex_);
    audio_encode_queue_.emplace_back(audio_data);
    // ESP_LOGI(TAG, "WebrtcReadAudioData = %d", audio_encode_queue_.size());
}

bool AppWebrtc::WebrtcEncodeVoiceAndSend(){

    if (!webrtc_is_runing) {
        return false;
    }

    std::unique_lock<std::mutex> lock(audio_encode_mutex_);
    if (audio_encode_queue_.empty()) {
        lock.unlock();
        return true;
    }

    // ESP_LOGW(TAG, "WebrtcEncodeVoiceAndSend - %d", audio_encode_queue_.size());
    auto opus = std::move(audio_encode_queue_.front());
    audio_encode_queue_.pop_front();
    SendAudioData(opus);
    lock.unlock();
    
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