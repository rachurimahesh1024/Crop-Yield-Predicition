

#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp_http_server.h"
#include "Arduino.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
//#include "SparkFunBME280.h"

#include "camwebserver.h"

// Replace with your network credentials
const char* ssid = "iotserver";
const char* password = "iotserver123";

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
//String chatId = "5196381532"; //PF
String chatId = "2022371156";


// Initialize Telegram BOT
//String BOTtoken = "5296825526:AAETQw_K8Foja3pPWKdoznPRQdLZTzzNYwg"; //PF
String BOTtoken = "7027853901:AAHTCs3pCrJOs2TtKwG4rGTA9T3S13SM8dE";
//7027853901:AAHTCs3pCrJOs2TtKwG4rGTA9T3S13SM8dE

bool sendPhoto = false;
bool speakersend = false;
bool mosb = false;

WiFiClientSecure clientTCP;

UniversalTelegramBot bot(BOTtoken, clientTCP);

char chr1,chr2,chr3,chr4,chr5,chr6,chr7,chr8,chr9,chr10,chr11,chr12,chr13,chr14,chr15;
char finallat[10],finallong[10];
String finallat1="";
String finallong1="";
      
int sti=0; 
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete


int cam_pin = 14;


/*
//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
*/

#define FLASH_LED_PIN 4
bool flashState = LOW;

// Motion Sensor
bool motionDetected = false;

// Define I2C Pins for BME280
//#define I2C_SDA 14
//#define I2C_SCL 15

//BME280 bme;
 
int botRequestDelay = 1000;   // mean time between scan messages
long lastTimeBotRan;     // last time messages' scan has been done

void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();

// Get BME280 sensor readings and return them as a String variable
String getReadings(){
  float temperature, humidity;
  //temperature = bme.readTempC();
  //temperature = bme.readTempF();
  //humidity = bme.readFloatHumidity();
 
  //String message = "Temperature: " + String(temperature) + " ºC \n";
  //message += "Humidity: " + String (humidity) + " % \n";

 //https://www.google.co.in/search?client=opera&q=17.87687%2C78.8980
  String message = "Need_Help_https://www.google.co.in/search?client=opera&q=";
  message = message + finallat1 + "%2C" + finallong1;
  return message;
}

// Indicates when motion is detected
static void IRAM_ATTR detectsMovement(void * arg){
  //Serial.println("MOTION DETECTED!!!");
  motionDetected = true;
}

void initWiFi()
{
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
}

void initCamera()
{
 // OV2640 camera module
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
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  } 

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 12;
    config.fb_count = 1;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
   if(httpd_start(&stream_httpd, &config) == ESP_OK) 
     {
       httpd_register_uri_handler(stream_httpd, &index_uri);
     }
}
void setup()
{
  Serial.begin(9600);

  pinMode(cam_pin, INPUT);
  
    
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

/*
  // Init BME280 sensor
  Wire.begin(I2C_SDA, I2C_SCL);
  bme.settings.commInterface = I2C_MODE;
  bme.settings.I2CAddress = 0x76;
  bme.settings.runMode = 3;
  bme.settings.tStandby = 0;
  bme.settings.filter = 0;
  bme.settings.tempOverSample = 1;
  bme.settings.pressOverSample = 1;
  bme.settings.humidOverSample = 1;
  bme.begin();
*/  

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
 
  initCamera(); delay(10);
  //startCameraServer();
}

int sts1=0;
char moss='x';
void loop()
{
  if(sendPhoto)
    {
     Serial.println("Preparing photo");
     sendPhotoTelegram(); 
     sendPhoto = false; 
     Serial.println("Ready");
    }
    /*
  if(digitalRead(cam_pin) == LOW)
    { delay(1000);
     //while(digitalRead(button) == LOW);
      delay(1000);
     //Serial.println("Someone Came");
      bot.sendMessage(chatId, "Need_Help_https://maps.app.goo.gl/HB73FJLkRAZH8mTy6", "");

     delay(2000);
        // Serial.write("https://www.google.co.in/search?client=opera&q=17.87687%2C78.8980");
        


     Serial.println("Preparing photo");
      sendPhotoTelegram(); 
     
     delay(5000);
     Serial.println("Ready");
    }
      */

    while(Serial.available()) 
        {
         char inChar = (char)Serial.read();
          if(inChar == '*')
            {sti=1;
            }
          if(sti == 1)
            {
                inputString += inChar;
            }
          if(inChar == '#')
            {sti=0;
              stringComplete = true;      
            }
        }
        
   if(stringComplete)
     {
  //    emailSubject = "";
  //    emailSubject = inputString;
      chr1 = inputString[1];
      chr2 = inputString[2];
      chr3 = inputString[3];
      chr4 = inputString[4];
      chr5 = inputString[5];
      chr6 = inputString[6];
      chr7 = inputString[7];
      
      chr8 = inputString[8];
      chr9 = inputString[9];
      chr10 = inputString[10];
      chr11 = inputString[11];
      chr12 = inputString[12];
      chr13 = inputString[13];
      chr14 = inputString[14];
      chr15 = inputString[15];

      finallat[0]=chr1;
      finallat[1]=chr2;
      finallat[2]=chr3;
      finallat[3]=chr4;
      finallat[4]=chr5;
      finallat[5]=chr6;
      finallat[6]=chr7;
      finallat[7]='\0';
      
      finallong[0]=chr8;
      finallong[1]=chr9;
      finallong[2]=chr10;
      finallong[3]=chr11;
      finallong[4]=chr12;
      finallong[5]=chr13;
      finallong[6]=chr14;
      finallong[7]=chr15;
      finallong[8]='\0';
      finallat1 = String(finallat);
      finallong1 = String(finallong);

      //bot.sendMessage(chatId, "Need_Help_https://maps.app.goo.gl/HB73FJLkRAZH8mTy6", "");

      String tele_message = "Need_Help_https://www.google.co.in/search?client=opera&q=" + finallat1 + "%2C" + finallong1;
      bot.sendMessage(chatId, tele_message, "");
  
      
      delay(2000);
        // Serial.write("https://www.google.co.in/search?client=opera&q=17.87687%2C78.8980");
        
      Serial.println("Preparing photo");
       sendPhotoTelegram(); 
     
      delay(5000);
      Serial.println("Ready");
     
      inputString="";
      stringComplete = false;     
      delay(5000);
     }

/*
 if(digitalRead(mos) == LOW)
    { sts1++;
      if(sts1 >= 2){sts1=2;}
   if(sts1 == 1)
     {   
       delay(1000);
      //while(digitalRead(button) == LOW);
       delay(1000);
       Serial.println("Mos Wet");
       moss='w';
       bot.sendMessage(chatId, "Mos_Wet", "");

       delay(2000);

       Serial.println("Preparing photo");
       sendPhotoTelegram(); 
       Serial.println("Ready");
     }
    }   
 if(digitalRead(mos) == HIGH)
   {
    moss='d'; 
    sts1=0;
   }
*/
   
/*
  if(motionDetected){
    bot.sendMessage(chatId, "Motion detected!!", "");
    Serial.println("Motion Detected");
    sendPhotoTelegram();
    motionDetected = false;
  }
*/

  
   if(millis() > lastTimeBotRan + botRequestDelay)
     {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
       while(numNewMessages)
            {
              Serial.println("got response");
              handleNewMessages(numNewMessages);
              numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            }
      lastTimeBotRan = millis();
     }
  
     
}

String sendPhotoTelegram()
{
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void handleNewMessages(int numNewMessages)
{
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++){
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String fromName = bot.messages[i].from_name;

    if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    }
    if (text == "/photo") {
      sendPhoto = true;
      Serial.println("New photo  request");
    }
    if (text == "/location"){
      String readings = getReadings();
      bot.sendMessage(chatId, readings, "");
    }
    if (text == "/speaker"){
      //String readings = getReadings();
      //bot.sendMessage(chatId, speaker, "");
      speakersend = true;
    }
    if (text == "/start"){
      String welcome = "Welcome to the ESP32-CAM Telegram bot.\n";
      welcome += "/photo : takes a new photo\n";
      welcome += "/flash : toggle flash LED\n";
      welcome += "/location : GPS Location\n";
      welcome += "You'll receive a photo whenever motion is detected.\n";
      bot.sendMessage(chatId, welcome, "Markdown");
    }
  }
}
