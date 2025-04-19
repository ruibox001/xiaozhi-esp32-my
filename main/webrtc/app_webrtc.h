#ifndef _APPWEBRTC_H_
#define _APPWEBRTC_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <string>
#include <list>

#include <opus_encoder.h>
#include <opus_decoder.h>
#include "peer.h"

class AppWebrtc {
public:

    AppWebrtc();
    ~AppWebrtc();

    void StartConnect(OpusEncoderWrapper* encoder, OpusDecoderWrapper* decoder, const char *mac);

    void StartAudio();
    void StopAudio();

    void SendAudioData(const std::vector<uint8_t>& data);
    void SendText(const std::string& text);
    void OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback);
    void OnPlayAudioData(std::function<void(std::vector<int16_t> pcm)> callback);
    void OnWebrtcStatusChange(std::function<void(int status)> callback);
    
    // webrtc运行状态
    bool webrtc_is_runing = false;
    void WebrtcReadAudioData(const std::vector<uint8_t>& audio_data);

    PeerConnectionState eState = PEER_CONNECTION_CLOSED;
    int gDataChannelOpened = 0;

    std::function<void(std::vector<uint8_t>&& data)> on_incoming_audio_;
    std::function<void(int status)> on_webrtc_status_change_;
    
    TaskHandle_t peer_signaling_task_handle_ = nullptr;
    
private:

    std::mutex audio_encode_mutex_;
    std::list<std::vector<uint8_t>> audio_encode_queue_;

    OpusEncoderWrapper* encoder_ptr_ = nullptr;  // 存储裸指针，不拥有所有权
    OpusDecoderWrapper* decoder_ptr_ = nullptr;

    bool WebrtcEncodeVoiceAndSend();

    void PeerConnectionTask();
    void PeerSignalingTask();

    TaskHandle_t peer_connection_task_handle_ = nullptr;

    PeerConnection* g_pc = nullptr;
    SemaphoreHandle_t xSemaphore = nullptr;

};

#endif // _APPWEBRTC_H_
