#include "WiFi.h"
#include "WiFiUdp.h"
#include "AudioTools.h"


const char* ssid     = "";
const char* password = "";
const char* udpAddress = "...";
const int udpPort = 12345;


WiFiUDP udp;

// 音频：16kHz，立体声，32位
AudioInfo info(16000, 2, 32);
I2SStream i2s_in;
I2SConfig i2s_config;

#define I2S_DMA_SAMPLES 1024
#define PACKET_SAMPLES  512    // 1024 int32 立体声 → 512 左声道
#define PACKET_BYTES    (PACKET_SAMPLES * sizeof(int16_t))  // = 1024 bytes

static int32_t i2s_dma[I2S_DMA_SAMPLES];
static int16_t audio_packet[PACKET_SAMPLES];  // 待发送的16位单声道数据

void connectWiFi() {
  Serial.printf("连接到WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n已连接");
}

void setupI2SInput() {
  i2s_config = i2s_in.defaultConfig(RX_MODE);
  i2s_config.copyFrom(info);

  // XVF3800引脚
  i2s_config.pin_bck = 8;     
  i2s_config.pin_ws = 7;      
  i2s_config.pin_data = 44;   
  i2s_config.pin_data_rx = 43;  
  i2s_config.is_master = true;  

  i2s_in.begin(i2s_config);
  Serial.println("I2S输入已启动。");
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  setupI2SInput();
  udp.begin(udpPort);
  Serial.printf("UDP target: %s:%d\n", udpAddress, udpPort);
}

void loop() {
  // 1. 等待并读取一帧I2S数据（1024个int32 = 4096字节）
  size_t bytes_read = i2s_in.readBytes((uint8_t*)i2s_dma, I2S_DMA_SAMPLES * sizeof(int32_t));


  // 2. 提取左声道并转换为16位（右移16位，保留高16位有效值）
  for (int i = 0; i < PACKET_SAMPLES; i++) {
    // 立体声交错：左声道在偶数索引，右声道在奇数索引
    audio_packet[i] = (int16_t)(i2s_dma[i * 2] >> 16);
  }

  // 3. 通过UDP发送
  udp.beginPacket(udpAddress, udpPort);
  udp.write((uint8_t*)audio_packet, PACKET_BYTES);
  udp.endPacket();

  // 可选调试打印
  Serial.printf("已发送UDP包, %d 字节\n", PACKET_BYTES);
}