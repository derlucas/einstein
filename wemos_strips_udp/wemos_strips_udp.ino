/**
 * Arduino IDE instructions
 * 
 * select "generic ESP8266 Module"
 * 80Mhz CPU, 40Mhz Flash
 * Upload Speed 921600
 * 1M (64K SPIFFS)
 * Flash Mode QIO
 * 
 */
 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

#define PIN           D1
#define TIMEOUT       5000
#define RECV_BUFLEN   1024
#define LEDS          170
#define MILLIWATT_PER_COLOR   72
#define MILLIWATT_MAX   25000

const uint8_t gammatable[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
        2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
        5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
       10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
       17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
       25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
       37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
       51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
       69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
       90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
      115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
      144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
      177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
      215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "eincost";
const char* password = "costumes2342";
WiFiUDP Udp;
unsigned int localUdpPort = 4210;  // local port to listen on
char incomingPacket[RECV_BUFLEN];  // buffer for incoming packets
unsigned long lastTimeReceivedData = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
 
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  uint8_t i = 0;
  strip.begin();

  allLeds(10,0,0);
  delay(1000);
  allLeds(0,10,0);
  delay(1000);
  allLeds(0,0,10);
  delay(1000);

  while (WiFi.status() != WL_CONNECTED) {
    strip.setPixelColor(i++, 255,0,0);
    strip.show();
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  delay(500);
  allLeds(0,20,4);
  
  strip.show();
  
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

  ArduinoOTA.begin();
}


void loop() {
  ArduinoOTA.handle();
  
  uint16_t packetSize = Udp.parsePacket();
  
  if (packetSize > 0) {    
    uint16_t len = Udp.read(incomingPacket, RECV_BUFLEN);
    
    if (len > 0) {
      incomingPacket[len] = 0; // null terminated string

      handleCommandReceived(incomingPacket, len);
      lastTimeReceivedData = millis();
    }
  }

  if(millis() - lastTimeReceivedData > TIMEOUT) {
    allLeds(0,0,0);
  }
  
}

void allLeds(uint8_t red, uint8_t green, uint8_t blue) {
  for(int i=0;i< strip.numPixels(); i++) {
    strip.setPixelColor(i, red, green, blue);
  }
  strip.show();
}

void handleCommandReceived(char *buffer, uint16_t len) {  
  if(len == (strip.numPixels() * 3)) {
      float powerConsumption = 0.0f;
      
      for(int i=0;i< strip.numPixels(); i++) {

        byte red = gammatable[buffer[i*3]];
        byte green = gammatable[buffer[i*3+1]];
        byte blue = gammatable[buffer[i*3+2]];
        
        strip.setPixelColor(i, red, green, blue);
        powerConsumption += MILLIWATT_PER_COLOR * (red/255.0f);
        powerConsumption += MILLIWATT_PER_COLOR * (green/255.0f);
        powerConsumption += MILLIWATT_PER_COLOR * (blue/255.0f);
      }

//      Serial.printf("power: %d\r\n", (int)(powerConsumption));

      // if we set more power than allowed, we will dim the whole strip
      int overpower = (int)powerConsumption - MILLIWATT_MAX;
      if(overpower > 0) {
        strip.setBrightness(255 * MILLIWATT_MAX / powerConsumption);
      }
      
      strip.show();
  }
}



uint8_t CRCCalc(char* pointer, uint16_t len) {
    uint8_t CRC = 0x00;
    uint16_t tmp;

    while(len > 0) {
        tmp = CRC << 1;
        tmp += *pointer;
        CRC = (tmp & 0xFF) + (tmp >> 8);
        pointer++;
        --len;
    }

    return CRC;
}