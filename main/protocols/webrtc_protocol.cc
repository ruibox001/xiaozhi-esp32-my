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
SemaphoreHandle_t xSemaphore = NULL;

PeerConnection* g_pc;
PeerConnectionState eState = PEER_CONNECTION_CLOSED;
int gDataChannelOpened = 0;

static void oniceconnectionstatechange(PeerConnectionState state, void* user_data) {
    ESP_LOGI(TAG, "PeerConnectionState: %d", state);
    eState = state;
    // not support datachannel close event
    if (eState != PEER_CONNECTION_COMPLETED) {
      gDataChannelOpened = 0;
    }
}
  
static void onmessage(char* msg, size_t len, void* userdata, uint16_t sid) {
    ESP_LOGI(TAG, "Datachannel message: %.*s", len, msg);
}
  
void onopen(void* userdata) {
    ESP_LOGI(TAG, "Datachannel opened");
    gDataChannelOpened = 1;
}
  
static void onclose(void* userdata) {

}
  
void peer_connection_task(void* arg) {
    ESP_LOGI(TAG, "peer_connection_task started");
  
    for (;;) {
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
        peer_connection_loop(g_pc);
        xSemaphoreGive(xSemaphore);
      }
  
      vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void peer_start_setup() {

    PeerConfiguration config = {
        .ice_servers = {
            {.urls = "stun:stun.l.google.com:19302"}},
        .audio_codec = CODEC_PCMA,
        .datachannel = DATA_CHANNEL_BINARY,
    };

    peer_init();

    g_pc = peer_connection_create(&config);
    peer_connection_oniceconnectionstatechange(g_pc, oniceconnectionstatechange);
    peer_connection_ondatachannel(g_pc, onmessage, onopen, onclose);
    // peer_signaling_connect(CONFIG_WEBRTC_URL, CONFIG_WEBRTC_ACCESS_TOKEN, g_pc);

}

WebrtcProtocol::WebrtcProtocol() {
    ESP_LOGI(TAG, "WebrtcProtocol started");
    xSemaphore = xSemaphoreCreateMutex();

    peer_start_setup();
}

WebrtcProtocol::~WebrtcProtocol() {
    
}

void WebrtcProtocol::Start() {
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
