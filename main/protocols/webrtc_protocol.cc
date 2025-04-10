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


static void oniceconnectionstatechange(PeerConnectionState state, void *user_data) {
    ESP_LOGI(TAG, "PeerConnectionState: %d", state);
    eState = state;
    // not support datachannel close event
    if (eState != PEER_CONNECTION_COMPLETED) {
      gDataChannelOpened = 0;
    }
}
  
static void onmessage(char *msg, size_t len, void *userdata, uint16_t sid) {
    ESP_LOGI(TAG, "Datachannel message: %.*s", len, msg);
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
    }
}
  
void onopen(void *userdata) {
    ESP_LOGI(TAG, "Datachannel opened");
    gDataChannelOpened = 1;
}
  
static void onclose(void *userdata) {
   
}
  
void peer_signaling_task(void *arg) {
  
    ESP_LOGI(TAG, "peer_signaling_task started");
    for(;;) {
      peer_signaling_loop();
      vTaskDelay(pdMS_TO_TICKS(100));
    }
}
  
void peer_connection_task(void *arg) {
  
    ESP_LOGI(TAG, "peer_connection_task started");
    for(;;) {
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          peer_connection_loop(g_pc);
          xSemaphoreGive(xSemaphore);
      }
      vTaskDelay(pdMS_TO_TICKS(100));
    }
}

WebrtcProtocol::WebrtcProtocol() {
    ESP_LOGI(TAG, "WebrtcProtocol started ---------------------- ");
    xSemaphore = xSemaphoreCreateMutex();
}

WebrtcProtocol::~WebrtcProtocol() {
    ESP_LOGI(TAG, "WebrtcProtocol destroyed ---------------------- ");
    peer_signaling_leave_channel();
    peer_connection_close(g_pc);
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
}

void WebrtcProtocol::Start() {
    //开始初始化webrtc
    static char deviceid[32] = {0};

    PeerConfiguration config = {
        .ice_servers = {
            { .urls = "stun:stun.l.google.com:19302" }
        },
        .audio_codec = CODEC_OPUS,
        .datachannel = DATA_CHANNEL_STRING,
    };
    config.onaudiotrack = on_audio_track_callback;
    // 2. 传递 this 指针，以便回调时能访问对象
    config.user_data = this;

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    strcpy(deviceid, SystemInfo::GetMacAddress().c_str());
    ESP_LOGI(TAG, "Device ID: %s", deviceid);

    xSemaphore = xSemaphoreCreateMutex();

    peer_init();

//   camera_init();

// #if defined(CONFIG_ESP32S3_XIAO_SENSE)
//   audio_init();
// #endif

    g_pc = peer_connection_create(&config);
    peer_connection_oniceconnectionstatechange(g_pc, oniceconnectionstatechange);
    peer_connection_ondatachannel(g_pc, onmessage, onopen, onclose);

    ServiceConfiguration service_config = SERVICE_CONFIG_DEFAULT();
    service_config.client_id = deviceid;
    service_config.pc = g_pc;
    service_config.mqtt_url = "broker.emqx.io";
    peer_signaling_set_config(&service_config);
    peer_signaling_join_channel();

#if defined(CONFIG_ESP32S3_XIAO_SENSE)
    // xTaskCreatePinnedToCore(audio_task, "audio", 8192, NULL, 7, &xAudioTaskHandle, 0);
#endif

    // xTaskCreatePinnedToCore(camera_task, "camera", 4096, NULL, 8, &xCameraTaskHandle, 1);

    xTaskCreatePinnedToCore(peer_connection_task, "peer_connection", 8192, NULL, 5, &xPcTaskHandle, 1);

    xTaskCreatePinnedToCore(peer_signaling_task, "peer_signaling", 8192, NULL, 6, &xPsTaskHandle, 0);

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "open https://sepfy.github.io/webrtc?deviceId=%s", deviceid);
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
    return gDataChannelOpened;
}

void WebrtcProtocol::CloseAudioChannel() {
    
}

bool WebrtcProtocol::OpenAudioChannel() {
    return (eState == PEER_CONNECTION_COMPLETED);
}

void WebrtcProtocol::ParseServerHello(const cJSON* root) {
    
}
