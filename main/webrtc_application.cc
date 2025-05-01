#include "webrtc_application.h"

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

#define TAG "WebrtcApplication"

//下面是类成员函数

WebrtcApplication::WebrtcApplication() {
    background_task_ = new BackgroundTask(4096 * 8);
}

WebrtcApplication::~WebrtcApplication() {
    if (background_task_ != nullptr) {
        delete background_task_;
    }
}

void WebrtcApplication::StartWebrtc() {
    uint32_t log_counter_ = 0;
    auto& board = Board::GetInstance();
    auto codec = Board::GetInstance().GetAudioCodec();

    /* Setup the display */
    auto display = board.GetDisplay();

    /* Setup the audio codec */
    opus_decoder_ = std::make_unique<OpusDecoderWrapper>(codec->output_sample_rate(), 1, OPUS_FRAME_DURATION_MS);
    opus_encoder_ = std::make_unique<OpusEncoderWrapper>(16000, 1, OPUS_FRAME_DURATION_MS);
    
    ESP_LOGI(TAG, "Realtime chat enabled, setting opus encoder complexity to 0");
    opus_encoder_->SetComplexity(0);
    codec->Start();

//     xTaskCreatePinnedToCore([](void* arg) {
//         WebrtcApplication* app = (WebrtcApplication*)arg;
//         app->AudioLoop();
//         vTaskDelete(NULL);
//     }, "audio_loop", 4096 * 2, this, 8, &audio_loop_task_handle_, 1);

//     /* Start the main loop */
//     xTaskCreatePinnedToCore([](void* arg) {
//         WebrtcApplication* app = (WebrtcApplication*)arg;
//         app->MainLoop();
//         vTaskDelete(NULL);
//     }, "main_loop", 4096 * 2, this, 4, &main_loop_task_handle_, 0);

//     /* Wait for the network to be ready */
    board.StartNetwork();
    board.SetPowerSaveMode(false);

    app_webrtc_ = std::make_unique<AppWebrtc>();
    app_webrtc_->OnIncomingAudioData([this](std::vector<uint8_t>&& data) {
        // std::lock_guard<std::mutex> lock(mutex_);
        // audio_decode_queue_.emplace_back(std::move(data));

        background_task_->Schedule([this, data = std::move(data)]() mutable {

            std::vector<int16_t> pcm;
            if (!opus_decoder_->Decode(std::move(data), pcm)) {
                return;
            }
            std::lock_guard<std::mutex> lock(mutex_);
            audio_decode_queue_.emplace_back(std::move(pcm));
        });

    });
    app_webrtc_->OnWebrtcStatusChange([this, codec](int status) {
        //PEER_CONNECTION_COMPLETED
        if (status == 4){
            WebrtcStartVoice();
        }
        else{
            WebrtcStopVoice(codec);
        }
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
                // protocol_->SendAudio(std::move(opus));
                // app_webrtc_->SendAudioData(std::move(opus));
                if (opus.empty()) {
                    return;
                }
                app_webrtc_->WebrtcReadAudioData(std::move(opus));
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

        if (!app_webrtc_->webrtc_is_runing){
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        else {
            // vTaskDelay(pdMS_TO_TICKS(5));
        }
    
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
            OnAudioOutput(codec);
        }
    }
}

// 从队列中读取音频数据，进行解码
void WebrtcApplication::OnAudioOutput(AudioCodec* codec) {

    // std::unique_lock<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> lock(mutex_);
    if (audio_decode_queue_.empty()) {
        return;
    }

    // ESP_LOGW(TAG, "OnAudioOutput - %d", audio_decode_queue_.size());
    auto opus = std::move(audio_decode_queue_.front());
    audio_decode_queue_.pop_front();
    codec->OutputData(opus);
}

void WebrtcApplication::OnAudioDecodeOnMainOutput(AudioCodec* codec) {

    // std::lock_guard<std::mutex> lock(mutex_);
    // if (audio_decode_queue_.empty()) {
    //     return;
    // }

    // // ESP_LOGW(TAG, "OnAudioOutput - %d", audio_decode_queue_.size());
    // auto opus = std::move(audio_decode_queue_.front());
    // audio_decode_queue_.pop_front();

    // background_task_->Schedule([this, codec, opus = std::move(opus)]() mutable {

    //     std::vector<int16_t> pcm;
    //     if (!opus_decoder_->Decode(std::move(opus), pcm)) {
    //         return;
    //     }
    //     codec->OutputData(pcm);
    // });
}

//webrtc相关 ----------------------------------------------------->
void WebrtcApplication::ButtonPressedDown() {
    ESP_LOGW(TAG, "ButtonPressedDown");
    // app_webrtc_->StartConnectOffer(SystemInfo::GetMacAddress().c_str());
    app_webrtc_->StartConnectAnswer(SystemInfo::GetMacAddress().c_str());
}

void WebrtcApplication::WebrtcStartVoice(){
    if (app_webrtc_->webrtc_is_runing){
        return;
    }
    ESP_LOGW(TAG, "WebrtcStartVoice");
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
    app_webrtc_->StartAudio();
    
}

void WebrtcApplication::WebrtcStopVoice(AudioCodec* codec){
    if (!app_webrtc_->webrtc_is_runing){
        return;
    }
    ESP_LOGW(TAG, "WebrtcStopVoice");
    audio_processor_.Stop();
    codec->EnableInput(false);
    app_webrtc_->StopAudio();
    codec->EnableOutput(false);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        audio_decode_queue_.clear();
    }
}