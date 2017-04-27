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

#define PIN_CH1   15
#define PIN_CH2   13
#define PIN_CH3   12
#define PIN_CH4   14
#define PIN_CH5    4
#define LED_RED    5
#define LED_GREEN  1
#define CHANNELS   5
#define TIMEOUT   5000

static const uint16_t gammatable[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,
    3,  3,  4,  4,  4,  5,  5,  5,  6,  6,  7,  7,  7,  8,  8,  9,
   10, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 17, 18, 19, 20, 20,
   21, 22, 23, 24, 25, 26, 27, 29, 30, 31, 32, 33, 35, 36, 37, 38,
   40, 41, 43, 44, 46, 47, 49, 50, 52, 54, 55, 57, 59, 61, 63, 64,
   66, 68, 70, 72, 74, 77, 79, 81, 83, 85, 88, 90, 92, 95, 97,100,
  102,105,107,110,113,115,118,121,124,127,130,133,136,139,142,145,
  149,152,155,158,162,165,169,172,176,180,183,187,191,195,199,203,
  207,211,215,219,223,227,232,236,240,245,249,254,258,263,268,273,
  277,282,287,292,297,302,308,313,318,323,329,334,340,345,351,357,
  362,368,374,380,386,392,398,404,410,417,423,429,436,442,449,455,
  462,469,476,483,490,497,504,511,518,525,533,540,548,555,563,571,
  578,586,594,602,610,618,626,634,643,651,660,668,677,685,694,703,
  712,721,730,739,748,757,766,776,785,795,804,814,824,833,843,853,
  863,873,884,894,904,915,925,936,946,957,968,979,990,1001,1012,1023 };


const char* ssid = "eincost";
const char* password = "costumes2342";
WiFiUDP Udp;
unsigned int localUdpPort = 4210;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets
uint8_t channel[5];
unsigned long lastTimeReceivedData = 0;

void dbg_printf ( const char *format, ... ) {

    static char sbuf[1400];                                                     // For debug lines
    va_list varArgs;                                                            // For variable number of params

    va_start ( varArgs, format );                                               // Prepare parameters
    vsnprintf ( sbuf, sizeof ( sbuf ), format, varArgs );                       // Format the message
    va_end ( varArgs );                                                         // End of using parameters

    Serial.print ( sbuf );

}

void eventWiFi(WiFiEvent_t event) {
     
  switch(event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
      dbg_printf("[WiFi] %d, Connected\n", event);
    break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      dbg_printf("[WiFi] %d, Disconnected - Status %d, %s\n", event, WiFi.status(), connectionStatus( WiFi.status() ).c_str() );      
    break;    
     case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
      dbg_printf("[WiFi] %d, AuthMode Change\n", event);
    break;    
    case WIFI_EVENT_STAMODE_GOT_IP:
      dbg_printf("[WiFi] %d, Got IP\n", event);
    break;    
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      dbg_printf("[WiFi] %d, DHCP Timeout\n", event);
    break;    
    case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
      dbg_printf("[AP] %d, Client Connected\n", event);
    break;    
    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
      dbg_printf("[AP] %d, Client Disconnected\n", event);
    break;    
    case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
//      dbg_printf("[AP] %d, Probe Request Recieved\n", event);
    break;
  }
  
}

String connectionStatus ( int which ) {
    switch ( which ) {
        case WL_CONNECTED: return "Connected";
        case WL_NO_SSID_AVAIL: return "Network not availible";
        case WL_CONNECT_FAILED: return "Wrong password";
        case WL_IDLE_STATUS: return "Idle status";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Unknown";
    }
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  
  pinMode(PIN_CH1, OUTPUT);
  pinMode(PIN_CH2, OUTPUT);
  pinMode(PIN_CH3, OUTPUT);
  pinMode(PIN_CH4, OUTPUT);
  pinMode(PIN_CH5, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  Serial.printf("Connecting to %s ", ssid);
  IPAddress ip(192, 168, 80, 112);
  IPAddress gateway(192, 168, 80, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet);
  WiFi.onEvent(eventWiFi);
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    digitalWrite(LED_GREEN, LOW);
    delay(250);
    Serial.print(".");
    digitalWrite(LED_GREEN, HIGH);
  }
  Serial.println(" connected");
  
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  

  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

  ArduinoOTA.begin();
}


void loop() {
  ArduinoOTA.handle();
  
  int packetSize = Udp.parsePacket();
  
  if (packetSize > 0) {
    digitalWrite(LED_GREEN, HIGH);
    
    int len = Udp.read(incomingPacket, 255);
    
    if (len > 0) {
      incomingPacket[len] = 0;  // to have a null terminated string
      handleCommandReceived(incomingPacket, len);
      lastTimeReceivedData = millis();
    }

    digitalWrite(LED_GREEN, LOW);
  }

  if(millis() - lastTimeReceivedData > TIMEOUT) {
    for(int i=0;i<CHANNELS;i++) {
      channel[i] = 0;
    }
    output();
  }
  
}

void handleCommandReceived(char *buffer, int len) {

  if(len == CHANNELS) {

      for(int i=0;i<CHANNELS;i++) {
        channel[i] = buffer[i];
        if(channel[i] > 255) channel[i] = 255;
      }

      output();
  }
}

void output() {
  analogWrite(PIN_CH1, gammatable[channel[0]]);
  analogWrite(PIN_CH2, gammatable[channel[1]]);
  analogWrite(PIN_CH3, gammatable[channel[2]]);
  analogWrite(PIN_CH4, gammatable[channel[3]]);
  analogWrite(PIN_CH5, gammatable[channel[4]]);
}

