#include "webrtc_protocol.h"
#include "board.h"
#include "system_info.h"
#include "application.h"

#include <cstring>
#include <cJSON.h>
#include <esp_log.h>
#include <arpa/inet.h>
#include "assets/lang_config.h"

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
  
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  
}
  
void peer_connection_task(void *arg) {
  
    ESP_LOGI(TAG, "peer_connection_task started");
  
    for(;;) {
  
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          peer_connection_loop(g_pc);
          xSemaphoreGive(xSemaphore);
      }
  
      vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void peer_start_setup() {

    static char deviceid[32] = {0};

    PeerConfiguration config = {
        .ice_servers = {
            { .urls = "stun:stun.l.google.com:19302" }
        },
        .audio_codec = CODEC_OPUS,
        .datachannel = DATA_CHANNEL_BINARY,
    };

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    const char *mac_str = SystemInfo::GetMacAddress().c_str();
    // strcpy(deviceid, SystemInfo::GetMacAddress().c_str());
    strncpy(deviceid, mac_str, sizeof(mac_str));
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

    xTaskCreatePinnedToCore(peer_signaling_task, "peer_signaling", 8192, NULL, 6, &xPsTaskHandle, 1);

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "open https://sepfy.github.io/webrtc?deviceId=%s", deviceid);

}

WebrtcProtocol::WebrtcProtocol() {
    ESP_LOGI(TAG, "WebrtcProtocol started");
    xSemaphore = xSemaphoreCreateMutex();

    peer_start_setup();
}

WebrtcProtocol::~WebrtcProtocol() {
    
}

void WebrtcProtocol::Start() {
    peer_start_setup();
}

void WebrtcProtocol::SendAudio(const std::vector<uint8_t>& data) {
    
}

void WebrtcProtocol::SendText(const std::string& text) {
    
}

bool WebrtcProtocol::IsAudioChannelOpened() const {
    return true;
}

void WebrtcProtocol::CloseAudioChannel() {
    
}

bool WebrtcProtocol::OpenAudioChannel() {

    return true;
}

void WebrtcProtocol::ParseServerHello(const cJSON* root) {
    
}
