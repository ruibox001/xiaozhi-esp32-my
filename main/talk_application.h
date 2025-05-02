#ifndef _TALKAPPLICATION_H_
#define _TALKAPPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <string>
#include <list>

#include <opus_encoder.h>
#include <opus_decoder.h>
#include <opus_resampler.h>

#include "audio_codec.h"

#include "background_task.h"
// #include "wake_word_detect.h"
#include "audio_processor.h"
#include "protocols/talk_udp.h"

// #define SCHEDULE_EVENT (1 << 0)
#define OPUS_ONE_FRAME_DURATION_MS 20

class TalkApplication {
public:
    static TalkApplication& GetInstance() {
        static TalkApplication instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    TalkApplication(const TalkApplication&) = delete;
    TalkApplication& operator=(const TalkApplication&) = delete;

    void StartTalk();
    // void Schedule(std::function<void()> callback);

    // void MainLoop();
    // void OnAudioInput();
    void OnAudioOutput(AudioCodec* codec);
    void OnAudioDecodeOnMainOutput(AudioCodec* codec);
    // void WebrtcIncomingAudio(uint8_t *data, size_t size);
    // void StartAudio();
    // void StopAudio();
    // void SetDecodeSampleRate(int sample_rate, int frame_duration);
    // void ReadAudio(std::vector<int16_t>& data, int sample_rate, int samples);
    // void AudioLoop();
    
    // bool webrtc_started_ = false;
    void ButtonPressedDown();
    
private:
    TalkApplication();
    ~TalkApplication();

    std::unique_ptr<OpusEncoderWrapper> opus_encoder_;
    std::unique_ptr<OpusDecoderWrapper> opus_decoder_;

    AudioProcessor audio_processor_;
    BackgroundTask* background_task_ = nullptr;

    // TaskHandle_t main_loop_task_handle_ = nullptr;
    // std::list<std::function<void()>> main_tasks_;
    std::mutex mutex_;

    // // Audio encode / decode
    // TaskHandle_t audio_loop_task_handle_ = nullptr;
    // BackgroundTask* background_task_ = nullptr;
    // std::list<std::vector<int16_t>> audio_decode_queue_;
    std::list<std::vector<uint8_t>> audio_decode_queue_;
    // EventGroupHandle_t event_group_ = nullptr;

    std::unique_ptr<TalkUdp> talk_app_ = nullptr;

    void StartVoice();
    void StopVoice(AudioCodec* codec);
};

#endif // _TALKAPPLICATION_H_
