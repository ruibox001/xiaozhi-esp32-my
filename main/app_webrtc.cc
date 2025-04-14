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
    
}

void AppWebrtc::StartConnect() {
    
}

void AppWebrtc::StartAudio() {
    
}

void AppWebrtc::StopAudio() {
    
}

// 把接受的音频数据解码成PCM数据，放到队列中播放
void AppWebrtc::IncomingAudio(uint8_t *data, size_t size) {
    
}
