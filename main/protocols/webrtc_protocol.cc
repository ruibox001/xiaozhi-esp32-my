#include "webrtc_protocol.h"
#include "board.h"
#include "system_info.h"
#include "application.h"

#include <cstring>
#include <cJSON.h>
#include <esp_log.h>
#include <arpa/inet.h>
#include "assets/lang_config.h"
#include "peer.h"

#define TAG "WEBRTC"

static TaskHandle_t xPcTaskHandle = nullptr;
static TaskHandle_t xPsTaskHandle = nullptr;
SemaphoreHandle_t xSemaphore = nullptr;

PeerConnection* g_pc;
PeerConnectionState eState = PEER_CONNECTION_CLOSED;
int gDataChannelOpened = 0;

static void peer_connection_task(void *arg);
static void peer_signaling_task(void *arg);

static void oniceconnectionstatechange(PeerConnectionState state, void *userdata) {
    ESP_LOGI(TAG, "PeerConnectionState: %d", state);
    eState = state;

    if (eState != PEER_CONNECTION_COMPLETED) {
      gDataChannelOpened = 0;
    }

    if (eState == PEER_CONNECTION_DISCONNECTED || eState == PEER_CONNECTION_FAILED || eState == PEER_CONNECTION_CLOSED)
    {
        // WebrtcProtocol* self = static_cast<WebrtcProtocol*>(userdata);
        // if (self && self->on_audio_channel_closed_ != nullptr) {
        //     self->on_audio_channel_closed_();
        // }
        return;
    }
    
    // if (eState == PEER_CONNECTION_COMPLETED)
    // {
    //     WebrtcProtocol* self = static_cast<WebrtcProtocol*>(userdata);
    //     if (self && self->on_audio_channel_opened_ != nullptr) {
    //         self->on_audio_channel_opened_();
    //     }
    //     return;
    // }
    
}
  
static void onmessage(char *msg, size_t len, void *userdata, uint16_t sid) {
    ESP_LOGI(TAG, "Datachannel message: %.*s", len, msg);
    WebrtcProtocol* self = static_cast<WebrtcProtocol*>(userdata);
    if (self) {
        self->last_incoming_time_ = std::chrono::steady_clock::now();
    }
}

// Define the callback function to handle incoming audio data
void on_audio_track_callback(uint8_t *data, size_t size, void *userdata) {
    // Here you can process the incoming audio data
    // For demonstration, we'll just print the size of the data received
    printf("Received audio data of size: %zu\n", size);

    // 3. 从 user_data 获取对象实例
    WebrtcProtocol* self = static_cast<WebrtcProtocol*>(userdata);
    if (self) {
        // 4. 调用真正的成员函数
        self->on_incoming_audio_(std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + size));
        self->last_incoming_time_ = std::chrono::steady_clock::now();
    }
}
  
void onopen(void *userdata) {
    ESP_LOGI(TAG, "Datachannel opened");
    gDataChannelOpened = 1;
    WebrtcProtocol* self = static_cast<WebrtcProtocol*>(userdata);
    if (self && self->on_audio_channel_opened_ != nullptr) {
        self->on_audio_channel_opened_();
    }
    if (xPsTaskHandle != nullptr) {
        vTaskDelete(xPsTaskHandle);     // 删除指定任务
        xPsTaskHandle = nullptr;        // 清除句柄，避免重复删除
    }
}
  
static void onclose(void *userdata) {
    ESP_LOGI(TAG, "Datachannel closed");
    gDataChannelOpened = 0;
    WebrtcProtocol* self = static_cast<WebrtcProtocol*>(userdata);
    if (self && self->on_audio_channel_closed_ != nullptr) {
        self->on_audio_channel_closed_();
    }
}
  
static void peer_signaling_task(void *arg) {
  
    ESP_LOGI(TAG, "peer_signaling_task started");
    while (g_pc) {
      peer_signaling_loop();
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "peer_signaling_task ended");
    vTaskDelete(nullptr);
}
  
static void peer_connection_task(void *arg) {

    ESP_LOGI(TAG, "peer_connection_task started");
    while(g_pc) {
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          peer_connection_loop(g_pc);
          xSemaphoreGive(xSemaphore);
      }
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI(TAG, "peer_connection_task ended");
    vTaskDelete(nullptr);
}


void stop_webrtc(WebrtcProtocol* self) {
    if (self->webrtc_started_ == false) {
        return;
    }
    ESP_LOGI(TAG, "stop_webrtc ---------------------- ");
    self->webrtc_started_ = false;
    if (self && self->on_audio_channel_closed_ != nullptr) {
        self->on_audio_channel_closed_();
    }
    peer_signaling_leave_channel();
    peer_connection_close(g_pc);
    eState = PEER_CONNECTION_CLOSED;
    if (xPsTaskHandle != nullptr) {
        vTaskDelete(xPsTaskHandle);     // 删除指定任务
        xPsTaskHandle = nullptr;        // 清除句柄，避免重复删除
    }
    if (xPcTaskHandle != nullptr) {
        vTaskDelete(xPcTaskHandle);     // 删除指定任务
        xPcTaskHandle = nullptr;        // 清除句柄，避免重复删除
    }
    if (xSemaphore != nullptr) {
        vSemaphoreDelete(xSemaphore);   // 销毁 Mutex
        xSemaphore = nullptr;           // 将指针设为 NULL，避免误用
    }
    peer_connection_destroy(g_pc);
    g_pc = nullptr;
}

void start_webrtc(WebrtcProtocol* self) {
    if (self->webrtc_started_) {
        stop_webrtc(self);
        return;
    }
    ESP_LOGI(TAG, "start_webrtc ---------------------- ");
    //开始初始化webrtc
    static char deviceid[32] = {0};
    self->error_occurred_ = false;

    PeerConfiguration config = {
        .ice_servers = {
            { .urls = "stun:stun.l.google.com:19302" }
        },
        .audio_codec = CODEC_OPUS,
        .datachannel = DATA_CHANNEL_STRING,
    };
    config.onaudiotrack = on_audio_track_callback;
    // 2. 传递 this 指针，以便回调时能访问对象
    config.user_data = self;

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    strcpy(deviceid, SystemInfo::GetMacAddress().c_str());
    ESP_LOGI(TAG, "Device ID: %s", deviceid);

    peer_init();

    g_pc = peer_connection_create(&config);
    peer_connection_oniceconnectionstatechange(g_pc, oniceconnectionstatechange);
    peer_connection_ondatachannel(g_pc, onmessage, onopen, onclose);

    ServiceConfiguration service_config = SERVICE_CONFIG_DEFAULT();
    service_config.client_id = deviceid;
    service_config.pc = g_pc;
    service_config.mqtt_url = CONFIG_WEBRTC_URL;
    peer_signaling_set_config(&service_config);
    peer_signaling_join_channel();

    xTaskCreatePinnedToCore(peer_connection_task, "peer_connection", 8192, NULL, 5, &xPcTaskHandle, 1);

    xTaskCreatePinnedToCore(peer_signaling_task, "peer_signaling", 8192, NULL, 6, &xPsTaskHandle, 0);

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "open https://sepfy.github.io/webrtc?deviceId=%s", deviceid);
    self->webrtc_started_ = true;
}


WebrtcProtocol::WebrtcProtocol() {
    ESP_LOGI(TAG, "WebrtcProtocol started ---------------------- ");
    xSemaphore = xSemaphoreCreateMutex();
}

WebrtcProtocol::~WebrtcProtocol() {
    ESP_LOGI(TAG, "WebrtcProtocol stopped ---------------------- ");
    stop_webrtc(this);
}

void WebrtcProtocol::Start() {
    
}


void WebrtcProtocol::SendAudio(const std::vector<uint8_t>& data) {
    if (gDataChannelOpened == 0) {
        return;
    }
    peer_connection_send_audio(g_pc, data.data(), data.size());
}

void WebrtcProtocol::SendText(const std::string& text) {
    if (gDataChannelOpened == 0) {
        return;
    }
    peer_connection_datachannel_send(g_pc, const_cast<char*>(text.c_str()), text.length());
}

bool WebrtcProtocol::IsAudioChannelOpened() const {
    return gDataChannelOpened != 0 && eState == PEER_CONNECTION_COMPLETED && !error_occurred_;
}

void WebrtcProtocol::CloseAudioChannel() {
    stop_webrtc(this);
}

bool WebrtcProtocol::OpenAudioChannel() {
    start_webrtc(this);
    return true;
}

void WebrtcProtocol::SendStopListening() {
    ESP_LOGI(TAG, "WebrtcProtocol SendStopListening ********");
}

void WebrtcProtocol::SendAbortSpeaking(AbortReason reason) {
    ESP_LOGI(TAG, "WebrtcProtocol SendAbortSpeaking ********");
}

void WebrtcProtocol::SendIotStates(const std::string& states) {

}

void WebrtcProtocol::ParseServerHello(const cJSON* root) {
    ESP_LOGI(TAG, "WebrtcProtocol ParseServerHello ********");
}
