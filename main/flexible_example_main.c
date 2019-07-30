/*  Flexible pipeline playback with different music

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "http_stream.h"
#include "mp3_decoder.h"
#include "aac_decoder.h"

#include "audio_hal.h"
#include "filter_resample.h"
#include "esp_peripherals.h"
#include "periph_button.h"

#define I2S_STREAM_CFG() {                                                      \
    .type = AUDIO_STREAM_WRITER,                                                \
    .task_prio = I2S_STREAM_TASK_PRIO,                                          \
    .task_core = I2S_STREAM_TASK_CORE,                                          \
    .task_stack = I2S_STREAM_TASK_STACK,                                        \
    .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,                                  \
    .i2s_config = {                                                             \
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  \
        .sample_rate = 44100,                                                   \
        .bits_per_sample = 16,                                                  \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
        .communication_format = I2S_COMM_FORMAT_I2S_LSB,                            \
        .dma_buf_count = 3,                                                     \
        .dma_buf_len = 300,                                                     \
        .use_apll = 1,                                                          \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,                               \
    },                                                                          \
    .i2s_pin_config = {                                                         \
        .bck_io_num = 27,                                                 \
        .ws_io_num = 26,                                                  \
        .data_out_num = 25,                                               \
        .data_in_num = -1,                                                \
    },                                                                          \
    .i2s_port = 0,                                                              \
}

static const char *TAG = "FLEXIBLE_PIPELINE";

#define SAVE_FILE_RATE      44100
#define SAVE_FILE_CHANNEL   2
#define SAVE_FILE_BITS      16

#define PLAYBACK_RATE       48000
#define PLAYBACK_CHANNEL    2
#define PLAYBACK_BITS       16

static audio_element_handle_t create_filter(int source_rate, int source_channel, int dest_rate, int dest_channel, audio_codec_type_t type)
{
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = source_rate;
    rsp_cfg.src_ch = source_channel;
    rsp_cfg.dest_rate = dest_rate;
    rsp_cfg.dest_ch = dest_channel;
    rsp_cfg.type = type;
    return rsp_filter_init(&rsp_cfg);
}

static audio_element_handle_t create_http_stream(const char *url){
    
}

static audio_element_handle_t create_i2s_stream(int sample_rates, int bits, int channels, audio_stream_type_t type)
{
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG();
    i2s_cfg.type = type;
    audio_element_handle_t i2s_stream = i2s_stream_init(&i2s_cfg);
    mem_assert(i2s_stream);
    audio_element_info_t i2s_info = {0};
    audio_element_getinfo(i2s_stream, &i2s_info);
    i2s_info.bits = bits;
    i2s_info.channels = channels;
    i2s_info.sample_rates = sample_rates;
    audio_element_setinfo(i2s_stream, &i2s_info);
    return i2s_stream;
}

static audio_element_handle_t create_mp3_decoder()
{
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    return mp3_decoder_init(&mp3_cfg);
}

static audio_element_handle_t create_aac_decoder()
{
    aac_decoder_cfg_t aac_cfg = DEFAULT_AAC_DECODER_CONFIG();
    return aac_decoder_init(&aac_cfg);
}

void flexible_pipeline_playback()
{
    audio_pipeline_handle_t pipeline_play = NULL;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline_play = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[ 1 ] Create all audio elements for playback pipeline")
    

    audio_element_handle_t fatfs_aac_reader_el = create_fatfs_stream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_READER);
    audio_element_handle_t fatfs_mp3_reader_el = create_fatfs_stream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_READER);
    audio_element_handle_t mp3_decoder_el = create_mp3_decoder();
    audio_element_handle_t aac_decoder_el = create_aac_decoder();
    audio_element_handle_t filter_upsample_el = create_filter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, PLAYBACK_RATE, PLAYBACK_CHANNEL, AUDIO_CODEC_TYPE_DECODER);
    audio_element_handle_t i2s_writer_el = create_i2s_stream(PLAYBACK_RATE, PLAYBACK_BITS, PLAYBACK_CHANNEL, AUDIO_STREAM_WRITER);

    ESP_LOGI(TAG, "[ 2 ] Register all audio elements to playback pipeline");
    audio_pipeline_register(pipeline_play, fatfs_aac_reader_el,  "file_aac_reader");
    audio_pipeline_register(pipeline_play, fatfs_mp3_reader_el,  "file_mp3_reader");
    audio_pipeline_register(pipeline_play, mp3_decoder_el,       "mp3_decoder");
    audio_pipeline_register(pipeline_play, aac_decoder_el,       "aac_decoder");
    audio_pipeline_register(pipeline_play, filter_upsample_el,   "filter_upsample");
    audio_pipeline_register(pipeline_play, i2s_writer_el,        "i2s_writer");

    char *p0_reader_tag = NULL;
    audio_element_set_uri(fatfs_aac_reader_el, "/sdcard/test.aac");
    p0_reader_tag = "file_aac_reader";
    audio_element_set_uri(fatfs_mp3_reader_el, "/sdcard/test.mp3");

    ESP_LOGI(TAG, "[ 3 ] Setup event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    audio_event_iface_set_listener(esp_periph_get_event_iface(), evt);

    ESP_LOGI(TAG, "[3.1] Setup i2s clock");
    i2s_stream_set_clk(i2s_writer_el, PLAYBACK_RATE, PLAYBACK_BITS, PLAYBACK_CHANNEL);

    ESP_LOGI(TAG, "[ 4 ] Start playback pipeline");
    bool source_is_mp3_format = false;
    audio_pipeline_link(pipeline_play, (const char *[]) {p0_reader_tag, "aac_decoder", "filter_upsample", "i2s_writer"}, 4);
    audio_pipeline_run(pipeline_play);
    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type != PERIPH_ID_BUTTON) {
            audio_element_handle_t el = (audio_element_handle_t)msg.source;
            ESP_LOGI(TAG, "Element tag:[%s],src_type:%x, cmd:%d, data_len:%d, data:%p",
                     audio_element_get_tag(el), msg.source_type, msg.cmd, msg.data_len, msg.data);
            continue;
        }
        if (((int)msg.data == GPIO_MODE) && (msg.cmd == PERIPH_BUTTON_PRESSED)) {
            source_is_mp3_format = !source_is_mp3_format;
            audio_pipeline_pause(pipeline_play);
            ESP_LOGE(TAG, "Changing music to %s", source_is_mp3_format ? "mp3 format" : "aac format");
            if (source_is_mp3_format) {
                audio_pipeline_breakup_elements(pipeline_play, aac_decoder_el);
                audio_pipeline_relink(pipeline_play, (const char *[]) {"file_mp3_reader", "mp3_decoder", "filter_upsample", "i2s_writer"}, 4);
                audio_pipeline_set_listener(pipeline_play, evt);
            } else {
                audio_pipeline_breakup_elements(pipeline_play, mp3_decoder_el);
                audio_pipeline_relink(pipeline_play, (const char *[]) {p0_reader_tag, "aac_decoder", "filter_upsample", "i2s_writer"}, 4);
                audio_pipeline_set_listener(pipeline_play, evt);
            }
            audio_pipeline_run(pipeline_play);
            audio_pipeline_resume(pipeline_play);
            ESP_LOGE(TAG, "[ 4.1 ] Start playback new pipeline");
        }
    }

    ESP_LOGI(TAG, "[ 5 ] Stop playback pipeline");
    audio_pipeline_terminate(pipeline_play);
    audio_pipeline_unregister_more(pipeline_play, fatfs_aac_reader_el,
                                   fatfs_mp3_reader_el, mp3_decoder_el,
                                   aac_decoder_el, filter_upsample_el, i2s_writer_el, NULL);

    audio_pipeline_remove_listener(pipeline_play);
    esp_periph_stop_all();
    audio_event_iface_remove_listener(esp_periph_get_event_iface(), evt);
    audio_event_iface_destroy(evt);

    audio_element_deinit(fatfs_aac_reader_el);
    audio_element_deinit(fatfs_mp3_reader_el);
    audio_element_deinit(mp3_decoder_el);
    audio_element_deinit(aac_decoder_el);
    audio_element_deinit(filter_upsample_el);
    audio_element_deinit(i2s_writer_el);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_DEBUG);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    tcpip_adapter_init();

    // Initialize peripherals management
    esp_periph_config_t periph_cfg = { 0 };
    esp_periph_init(&periph_cfg);

    flexible_pipeline_playback();
    esp_periph_destroy();
}
