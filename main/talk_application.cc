#include "talk_application.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>
#include <esp_app_desc.h>

#include "board.h"
// #include "display.h"
#include "system_info.h"
#include "audio_codec.h"
// #include "assets/lang_config.h"

#define TAG "TalkApplication"

//下面是类成员函数

TalkApplication::TalkApplication() {
    background_task_ = new BackgroundTask(4096 * 8);
}

TalkApplication::~TalkApplication() {
    if (background_task_ != nullptr) {
        delete background_task_;
    }
}

void TalkApplication::StartTalk() {
    uint32_t log_counter_ = 0;
    auto& board = Board::GetInstance();
    auto codec = Board::GetInstance().GetAudioCodec();

    /* Setup the display */
    auto display = board.GetDisplay();

    /* Setup the audio codec */
    opus_decoder_ = std::make_unique<OpusDecoderWrapper>(codec->output_sample_rate(), 1, OPUS_ONE_FRAME_DURATION_MS);
    opus_encoder_ = std::make_unique<OpusEncoderWrapper>(16000, 1, OPUS_ONE_FRAME_DURATION_MS);
    
    ESP_LOGI(TAG, "Realtime chat enabled, setting opus encoder complexity to 0");
    opus_encoder_->SetComplexity(0);
    codec->Start();

//     /* Wait for the network to be ready */
    board.StartNetwork();
    board.SetPowerSaveMode(false);

    talk_app_ = std::make_unique<TalkUdp>();
    talk_app_->OnIncomingAudioData([this](std::vector<uint8_t>&& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        audio_decode_queue_.emplace_back(std::move(data));

        // background_task_->Schedule([this, data = std::move(data)]() mutable {

        //     std::vector<int16_t> pcm;
        //     if (!opus_decoder_->Decode(std::move(data), pcm)) {
        //         return;
        //     }
        //     std::lock_guard<std::mutex> lock(mutex_);
        //     audio_decode_queue_.emplace_back(std::move(pcm));
        // });

    });
    talk_app_->OnStatusChange([this, codec](int status) {
        StartVoice();
    });

    audio_processor_.Initialize(codec, true);
    audio_processor_.OnOutput([this](std::vector<int16_t>&& data) {

        if (data.empty()) {
            return;
        }

        background_task_->Schedule([this, data = std::move(data)]() mutable {

            if (data.empty()) {
                return;
            }
            
            opus_encoder_->Encode(std::move(data), [this](std::vector<uint8_t>&& opus) {
                if (opus.empty()) {
                    return;
                }
                talk_app_->SendAudio(std::move(opus));
            });
        });
    });

    while (true)
    {
        log_counter_ += 1;
        if (log_counter_ > 1000) {
            ESP_LOGI(TAG, "MainLoop");
            log_counter_ = 0;
        }

        if (!talk_app_->is_runing){
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        // else {
        //     vTaskDelay(pdMS_TO_TICKS(5));
        // }
    
        std::vector<int16_t> data;
        if (audio_processor_.IsRunning()) {
            int samples = audio_processor_.GetFeedSize();
            if (samples > 0) {
                data.resize(samples);
                codec->InputData(data);
                audio_processor_.Feed(data);
            }
        }
        if (codec->output_enabled()) {
            // OnAudioOutput(codec);
            OnAudioDecodeOnMainOutput(codec);
        }
    }
}

// 从队列中读取音频数据，进行解码
void TalkApplication::OnAudioOutput(AudioCodec* codec) {

    // std::lock_guard<std::mutex> lock(mutex_);
    // if (audio_decode_queue_.empty()) {
    //     return;
    // }

    // ESP_LOGW(TAG, "OnAudioOutput - %d", audio_decode_queue_.size());
    // auto opus = std::move(audio_decode_queue_.front());
    // audio_decode_queue_.pop_front();
    // codec->OutputData(opus);
}

void TalkApplication::OnAudioDecodeOnMainOutput(AudioCodec* codec) {

    std::lock_guard<std::mutex> lock(mutex_);
    if (audio_decode_queue_.empty()) {
        return;
    }

    ESP_LOGW(TAG, "OnAudioOutput - %d", audio_decode_queue_.size());
    auto opus = std::move(audio_decode_queue_.front());
    audio_decode_queue_.pop_front();

    background_task_->Schedule([this, codec, opus = std::move(opus)]() mutable {

        std::vector<int16_t> pcm;
        if (!opus_decoder_->Decode(std::move(opus), pcm)) {
            return;
        }
        codec->OutputData(pcm);
    });
}

//webrtc相关 ----------------------------------------------------->
void TalkApplication::ButtonPressedDown() {
    ESP_LOGW(TAG, "ButtonPressedDown");
    talk_app_->OpenUdp();
}

void TalkApplication::StartVoice(){
    if (talk_app_->is_runing){
        return;
    }
    ESP_LOGW(TAG, "UdpStartVoice");
    auto& board = Board::GetInstance();
    opus_encoder_->ResetState();

    // 预先关闭音频输出，避免升级过程有音频操作
    auto codec = board.GetAudioCodec();
    codec->EnableInput(true);
    codec->EnableOutput(true);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        audio_decode_queue_.clear();
    }
    audio_processor_.Start();
    talk_app_->is_runing = true;
    
}

void TalkApplication::StopVoice(AudioCodec* codec){
    if (!talk_app_->is_runing){
        return;
    }
    ESP_LOGW(TAG, "UdpStopVoice");
    audio_processor_.Stop();
    codec->EnableInput(false);
    talk_app_->is_runing = false;
    codec->EnableOutput(false);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        audio_decode_queue_.clear();
    }
}