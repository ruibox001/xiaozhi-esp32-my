#ifndef _TALKUDP_H_
#define _TALKUDP_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <udp.h>

#include <string>


class TalkUdp {
public:

    TalkUdp();
    ~TalkUdp();

    void SendAudio(const std::vector<uint8_t>& data);
    void SendText(const std::string & data);

    void CloseUdp();
    bool OpenUdp();

    void OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback);
    void OnStatusChange(std::function<void(int status)> callback);

    // webrtc运行状态
    bool is_runing = false;
    std::function<void(std::vector<uint8_t>&& data)> on_incoming_audio_;
    std::function<void(int status)> on_status_change_;
    
private:
    Udp* udp_ = nullptr;
    std::string udp_server_;
    int udp_port_;

};

#endif // _TALKUDP_H_
