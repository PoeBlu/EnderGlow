#include <ESP8266WiFi.h>
#include <WS2812FX.h>
#include "SSD1306Wire.h" 
#include "Images.h"
#include "Fonts.h"
#include <String.h>

#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <Thread.h>             
#include <ThreadController.h>

#define FACTORYRESET_ENABLE     1
#define WIFI_SSID "<SSID>" //WiFi AP
#define WIFI_PASS "<PASSWORD>" //WiFi Password

#define MQTT_SERVER "io.adafruit.com" // used io.adafruit.com for testing
#define MQTT_PORT 1883 // Default MQTT Port is 1883
#define MQTT_USER "<USER>" // adafruit or other MQTT server user
#define MQTT_PASS "<KEY>" // API Key, or Adafruit IO Key
#define MQTT_FEED "<USER>/feeds/<FEEDNAME>" //feed URL - example for Adafruit IO

//WIFI KIT 8 - Pins for OLED
#define pinSCL 5
#define pinSDA 4
#define pinReset 16

//WEMOS D1 Mini
//#define pinSCL D1
//#define pinSDA D2
//#define pinReset 16


#define LED_COUNT 19 // Number of LEDs in your strip
#define LED_PIN D3 //LED PIN
#define TIMER_MS 5000

//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, espClient);
PubSubClientTools mqtt(client);
ThreadController threadControl = ThreadController();
Thread thread = Thread();

int value = 0;
String s = "";
unsigned long last_change = 0;
unsigned long now = 0;
//Set the Geometry to your screen size
SSD1306Wire display(0x3c, pinSDA, pinSCL, GEOMETRY_128_32);

String lastMessage = "";
String stringMessage = "";
int pos = 0, dir = 1;
String dataRX[3];

void setup()
{
  delay(500);

  //this may be required for some displays
  pinMode(pinReset, OUTPUT);
  digitalWrite(pinReset, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, pinReset); // while OLED is running, must set GPIO16 in high、
  
  display.init(); 
  // If your screen is upside down, uncomment the line below
  //display.flipScreenVertically();
  
  //draw the Splash Screen
  drawSplash();
  
  Serial.begin(115200);

  // Connect to WiFi
  ConnectWIFI();

  // Connect to the MQTT Service
  ConnectMQTT();
  ws2812fx.init();
  ws2812fx.setBrightness(255);
  ws2812fx.setSpeed(1000);
  ws2812fx.setMode(FX_MODE_COLOR_WIPE);
  ws2812fx.setColor(BLACK);
  ws2812fx.start();
}

void ConnectMQTT()
{
// Connect to MQTT
  Serial.print(s + "Connecting to MQTT: " + MQTT_SERVER + " ... ");
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("connected");
    drawGoogle();
    //here, you can set up multiple subscriptions and callback functions
    //mqtt.subscribe(<Feed Name>, <Callback>)
    mqtt.subscribe(MQTT_FEED, topic_subscriber);
  } else {
    Serial.println(s + "failed, rc=" + client.state());
    drawGoogle();
  }

  // Enable Thread for publishing and receiving messages
  thread.onRun(publisher);
  thread.setInterval(2000);
  threadControl.add(&thread);
}

void ConnectWIFI()
{
  Serial.print(s + "Connecting to WiFi: " + WIFI_SSID + " ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    drawWIFI();
  }
  drawWIFI();
}

void loop()
{

  client.loop();
  threadControl.run();
  now = millis();

  //start the service
  ws2812fx.service();

  if(!WiFi.status() == WL_CONNECTED)
  {
    drawWIFI();
    ConnectWIFI();
  }
  //ping the server to keep the mqtt connection alive
  if (!client.connected() && (WiFi.status() == WL_CONNECTED))
  {
    client.disconnect();
    drawGoogle();
    ConnectMQTT();
  }
}

void drawCommand() {
  if(WiFi.status() == WL_CONNECTED)
    {
        display.clear();
        drawGoogle();
        drawWIFI();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        //display.setFont(Roboto_9);
        //display.drawString(64, 0, "COLOR");
        display.setFont(Roboto_28);
        
        //if(message.length() < 11)
        //{
        //  display.setFont(Roboto_22);
        //}
        
        display.drawString(64, 0, CapitalizeString(dataRX[1]));
        display.display();
    }
 }

void drawSplash() {
  //Draw the Splash Screen
  display.clear();
  display.drawXbm(0, 8, splash_width, splash_height, splash_bits);
  display.display();
}


void drawGoogle() {
  //Draw a Google Home icon if connected.  Ugly code here
  if(client.connected())
  {
    display.drawXbm(0, 0, google_width, google_height, google_bits);
  }
  else
  {
    display.drawXbm(0, 0, clear_width, clear_height, clear_bits);
  }
  display.display();
}

void drawWIFI() {
  //Draw a WiFi icon if connected.  This code needs cleanup
  if(WiFi.status() == WL_CONNECTED)
  {
    display.drawXbm(120, 0, wifi_width, wifi_height, wifi_bits);
  }
  else
  {
    display.drawXbm(120, 0, clear_width, clear_height, clear_bits);
  }
  display.display();
}

void publisher() {
  //used to publish data to your feed
}

String CapitalizeString(String &str) {
  //Just a way to clean up messages for presentation on the OLED or 3D LCD Screen
  String firstChar = str.substring(0,1);
  firstChar.toUpperCase();
  String remainingChars = str.substring(1);
  return firstChar + remainingChars;
}

void SetLightPattern(String lightPattern) {
  //These are the built-in functions for the WS2812FX Library.  You can extend this if needed
  if (lightPattern.equalsIgnoreCase("LARSON SCANNER")) {
    ws2812fx.setMode(FX_MODE_LARSON_SCANNER);
  }
  if (lightPattern.equalsIgnoreCase("SOLID")) {
    ws2812fx.setMode(FX_MODE_COLOR_WIPE);
  }
  if (lightPattern.equalsIgnoreCase("COLOR WIPE INVERSE")) {
    ws2812fx.setMode(FX_MODE_COLOR_WIPE_INV);
  }
  if (lightPattern.equalsIgnoreCase("COLOR WIPE REVERSE")) {
    ws2812fx.setMode(FX_MODE_COLOR_WIPE_REV);
  }
  if (lightPattern.equalsIgnoreCase("COLOR WIPE REVERSE INVERSE")) {
    ws2812fx.setMode(FX_MODE_COLOR_WIPE_REV_INV);
  }
  if (lightPattern.equalsIgnoreCase("COLOR WIPE RANDOM")) {
    ws2812fx.setMode(FX_MODE_COLOR_WIPE_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("RANDOM COLOR")) {
    ws2812fx.setMode(FX_MODE_RANDOM_COLOR);
  }
  if (lightPattern.equalsIgnoreCase("HALLOWEEN")) {
    ws2812fx.setMode(FX_MODE_HALLOWEEN);
  }
  if (lightPattern.equalsIgnoreCase("SINGLE DYNAMIC")) {
    ws2812fx.setMode(FX_MODE_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("MULTI DYNAMIC")) {
    ws2812fx.setMode(FX_MODE_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("RAINBOW")) {
    ws2812fx.setMode(FX_MODE_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("RAINBOW CYCLE")) {
    ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
  }
  if (lightPattern.equalsIgnoreCase("SCAN")) {
    ws2812fx.setMode(FX_MODE_SCAN);
  }
  if (lightPattern.equalsIgnoreCase("DUAL SCAN")) {
    ws2812fx.setMode(FX_MODE_DUAL_SCAN);
  }
  if (lightPattern.equalsIgnoreCase("FADE")) {
    ws2812fx.setMode(FX_MODE_FADE);
  }
  if (lightPattern.equalsIgnoreCase("THEATER CHASE")) {
    ws2812fx.setMode(FX_MODE_THEATER_CHASE);
  }
  if (lightPattern.equalsIgnoreCase("THEATER CHASE RAINBOW")) {
    ws2812fx.setMode(FX_MODE_THEATER_CHASE_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("RUNNING LIGHTS")) {
    ws2812fx.setMode(FX_MODE_RUNNING_LIGHTS);
  }
  if (lightPattern.equalsIgnoreCase("TWINKLE")) {
    ws2812fx.setMode(FX_MODE_TWINKLE);
  }
  if (lightPattern.equalsIgnoreCase("TWINKLE RANDOM")) {
    ws2812fx.setMode(FX_MODE_TWINKLE_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("TWINKLE FADE")) {
    ws2812fx.setMode(FX_MODE_TWINKLE_FADE);
  }
  if (lightPattern.equalsIgnoreCase("TWINKLE FADE RANDOM")) {
    ws2812fx.setMode(FX_MODE_TWINKLE_FADE_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("SPARKLE")) {
    ws2812fx.setMode(FX_MODE_SPARKLE);
  }
  if (lightPattern.equalsIgnoreCase("FLASH SPARKLE")) {
    ws2812fx.setMode(FX_MODE_FLASH_SPARKLE);
  }
  if (lightPattern.equalsIgnoreCase("HYPER SPARKLE")) {
    ws2812fx.setMode(FX_MODE_HYPER_SPARKLE);
  }
  if (lightPattern.equalsIgnoreCase("STROBE")) {
    ws2812fx.setMode(FX_MODE_STROBE);
  }
  if (lightPattern.equalsIgnoreCase("STROBE RAINBOW")) {
    ws2812fx.setMode(FX_MODE_STROBE_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("MULTI STROBE")) {
    ws2812fx.setMode(FX_MODE_MULTI_STROBE);
  }
  if (lightPattern.equalsIgnoreCase("BLINK RAINBOW")) {
    ws2812fx.setMode(FX_MODE_BLINK_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("CHASE WHITE")) {
    ws2812fx.setMode(FX_MODE_CHASE_WHITE);
  }
  if (lightPattern.equalsIgnoreCase("CHASE COLOR")) {
    ws2812fx.setMode(FX_MODE_CHASE_COLOR);
  }
  if (lightPattern.equalsIgnoreCase("CHASE RANDOM")) {
    ws2812fx.setMode(FX_MODE_CHASE_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("CHASE RAINBOW")) {
    ws2812fx.setMode(FX_MODE_CHASE_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("CHASE FLASH")) {
    ws2812fx.setMode(FX_MODE_CHASE_FLASH);
  }
  if (lightPattern.equalsIgnoreCase("CHASE FLASH RANDOM")) {
    ws2812fx.setMode(FX_MODE_CHASE_FLASH_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("CHASE RAINBOW WHITE")) {
    ws2812fx.setMode(FX_MODE_CHASE_RAINBOW_WHITE);
  }
  if (lightPattern.equalsIgnoreCase("CHASE BLACKOUT")) {
    ws2812fx.setMode(FX_MODE_CHASE_BLACKOUT);
  }
  if (lightPattern.equalsIgnoreCase("CHASE BLACKOUT RAINBOW")) {
    ws2812fx.setMode(FX_MODE_CHASE_BLACKOUT_RAINBOW);
  }
  if (lightPattern.equalsIgnoreCase("COLOR SWEEP RANDOM")) {
    ws2812fx.setMode(FX_MODE_COLOR_SWEEP_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("RUNNING COLOR")) {
    ws2812fx.setMode(FX_MODE_RUNNING_COLOR);
  }
  if (lightPattern.equalsIgnoreCase("RUNNING RED BLUE")) {
    ws2812fx.setMode(FX_MODE_RUNNING_RED_BLUE);
  }
  if (lightPattern.equalsIgnoreCase("RUNNING RANDOM")) {
    ws2812fx.setMode(FX_MODE_RUNNING_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("COMET")) {
    ws2812fx.setMode(FX_MODE_COMET);
  }
  if (lightPattern.equalsIgnoreCase("FIREWORKS")) {
    ws2812fx.setMode(FX_MODE_FIREWORKS);
  }
  if (lightPattern.equalsIgnoreCase("FIREWORKS RANDOM")) {
    ws2812fx.setMode(FX_MODE_FIREWORKS_RANDOM);
  }
  if (lightPattern.equalsIgnoreCase("MERRY CHRISTMAS")) {
    ws2812fx.setMode(FX_MODE_MERRY_CHRISTMAS);
  }
  if (lightPattern.equalsIgnoreCase("FIRE FLICKER")) {
    ws2812fx.setMode(FX_MODE_FIRE_FLICKER);
  }
  if (lightPattern.equalsIgnoreCase("FIRE FLICKER SOFT")) {
    ws2812fx.setMode(FX_MODE_FIRE_FLICKER_SOFT);
  }
  if (lightPattern.equalsIgnoreCase("FIRE FLICKER INTENSE")) {
    ws2812fx.setMode(FX_MODE_FIRE_FLICKER_INTENSE);
  }
  if (lightPattern.equalsIgnoreCase("CIRCUS COMBUSTUS")) {
    ws2812fx.setMode(FX_MODE_CIRCUS_COMBUSTUS);
  }
  if (lightPattern.equalsIgnoreCase("BICOLOR CHASE")) {
    ws2812fx.setMode(FX_MODE_BICOLOR_CHASE);
  }
  if (lightPattern.equalsIgnoreCase("ICU")) {
    ws2812fx.setMode(FX_MODE_ICU);
  }
  if (lightPattern.equalsIgnoreCase("BLINK")) {
    ws2812fx.setMode(FX_MODE_BLINK);
  }
  if (lightPattern.equalsIgnoreCase("TRICOLOR CHASE")) {
    ws2812fx.setMode(FX_MODE_TRICOLOR_CHASE);
  }
  if (lightPattern.equalsIgnoreCase("BREATH")) {
    ws2812fx.setMode(FX_MODE_BREATH);
  }
  if (lightPattern.equalsIgnoreCase("RANDOM COLOR")) {
    ws2812fx.setMode(FX_MODE_RANDOM_COLOR);
  }
}

void SetLightColor(String lightColor)
{
  //More colors can be defined in the WS2812FX.h file, or you can set them manually via HEX or RGB
  if (lightColor.equalsIgnoreCase("RED")) {
    ws2812fx.setColor(RED);
  }
  if (lightColor.equalsIgnoreCase("GREEN")) {
    ws2812fx.setColor(GREEN);
  }
  if (lightColor.equalsIgnoreCase("BLUE")) {
    ws2812fx.setColor(BLUE);
  }
  if (lightColor.equalsIgnoreCase("PURPLE")) {
    ws2812fx.setColor(PURPLE);
  }
  if (lightColor.equalsIgnoreCase("ORANGE")) {
    ws2812fx.setColor(ORANGE);
  }
  if (lightColor.equalsIgnoreCase("YELLOW")) {
    ws2812fx.setColor(YELLOW);
  }
  if (lightColor.equalsIgnoreCase("CYAN")) {
    ws2812fx.setColor(CYAN);
  }
  if (lightColor.equalsIgnoreCase("PINK")) {
    ws2812fx.setColor(PINK);
  }
  if (lightColor.equalsIgnoreCase("BLACK")) {
    ws2812fx.setColor(BLACK);
  }
  if (lightColor.equalsIgnoreCase("OFF")) {
    ws2812fx.setColor(BLACK);
  }
  if (lightColor.equalsIgnoreCase("WHITE")) {
    ws2812fx.setColor(WHITE);
  }
  if (lightColor.equalsIgnoreCase("BRIGHT WHITE")) {
    ws2812fx.setColor(ULTRAWHITE);
  }
}

void topic_subscriber(String topic, String message) {
  Serial.println(s + "Message arrived in function 1 [" + topic + "] " + CapitalizeString(message));
  lastMessage = message;
  //Split the response into an array, check the first member to see the action type
  SplitString(message, ":");
  
  if (dataRX[0].equalsIgnoreCase("PATTERN")) {
    drawCommand();
    SetLightPattern(dataRX[1]);
  }
  if (dataRX[0].equalsIgnoreCase("COLOR")) {
    drawCommand();
    SetLightColor(dataRX[1]);
  }  
}

void SplitString(String str, char* delimiter)
{
  //used for parsing data from the MQTT feed
  //Currently set to see a command come back as one of the following:
  //COLOR:<COLOR NAME>
  //PATTERN:<PATTERN NAME>
  int strLength = str.length() + 1; 
  char strArray[strLength];
  str.toCharArray(strArray, strLength);
      
  char * pch;
  pch = strtok (strArray, delimiter);
  int i = 0;
  while (pch != NULL) {
    dataRX[i] = pch;
    i++;
    pch = strtok (NULL, delimiter);
  }
  if(i>=2)
  {
    //clean buffer values
    for(int j=i;j<5;j++) dataRX[j] = "";
  }
}