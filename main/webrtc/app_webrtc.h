#ifndef _APPWEBRTC_H_
#define _APPWEBRTC_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <string>
#include <list>

#include <opus_encoder.h>
#include <opus_decoder.h>


class AppWebrtc {
public:

    AppWebrtc();
    ~AppWebrtc();

    void StartConnect(OpusEncoderWrapper* encoder, OpusDecoderWrapper* decoder);
    void StopConnect();

    void StartAudio();
    void StopAudio();

    void SendAudioData(const std::vector<uint8_t>& data);
    void OnIncomingAudioData(std::function<void(std::vector<uint8_t>&& data)> callback);
    void OnPlayAudioData(std::function<void(std::vector<int16_t> pcm)> callback);
    
    // webrtc运行状态
    bool webrtc_is_runing = false;
    void WebrtcReadAudioData(std::vector<int16_t>&& audio_data);
    
private:

    std::function<void(std::vector<uint8_t>&& data)> on_incoming_audio_;
    std::function<void(std::vector<int16_t> pcm)> on_play_audio_;

    std::mutex audio_encode_mutex_;
    std::list<std::vector<int16_t>> audio_encode_queue_;

    OpusEncoderWrapper* encoder_ptr_ = nullptr;  // 存储裸指针，不拥有所有权
    OpusDecoderWrapper* decoder_ptr_ = nullptr;

    bool WebrtcEncodeVoiceAndSend();
    void WebrtcDecodePVoiceAndPlay(uint8_t *data, size_t size);

};

#endif // _APPWEBRTC_H_
