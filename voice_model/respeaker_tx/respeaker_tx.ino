#define EIDSP_QUANTIZE_FILTERBANK   0
#include <chiojynquee-project-1_inferencing.h>
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led_control.h"

// ==== AUDIO CONFIG ====
#define I2S_PORT            I2S_NUM_0
#define I2S_WS              7   // L/R clock
#define I2S_SD              43  // Serial Data In
#define I2S_SCK             8   // Bit Clock

#define SAMPLE_RATE         16000
#define I2S_SAMPLE_BITS     32
#define SAMPLE_BUFFER_SIZE  2048

#define XVF_ADDR 0x2C

#define XIAO_RX 1
#define XIAO_TX 3

// 如果导出的库中没有定义这三个宏，可以手动定义（根据你校准的结果）
#ifndef EI_CLASSIFIER_THRESHOLD
    #define EI_CLASSIFIER_THRESHOLD      0.5
#endif
#ifndef EI_CLASSIFIER_SUPPRESS_MS
    #define EI_CLASSIFIER_SUPPRESS_MS    100
#endif
#ifndef EI_CLASSIFIER_AVERAGE_WINDOW
    #define EI_CLASSIFIER_AVERAGE_WINDOW 1
#endif

// 全局变量（放在 setup 之前或文件顶部）
static int vote_count = 0;
static const char* voted_label = "";          // 当前正在投票的标签
static unsigned long last_action_time = 0;    // 上次执行动作的时间
unsigned long previousActionTime = 0;
const unsigned long ACTION_DURATION = 2000;  // 2秒


// ==== INFERENCE STATE ====
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static int32_t i2s_samples[SAMPLE_BUFFER_SIZE];
static bool record_status = true;
static bool debug_nn = false;

// ==== FUNCTION DECLARATIONS ====
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static void audio_inference_callback(uint32_t n_bytes);
static void capture_samples(void *arg);
static int i2s_init();
static void i2s_deinit();

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.printf("PSRAM free: %u KB\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
    Serial.printf("DRAM free:  %u KB\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);

    Serial.printf("XVF3800 Keyword Spotting Inference Start\n");

    Serial.printf("Model info:\n");
    Serial.printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    Serial.printf("\tSample length: %d ms\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / (SAMPLE_RATE / 1000));
    Serial.printf("\tInterval: %.2f ms\n", EI_CLASSIFIER_INTERVAL_MS);

    Serial.printf("threshold: %.2f \n", EI_CLASSIFIER_THRESHOLD);
    Serial.printf("supress: %d \n", EI_CLASSIFIER_SUPPRESS_MS);
    Serial.printf("window: %d \n", EI_CLASSIFIER_AVERAGE_WINDOW);

    if (!microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT)) {
        Serial.printf("ERR: Audio buffer allocation failed.\n");
        return;
    }

    // 与扩展板通信
    // Serial2.begin(9600, SERIAL_8N1, XIAO_RX, XIAO_TX);

    initLEDs();

    // 导出音频连wifi
    // Serial.printf("Connecting to WiFi...\n");
    // WiFi.begin(ssid, password);
    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(500);
    //     Serial.print(".");
    // }
    // Serial.println("\nWiFi connected");
    // udp.begin(udpPort);
    // 结束

    Serial.printf("Listening...\n");
}

void loop() {
    // 定义一下识别的词best和对应的值best_val
    const char* best = nullptr;
    float best_val = 0;

    if (!microphone_inference_record()) {
        Serial.printf("ERR: Failed to record audio.\n");
        return;
    }

    // --- 发送音频到 PC ---
    // uint8_t* ptr = (uint8_t*)inference.buffer;
    // int remaining = EI_CLASSIFIER_RAW_SAMPLE_COUNT * sizeof(int16_t);
    // while (remaining > 0) {
    //     int chunk = min(remaining, 1024);
    //     udp.beginPacket(udpAddress, udpPort);
    //     udp.write(ptr, chunk);
    //     udp.endPacket();
    //     ptr += chunk;
    //     remaining -= chunk;
    // }
    // ---------------------

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;

    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);

    if (r != EI_IMPULSE_OK) {
        Serial.printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    Serial.printf("Predictions:\n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.printf("  %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
        if (result.classification[ix].value > best_val) {
        best_val = result.classification[ix].value;
        best = result.classification[ix].label;
        }
    }

    // 测试RMS能量
    float sum_sq = 0;
    for (size_t i = 0; i < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++) {
        float sample = (float)inference.buffer[i];
        sum_sq += sample * sample;
    }
    float rms = sqrt(sum_sq / EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    Serial.printf("!!!energy (%.1f)\n", rms);

    // ===================== 误触发过滤逻辑开始 =====================

    // 1. 如果正在执行动作，检查是否已到时间
    unsigned long now = millis();
    if (previousActionTime != 0) {
        if (now - previousActionTime >= ACTION_DURATION) {
            previousActionTime = 0;  // 重置，允许下一次动作
            Serial.println("Action finished.");
            // Serial2.write('O');
            clearArrows();
        }
        // 动作未结束，跳过本次推理
        return;
    }
    if (now - last_action_time < EI_CLASSIFIER_SUPPRESS_MS) {
        Serial.printf("Suppression active, ignoring result.\n");
        Serial.printf("\n===============================================\n");
        return;
    }

    // 2. 能量过低（静音）或置信度不足，直接丢弃
    if (best_val < EI_CLASSIFIER_THRESHOLD) {
        // 清空投票计数（因为当前帧不可靠）
        vote_count = 0;
        voted_label = "";
        Serial.printf("Low confidence, reset vote. 最高值: %.2f \n", best_val);
        Serial.printf("\n===============================================\n");
        return;
    }

    // 3. 排除 noise / unknown 标签
    if (best == "noise" || best == "unknown") {
        vote_count = 0;
        voted_label = "";
        Serial.printf("听不懂");
        Serial.printf("\n===============================================\n");
        return;
    }

    // 4. 连续投票逻辑
    if (strcmp(voted_label, best) == 0) {
        // 与上一帧相同的标签，投票计数 +1
        vote_count++;
    } else {
        // 标签改变了，重置投票计数，开始新的一轮
        vote_count = 1;
        voted_label = best;
    }
    Serial.printf("投票: %d/%d 给 '%s'\n", vote_count, EI_CLASSIFIER_AVERAGE_WINDOW, best);


    if (vote_count >= EI_CLASSIFIER_AVERAGE_WINDOW){
        if (best == "right") {
            Serial.printf("最高的是 right, 执行 右转");
            // Serial2.write('R');
            drawArrow(false);
        }
        else if (best == "left") {
            Serial.printf("最高的是 left, 执行 左转");
            // Serial2.write('L');
            drawArrow(true);
        }
        else if (best == "turn_on") {
            Serial.printf("最高的是 turn_on, 执行 超亮灯");
            // Serial2.write('H');
            setHeadlight(250);
        }
        else if (best == "dim") {
            Serial.printf("最高的是 dim, 执行 不亮灯");
            // Serial2.write('M');
            setHeadlight(50);
        }
        else if (best == "off") {
            Serial.printf("最高的是 off, 执行 关灯");
            // Serial2.write('D');
            setHeadlight(0);
        }
    }
    // 执行动作后重置投票，并记录抑制时间
    previousActionTime = millis();  // 记录动作开始时间
    vote_count = 0;
    voted_label = "";
    last_action_time = now;

    Serial.printf("\n===============================================\n");

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    Serial.printf("  Anomaly score: ");
    ei_printf_float(result.anomaly);
    Serial.printf("\n");
#endif
}

// ==== INFERENCE AND AUDIO HANDLING ====

static void audio_inference_callback(uint32_t n_bytes) {
    uint32_t sample_pairs = n_bytes / (2 * sizeof(int32_t)); // 立体声对数
    for (uint32_t i = 0; i < sample_pairs; i++) {
        int16_t val = (int16_t)(i2s_samples[i * 2] >> 16);  // 左声道
        inference.buffer[inference.buf_count++] = val;
        if (inference.buf_count >= inference.n_samples) {
            inference.buf_ready = 1;
            inference.buf_count = 0;
        }
    }
}

static void capture_samples(void *arg) {
    size_t bytes_read;
    while (record_status) {
        i2s_read(I2S_PORT, (char *)i2s_samples, SAMPLE_BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);

        if (bytes_read > 0) {
            audio_inference_callback(bytes_read);
        } else {
            ei_printf("ERR: I2S read failed\n");
        }
    }
    vTaskDelete(NULL);
}

static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));
    if (!inference.buffer) return false;

    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    if (i2s_init() != 0) {
        ei_printf("ERR: I2S init failed\n");
        return false;
    }

    xTaskCreate(capture_samples, "CaptureSamples", 4096, NULL, 1, NULL);
    return true;
}

static bool microphone_inference_record(void) {
    while (!inference.buf_ready) {
        delay(10);
    }
    inference.buf_ready = 0;
    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}

static int i2s_init() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)I2S_SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   // ← 改为立体声交错
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    esp_err_t err;
    err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) return err;

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) return err;

    err = i2s_zero_dma_buffer(I2S_PORT);
    return err;
}

static void i2s_deinit() {
    i2s_driver_uninstall(I2S_PORT);
}
