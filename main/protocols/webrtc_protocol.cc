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


WebrtcProtocol::WebrtcProtocol() {
    
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
