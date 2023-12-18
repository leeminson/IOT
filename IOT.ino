#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <WebServer.h> 
#include <Arduino_JSON.h>
const char* ssid = "TP-Link_434A";
const char* password = "56128048";
String chatId = "6603030584";
String BOTtoken = "6558246870:AAHHQu-dLKnMVHmxcOK1IrPGjL05SARaUoo";
bool sendPhoto = false;
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#define FLASH_LED_PIN 15
bool flashState = LOW;
const byte motionSensor = 15;
bool motionDetected = false;
bool motionDetectEnable = false;
const byte door1 = 14;
const byte door2 = 15;
bool door1stat = true;
bool door2stat = true;
bool doorLockMonitor = false;
bool fireDetectMonitor = false;
const byte firePin = 12;
bool fire = false;
String fireStatus="Không phát hiện đám cháy ";
String smokeStatus="Không có bất thường về nồng độ gas";
bool smokeDetectMonitor = false;
const byte smokePin = 02;
bool smoke = false;

int botRequestDelay = 1000;
long lastTimeBotRan;
String jsonString;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
String web="<!DOCTYPE html><html><head><meta charset= 'UTF-8' ><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>body {font-family: Arial, sans-serif;margin: 0;padding: 0;display: flex;align-items: center;justify-content: center;height: 100vh;background-color: #f4f4f4;}#alert-container {position: fixed;top: 50%;left: 50%;transform: translate(-50%, -50%);display: flex;align-items: center;justify-content: space-between;background-color: #ffffff;box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);border-radius: 5px;width: 700px;}.alert-box {padding: 20px;text-align: center;flex: 1;margin: 0 10px;}#gas-alert {color: #ffcc00;font-size: 18px;}#fire-alert {color: #ff0000;font-size: 18px;}</style><title>Gas and Fire Alert</title></head><body><div id='alert-container'><div class='alert-box'><p id='gas-alert'>%gasMsg%</p></div><div class='alert-box'><p id='fire-alert'>%fireMsg%</p></div></div><script>var Socket;function init() {Socket = new WebSocket('ws://' + window.location.hostname + ':81/');Socket.onmessage = function (event) {processCommand(event);};}function processCommand(event) {var obj = JSON.parse(event.data);document.getElementById('fire-alert').innerHTML = obj.fireStatus;document.getElementById('gas-alert').innerHTML = obj.smokeStatus;console.log(obj.fireStatus);console.log(obj.smokeStatus)}window.onload = function (event) { init(); }</script></body></html>";

void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();
static void IRAM_ATTR detectsMovement(void* arg) {
  motionDetected = true;
}
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // enum that read status this is used for debugging.
      Serial.print("WS Type ");
      Serial.print(type);
      Serial.println(": DISCONNECTED");
      break;
    case WStype_CONNECTED:  // Check if a WebSocket client is connected or not
      Serial.print("WS Type ");
      Serial.print(type);
      Serial.println(": CONNECTED");
      fireStatus="Không phát hiện đám cháy ";
      smokeStatus="Không có bất thường về nồng độ gas";
      update_webpage();
      
      break;
    case WStype_TEXT: // check responce from client
      Serial.println(); // the payload variable stores teh status internally
      Serial.println(payload[0]);
      
      break;
  }
}
void update_webpage()
{
  StaticJsonDocument<100> doc;
  // create an object
  JsonObject object = doc.to<JsonObject>();
  object["fireStatus"] = fireStatus ;
  object["smokeStatus"] = smokeStatus ;
  serializeJson(doc, jsonString); // serialize the object and save teh result to teh string variable.
  Serial.println( jsonString ); // print the string for debugging.
  webSocket.broadcastTXT(jsonString); // send the JSON object through the websocket
  jsonString = ""; // clear the String.
}
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);



  pinMode(door1, INPUT);
  pinMode(door2, INPUT);
  pinMode(firePin, INPUT);
  pinMode(smokePin, INPUT);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  sensor_t* s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);
  err = gpio_isr_handler_add(GPIO_NUM_13, &detectsMovement, (void*)13);
  if (err != ESP_OK) {
    Serial.printf("handler add failed with error 0x%x \r\n", err);
  }
  err = gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_POSEDGE);
  if (err != ESP_OK) {
    Serial.printf("set intr type failed with error 0x%x \r\n", err);
  }
  delay(40000);

   server.on("/", []() {
    server.send(200, "text\html", web);
  });
  server.begin(); // init the server
  webSocket.begin();  // init the Websocketserver
  webSocket.onEvent(webSocketEvent);
  
}
void loop() {
  server.handleClient();  // webserver methode that handles all Client
  webSocket.loop(); // websocket server methode that handles all Client
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  if (sendPhoto) {
    Serial.println("Đang chụp hình...");
    sendPhotoTelegram();
    sendPhoto = false;
  }
  fire = digitalRead(firePin);
  if (!fire && fireDetectMonitor) {
    bot.sendMessage(chatId, "Phát hiện đám cháy !!", "");
    fireStatus = "Phát hiện đám cháy";
    update_webpage();
    Serial.println("Fire Detected");
    fire = false;
  }
  smoke = digitalRead(smokePin);
  if (!smoke && smokeDetectMonitor) {
    bot.sendMessage(chatId, "Phát hiện khí gas rò rỉ !!", "");
    smokeStatus = "Phát hiện rò rỉ khí gas";
    update_webpage();
    Serial.println("Smoke Detected");
    smoke = false;
  }
}

String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t* fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Take Photo failed");
    delay(1000);
    ESP.restart();
    return "Take photo failed";
  }

  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    String head = "--BtlIOT\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--BtlIOT\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--BtlIOT--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    clientTCP.println("POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=BtlIOT");
    clientTCP.println();
    clientTCP.print(head);

    uint8_t* fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        clientTCP.write(fbBuf, remainder);
      }
    }

    clientTCP.print(tail);

    esp_camera_fb_return(fb);

    int waitTime = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + waitTime) > millis()) {
      Serial.print(".");
      delay(100);
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state == true) getBody += String(c);
        if (c == '\n') {
          if (getAll.length() == 0) state = true;
          getAll = "";
        } else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length() > 0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  } else {
    getBody = "Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    String text = bot.messages[i].text;
    Serial.println(text);

    String fromName = bot.messages[i].from_name;
    if (text == "/FlashOn") {
      flashState = true;
      bot.sendMessage(chat_id, "Bật Flash", "");
      digitalWrite(FLASH_LED_PIN, flashState);
    }
    if (text == "/FlashOff") {
      flashState = false;
      bot.sendMessage(chat_id, "Tắt Flash", "");
      digitalWrite(FLASH_LED_PIN, flashState);
    }
    if (text == "/takePhoto") {
      sendPhoto = true;
      bot.sendMessage(chat_id, "Đang chụp hình......", "");
      Serial.println("New photo request");
    }
    if (text == "/FireAlertOn") {
      fireDetectMonitor = true;
      bot.sendMessage(chat_id, "Đã bật cảm biến lửa", "");
      Serial.println("Enable the Fire Detector");
    }

    if (text == "/FireAlertOff") {
      fireDetectMonitor = false;
      bot.sendMessage(chat_id, "Đã tắt cảm biến lửa", "");
      Serial.println("Disable the Fire Detector");
    }


    if (text == "/SmokeAlertOn") {
      smokeDetectMonitor = true;
      bot.sendMessage(chat_id, "Đã bật cảm biến gas ", "");
      Serial.println("Enable the Smoke Detector");
    }

    if (text == "/SmokeAlertOff") {
      smokeDetectMonitor = false;
      bot.sendMessage(chat_id, "Đã tắt cảm biến gas", "");
      Serial.println("Disable the Smoke Detector");
    }
    if (text == "/start") {
      String welcome = "ESP32-CAM Telegram bot.\n\n";
      welcome += "Dùng lệnh để thiết lập.\n\n";
      welcome += "/FlashOn : Bật flash LED\n";
      welcome += "/FlashOff : Tắt flash LED\n";
      welcome += "/takePhoto : Chụp hình\n\n";
      welcome += "/SmokeAlertOn : Bật cảm biến khí gas\n";
      welcome += "/SmokeAlertOff : Tắt cảm biến khí gas\n\n";
      welcome += "/FireAlertOn : Bật cảm biến lửa\n";
      welcome += "/FireAlertOff : Tắt cảm biến lửa\n\n";
      bot.sendMessage(chatId, welcome, "Markdown");
    }
  }
}
