/* * Board: DOIT ESP32 DEVKIT V1, 80Mhz, 4MB(32Mhz),921600 None op COM3 */
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "credentials.h" // contains passwords
#include "vars.h"        // set the variables
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

/** Declare functions for c++ the functions are below the main loop */
void callback(char *topic, byte *payload, unsigned int length);
void connect_MQTT();
void IRAM_ATTR addLiter();
void SendMQTT(int waterCount, String tempVal);
void setupPins();
void setupOTA();
void setupWiFi();

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

WiFiServer server(80);
WiFiClient wifiClient;
// MQTT Client
PubSubClient mqttClient(wifiClient);

// set up everything
void setup(void)
{
  Serial.begin(115200);
  setupWiFi();
  setupOTA();
  mqttClient.setServer(ip_MQTT, port_MQTT);
  connect_MQTT();
  SendMQTT(0, tempWard);
  setupPins();
  attachInterrupt(digitalPinToInterrupt(PIN27_DETECT), addLiter, RISING);
  server.begin();
}

/* main loop */
void loop(void)
{
  ArduinoOTA.handle(); // Over The Air update see platformio.ini for settings
  timeClient.update(); // update the time
  String formattedTime = timeClient.getFormattedTime();
  // at 7 minutes before 04 AM restart the ESP
  if (formattedTime >= "03:53:00" && formattedTime <= "03:53:02")
  {
    ESP.restart();
  }

  mqttClient.loop();
  // make sure MQTT stays connected
  if (!mqttClient.connected())
  {
    connect_MQTT();
  }
  // if we don't have a starting watermeter Value request from openHAB
  if (waterMeterValue < 10)
  {
    SendMQTT(0, "request");
    delay(500);
    SendMQTT(1, "request");
  }
  // Usage send directly the
  if (waterMeterValue > previousWater)
  {
    // A metervalue of 0 can't be send
    if (waterMeterValue > 0)
    {
      // it is not possible to tap more then 2 liters a second
      if (waterMeterValue - previousWater > 2)
      {
        waterMeterValue = previousWater + 2;
      }
      SendMQTT(waterMeterValue, "puls");
      previousWater = waterMeterValue;
    }
  }
  // every 10 minutes send the last value
  if (formattedTime.substring(3, 5).toInt() % 10 == 0 &&
      formattedTime.substring(6, 8).toInt() < 2)
  {
    SendMQTT(waterMeterValue, "puls");
  }
  // dealy a littele less then 1 second
  delay(1000);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("-------new message from broker--> ");
  Serial.print(" channel:");
  Serial.print(topic);
  Serial.print(" data:");
  Serial.write(payload, length);
  Serial.print(" length: ");
  Serial.println(length);
  String str = "";
  int punt = 0;
  int crCnt = 0;
  for (uint8_t i = 0; i < length; i++)
  {
    if (payload[i] == 46)
    {
      punt = 1;
    }
    if (crCnt < 2)
    {
      str += char(payload[i]);
    }
    if (punt == 1)
    {
      crCnt = crCnt + 1;
    }
  }

  String setTemp = str;
  waterMeterValue = str.toInt();
  if (waterMeterValue < 201000)
  {
    waterMeterValue = 0;
  }
  previousWater = waterMeterValue;
  String txt = topic;
  txt = txt.substring(8, txt.length());
  String setText = txt;
  Serial.print("Received value from openHAB is " + (String)waterMeterValue + " liters");
  Serial.println("");
}

void connect_MQTT()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    // Attempt to connect
    // Create a random client ID
    Serial.println("");
    Serial.print("Connecting to MQTT ...");
    String clientId = "Water-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), my_MQTT_USER, my_MQTT_PW))
    {
      Serial.println("..Yes we did!");
      mqttClient.setCallback(callback);
      mqttClient.subscribe(mqttTopicSubscribe);
      mqttClient.subscribe(mqttTopicSubscriber);
      SendMQTT(0, "request");
    }
    else
    {
      Serial.println(" .. Failed to connect to MQTT ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void IRAM_ATTR addLiter()
{
  waterMeterValue++;
}

void SendMQTT(int waterCount, String tempVal)
{
  digitalWrite(ONBOARD_LED, HIGH);
  mqttClient.setServer(ip_MQTT, port_MQTT);
  while (!mqttClient.connected())
  {
    Serial.println("Connecting to MQTT...");
    String clientId = "Water-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), my_MQTT_USER, my_MQTT_PW))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(1000);
    }
  }

  if (tempVal == "puls")
  {
    if (waterCount < 201000)
    {
      return;
    }

    Topic = "water_puls";
    valToSend = waterCount;
  }
  else if (tempVal == "request")
  {
    Topic = "water_request";
    if (waterCount == 0)
    {
      valToSend = "0";
    }
    else
    {
      valToSend = "1";
    }
  }
  else if (tempVal == "testen")
  {
    Topic = "water_test";
    valToSend = waterCount;
  }

  int msgLen = valToSend.length();
  mqttClient.beginPublish(Topic, msgLen, false);
  mqttClient.print(valToSend);
  mqttClient.endPublish();
  digitalWrite(ONBOARD_LED, LOW);
  //mqttClient.disconnect();
}

void setupOTA()
{
  ArduinoOTA.setHostname("Water-Meter");
  ArduinoOTA.setPassword(my_OTA_PW);

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();
}

void setupPins()
{
  pinMode(PIN27_DETECT, INPUT);
  pinMode(ONBOARD_LED, OUTPUT);
}

void setupWiFi()
{
  // start connection to WiFi network with a fixed IPaddress
  Serial.println();
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure");
  }
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Enter this address in your Internet browser.");
}
