#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 1024
#endif

#define PIN27_DETECT 34
#define ONBOARD_LED 2

#define NTP_OFFSET 2 * 60 * 60 // In seconds
#define NTP_INTERVAL 60 * 1000 // In miliseconds
#define NTP_ADDRESS "ntp2.xs4all.nl"
// Buffers for JSON
//const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3);
//DynamicJsonBuffer jsonBuffer(bufferSize);

//variables

const char *ssid = my_SSID;
const char *password = my_PASSWORD;

const char *assid = ap_SSID;
const char *asecret = ap_SECRET;

IPAddress local_IP(192, 168, 178, 208);
// Set your Gateway IP address
IPAddress gateway(192, 168, 178, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

char const *mqttTopicPublish = "water_request";
char const *mqttTopicSubscribe = "water_puls";
char const *mqttTopicSubscriber = "openHAB/water_set";
const char *Topic = "none";
const char *tempWard = "request";

int waterMeterValue = 0;
int previousWater = 0;
String valToSend = "0";

int previous_Monitoring = 0;
int monitoring = 0;

String formattedTime = "";
String formattedDate = "";
String version = "0.1.0";