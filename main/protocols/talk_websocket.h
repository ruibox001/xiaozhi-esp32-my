#ifndef _TALKWEBSOCKET_H_
#define _TALKWEBSOCKET_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <web_socket.h>

#include <string>
#include <list>

#include <opus_encoder.h>
#include <opus_decoder.h>

class TalkWebsocket {
public:

    TalkWebsocket();
    ~TalkWebsocket();

    void SendAudio(const std::vector<uint8_t>& data);
    void SendText(const std::string& text);

    void CloseWebsocket();
    bool OpenWebsocket();

    void OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback);
    void OnStatusChange(std::function<void(int status)> callback);

    // webrtc运行状态
    bool is_runing = false;
    std::function<void(std::vector<uint8_t>&& data)> on_incoming_audio_;
    std::function<void(int status)> on_status_change_;
    
private:
    WebSocket* websocket_ = nullptr;

};

#endif // _TALKWEBSOCKET_H_
