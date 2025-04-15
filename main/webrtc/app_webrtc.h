#ifndef _APPWEBRTC_H_
#define _APPWEBRTC_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <string>
#include <list>

#include <opus_encoder.h>
#include <opus_decoder.h>
#include <opus_resampler.h>


class AppWebrtc {
public:

    AppWebrtc();
    ~AppWebrtc();

    void StartConnect();
    void StartAudio();
    void StopAudio();
    void SendAudioData(const std::vector<uint8_t>& data);
    void OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback);
    
    // webrtc运行状态
    bool webrtc_is_runing = false;
    
private:
    std::function<void(std::vector<uint8_t>&& data)> on_incoming_audio_;

};

#endif // _APPWEBRTC_H_
