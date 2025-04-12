#ifndef _WEBRTC_PROTOCOL_H_
#define _WEBRTC_PROTOCOL_H_


#include "protocol.h"

#include <web_socket.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

class WebrtcProtocol : public Protocol {
public:
    WebrtcProtocol();
    ~WebrtcProtocol();

    void Start() override;
    void SendAudio(const std::vector<uint8_t>& data) override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel() override;
    bool IsAudioChannelOpened() const override;
    void SendStopListening() override;
    void SendAbortSpeaking(AbortReason reason) override;
    void SendIotStates(const std::string& states) override;

    bool webrtc_started_ = false;
    
private:

    void ParseServerHello(const cJSON* root);
    void SendText(const std::string& text) override;
};

#endif
