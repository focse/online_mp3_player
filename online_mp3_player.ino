#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include <Preferences.h>

#define I2S_DOUT 25 //需替换
#define I2S_BCLK 26 //需替换
#define I2S_LRC 22  //需替换

Preferences preferences;
Audio audio;

WiFiMulti wifiMulti;
String ssid = "你的WiFi名称";
String password = "你的WiFi密码";

const uint16_t port = 8000;
static String music_id = "";

WiFiServer server(port);
TaskHandle_t Task;

void setup() {
  Serial.begin(115200);
  preferences.begin("my-app", false);
  music_id = preferences.getString("music", "1");

  // 设置WiFi模式为终端模式
  WiFi.mode(WIFI_STA);
  // 开始连接WiFi
  WiFi.begin(ssid, password);
  
  // 等待WiFi连接
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.localIP());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(10);  // 0...21

  char httpPath[512];

  //尝试1
  //https://ting8.yymp3.com/new10/sunlu3/1.mp3
  //sprintf(httpPath, "%s%s%s", "https://ting8.yymp3.com/new10/sunlu3/", music_id, ".mp3");

  //尝试2
  //https://m801.music.126.net/20241129141718/b4a366ca5843d8d8afe006284e318074/jdymusic/obj/wo3DlMOGwrbDjj7DisKw/23375432727/809b/5641/7e43/c969dea4f0a4d51acf8c399d4b22111e.mp3
  //https://96.f.1ting.com/local_to_cube_202004121813/96kmp3/2021/11/27/27c_zym/01.mp3
  sprintf(httpPath, "https://96.f.1ting.com/local_to_cube_202004121813/96kmp3/2021/11/27/27c_zym/01.mp3");
  
  Serial.println(httpPath);

  audio.connecttohost(httpPath);  //  128k mp3

  server.begin();

  xTaskCreatePinnedToCore(
    Taskcode, /* Task function. */
    "Task",   /* name of task. */
    10000,    /* Stack size of task */
    NULL,     /* parameter of the task */
    0,        /* priority of the task */
    &Task,    /* Task handle to keep track of created task */
    0);       /* pin task to core 1 */
}

void loop() {
  audio.loop();
}

//Taskcode
void Taskcode(void *pvParameters) {
  Serial.println("Now Playing...");
  server.begin();
  for (;;) {
    WiFiClient client = server.available();  //尝试建立客户对象
    if (client)                              //如果当前客户可用
    {
      Serial.println("[Client connected]");
      while (client.connected())  //如果客户端处于连接状态
      {
        if (client.available())  //如果有可读数据
        {
          audio.stopSong();//停止播放 
          String c = client.readStringUntil('\n');
          Serial.print(c);
          preferences.putString("music", c);
          preferences.end();
          client.stop();
          esp_restart();
        }
      }
      client.stop();  //结束当前连接: 
      Serial.println("[Client disconnected]");
    }
  }
  vTaskDelay(1000);
}