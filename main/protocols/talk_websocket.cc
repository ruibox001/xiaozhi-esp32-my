#include "talk_websocket.h"
#include <cstring>
#include <esp_log.h>
#include <cJSON.h>

#include "board.h"
#include "system_info.h"

#define TAG "TalkWebsocket"
#define WEBSOCKET_URL "ws://192.168.1.100:8080/ws"
#define WEBSOCKET_ACCESS_TOKEN "your_access_token"


//下面是类成员函数

TalkWebsocket::TalkWebsocket() {
    
}

TalkWebsocket::~TalkWebsocket() {
    if (websocket_ != nullptr) {
        delete websocket_;
    }
}

void TalkWebsocket::SendAudio(const std::vector<uint8_t>& data) {
    if (websocket_ == nullptr) {
        return;
    }

    websocket_->Send(data.data(), data.size(), true);
}

void TalkWebsocket::SendText(const std::string& text) {
    if (websocket_ == nullptr) {
        return;
    }

    if (!websocket_->Send(text)) {
        ESP_LOGE(TAG, "Failed to send text: %s", text.c_str());
    }
}


void TalkWebsocket::CloseWebsocket() {
    if (websocket_ != nullptr) {
        websocket_->Close();
        delete websocket_;
        websocket_ = nullptr;
    }
}

bool TalkWebsocket::OpenWebsocket() {
    if (websocket_ != nullptr) {
        delete websocket_;
    }

    std::string url = WEBSOCKET_URL;
    std::string token = "Bearer " + std::string(WEBSOCKET_ACCESS_TOKEN);
    websocket_ = Board::GetInstance().CreateWebSocket();
    websocket_->SetHeader("Authorization", token.c_str());
    websocket_->SetHeader("Protocol-Version", "1");
    websocket_->SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());

    websocket_->OnData([this](const char* data, size_t len, bool binary) {
        if (binary) {
            if (on_incoming_audio_ != nullptr) {
                on_incoming_audio_(std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + len));
            }
        } else {
            // Parse JSON data
            auto root = cJSON_Parse(data);
            // auto type = cJSON_GetObjectItem(root, "type");
            // if (type != NULL) {
            //     if (strcmp(type->valuestring, "hello") == 0) {
            //         ParseServerHello(root);
            //     } else {
            //         if (on_incoming_json_ != nullptr) {
            //             on_incoming_json_(root);
            //         }
            //     }
            // } else {
            //     ESP_LOGE(TAG, "Missing message type, data: %s", data);
            // }
            cJSON_Delete(root);
        }
    });

    websocket_->OnDisconnected([this]() {
        ESP_LOGI(TAG, "Websocket disconnected");
    });

    if (!websocket_->Connect(url.c_str())) {
        ESP_LOGE(TAG, "Failed to connect to websocket server");
        return false;
    }

    return true;
}


// 把接受的音频数据解码成PCM数据，放到队列中播放
void TalkWebsocket::OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback) {
    ESP_LOGI(TAG, "set OnIncomingAudioData = %p", this);
    on_incoming_audio_ = callback;
}

// 监听状态
void TalkWebsocket::OnStatusChange(std::function<void(int status)> callback) {
    ESP_LOGI(TAG, "set OnStatusChange = %p", this);
    on_status_change_ = callback;
}