#include "talk_udp.h"
#include <cstring>
#include <esp_log.h>
#include <cJSON.h>

#include "board.h"
#include "system_info.h"

#define TAG "TalkUdp"


//下面是类成员函数

TalkUdp::TalkUdp() {
    udp_server_ = "192.168.1.115";
    // udp_server_ = "192.168.1.87";
    // udp_server_ = "192.168.1.29";
    udp_port_ = 8080;
}

TalkUdp::~TalkUdp() {
    if (udp_ != nullptr) {
        delete udp_;
    }
}

void TalkUdp::SendAudio(const std::vector<uint8_t>& data) {
    if (udp_ == nullptr) {
        return;
    }
    std::string encrypted(data.begin(), data.end());
    udp_->Send(encrypted);
}

void TalkUdp::SendText(const std::string & data) {
    if (udp_ == nullptr) {
        return;
    }
    udp_->Send(data);
}


void TalkUdp::CloseUdp() {
    if (udp_ != nullptr) {
        delete udp_;
        udp_ = nullptr;
    }
}

bool TalkUdp::OpenUdp() {
    if (udp_ != nullptr) {
        delete udp_;
    }
    udp_ = Board::GetInstance().CreateUdp();
    udp_->OnMessage([this](const std::string& data) {
        // ESP_LOGW(TAG, "UDP OnMessage = %d", data.size());
        if (on_incoming_audio_ != nullptr) {
            std::vector<uint8_t> temp(data.begin(), data.end());
            on_incoming_audio_(std::move(temp));
        }
    });

    bool state = udp_->Connect(udp_server_, udp_port_);
    on_status_change_(1);
    if (state == false) {
        ESP_LOGE(TAG, "udp is not connected");
        return false;
    }
    else {
        ESP_LOGW(TAG, "udp is connected");
        if (on_status_change_!= nullptr) {
            // on_status_change_(1);
        }
    }
    return true;
}


// 把接受的音频数据解码成PCM数据，放到队列中播放
void TalkUdp::OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback) {
    ESP_LOGI(TAG, "set OnIncomingAudioData = %p", this);
    on_incoming_audio_ = callback;
}

// 监听状态
void TalkUdp::OnStatusChange(std::function<void(int status)> callback) {
    ESP_LOGI(TAG, "set OnStatusChange = %p", this);
    on_status_change_ = callback;
}