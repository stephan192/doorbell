#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include "Sip.h"
#include "config.h"

#define HW_VERSION "v2"
#define SW_VERSION "v1.1"

#define IN1     12
#define IN2     13
#define OUT1    4
#define OUT2    5
#define OUT3    14
#define OUT4    15
#define LED     2

#define SIP1_ENABLED          0x0001u /* Button 1 dails SIP_DIAL_NR1 */
#define SIP2_ENABLED          0x0002u /* Button 2 dails SIP_DIAL_NR2 */
#define CHIME1_ENABLED        0x0004u /* Button 1 activates chime 1 (OUT3) for BUTTON_ACTION_TIME */
#define CHIME2_ENABLED        0x0008u /* Button 2 activates chime 2 (OUT4) for BUTTON_ACTION_TIME */
#define SIPLOCK_ENABLED       0x0010u /* Unlock door (OUT3) when SIP_DIAL_DIGIT is received via SIP */
#define MQTTLOCK_ENABLED      0x0020u /* Unlock door (OUT3) when corresponding command is received via MQTT */
#define MQTTBELL1_ENABLED     0x0040u /* Button 1 publishes device trigger to corresponding MQTT topic */
#define MQTTBELL2_ENABLED     0x0080u /* Button 2 publishes device trigger to corresponding MQTT topic */
#define MQTTBUTTON1_ENABLED   0x0100u /* Button 1 can be enabled/disabled via MQTT */
#define MQTTBUTTON2_ENABLED   0x0200u /* Button 2 can be enabled/disabled via MQTT */

#define BUTTON1_ENABLED       (SIP1_ENABLED|CHIME1_ENABLED|MQTTBELL1_ENABLED)
#define BUTTON2_ENABLED       (SIP2_ENABLED|CHIME2_ENABLED|MQTTBELL2_ENABLED)
#define SIP_ENABLED           (SIP1_ENABLED|SIP2_ENABLED)
#define LOCK_ENABLED          (SIPLOCK_ENABLED|MQTTLOCK_ENABLED)
#define MQTT_ENABLED          (MQTTLOCK_ENABLED|MQTTBELL1_ENABLED|MQTTBELL2_ENABLED|MQTTBUTTON1_ENABLED|MQTTBUTTON2_ENABLED)

#if ((DEVICE_CONFIG&CHIME1_ENABLED)&&(DEVICE_CONFIG&LOCK_ENABLED))
#error Combination not allowed!
#endif
#if ((!(DEVICE_CONFIG&SIP1_ENABLED))&&(!(DEVICE_CONFIG&SIP2_ENABLED))&&(DEVICE_CONFIG&SIPLOCK_ENABLED))
#error Combination not allowed!
#endif


#if (DEVICE_CONFIG&SIP_ENABLED)
const char* sipip = SIP_IP;
const int   sipport = SIP_PORT;
const char* sipuser = SIP_USER;
const char* sippasswd = SIP_PASS;

const char* sipdialnr1 = SIP_DIAL_NR1;
const char* sipdialnr2 = SIP_DIAL_NR2;
const char* sipsenderdesc = HOSTNAME;
const int   sipdigit = SIP_DIAL_DIGIT;

char caSipIn[2048];
char caSipOut[2048];

Sip aSip(caSipOut, sizeof(caSipOut));

char ip[17];
bool bInDial = false;
#endif

WiFiClient esp_client;
uint32_t ReconnectCounter = 0;

#if (DEVICE_CONFIG&MQTT_ENABLED)
PubSubClient mqtt_client(MQTT_SERVER, MQTT_PORT, esp_client);
PubSubClientTools mqtt(mqtt_client);
String unique_id;
String avty_topic;
#endif

#if (DEVICE_CONFIG&MQTTLOCK_ENABLED)
String lock_cmd_topic;
bool mqtt_lock_trigger = false;
#endif
#if (DEVICE_CONFIG&MQTTBELL1_ENABLED)
String bell1_action_topic;
#endif
#if (DEVICE_CONFIG&MQTTBELL2_ENABLED)
String bell2_action_topic;
#endif
#if (DEVICE_CONFIG&MQTTBUTTON1_ENABLED)
String button1_state_topic;
String button1_cmd_topic;
bool button1_enabled = true;
#endif
#if (DEVICE_CONFIG&MQTTBUTTON2_ENABLED)
String button2_state_topic;
String button2_cmd_topic;
bool button2_enabled = true;
#endif


void setup() {
  // Init pins =======================================================================================
  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP);
  pinMode(OUT1, OUTPUT);
  digitalWrite(OUT1, LOW);
  pinMode(OUT2, OUTPUT);
  digitalWrite(OUT2, LOW);
  pinMode(OUT3, OUTPUT);
  digitalWrite(OUT3, LOW);
  pinMode(OUT4, OUTPUT);
  digitalWrite(OUT4, LOW);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Configure Uart ==================================================================================
  Serial.begin(115200);
  Serial.println();

  // Configure WiFi ==================================================================================
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("connected");

  // Configure OTA ===================================================================================
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORT);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.print("OTA ready - IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to SIP server ===========================================================================
  #if (DEVICE_CONFIG&SIP_ENABLED)
  strcpy(ip, WiFi.localIP().toString().c_str());
  aSip.Init(sipip, sipport, ip, sipport, sipuser, sippasswd, SIP_MAX_DAIL_TIME);
  #endif

  // Connect to MQTT server ==========================================================================
  #if (DEVICE_CONFIG&MQTT_ENABLED)
  byte mac[6];
  WiFi.macAddress(mac);
  unique_id = "doorbell_" + String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  unique_id.toLowerCase();
  Serial.print("MQTT client id: ");
  Serial.println(unique_id);
  setup_mqtt_topics();
  
  Serial.print("Connecting to MQTT: ");
  Serial.print(MQTT_SERVER);
  Serial.print("...");
  if (mqtt_client.connect(unique_id.c_str(), MQTT_USER, MQTT_PASSWORD, avty_topic.c_str(), 0, true, "offline")) {
    Serial.println("connected");
    publish_mqtt_autodiscovery();
    mqtt_init_publish_and_subscribe();
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqtt_client.state());
  }
  #endif
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
  {
    reconnect_wifi();
    if (WiFi.status() == WL_CONNECTED)
    {
      #if (DEVICE_CONFIG&SIP_ENABLED)
      strcpy(ip, WiFi.localIP().toString().c_str());
      aSip.Init(sipip, sipport, ip, sipport, sipuser, sippasswd, 15);
      #endif
    }
    else
    {
      ReconnectCounter++;
    }
  }
  else
  {
    // Read and process inputs =======================================================================
    #if (DEVICE_CONFIG&BUTTON1_ENABLED)
    handle_button1();
    #endif
    #if (DEVICE_CONFIG&BUTTON2_ENABLED)
    handle_button2();
    #endif

    // Handle sip communication ======================================================================
    #if (DEVICE_CONFIG&SIP_ENABLED)
    handle_sip();
    #endif

    // Control lock ==================================================================================
    #if (DEVICE_CONFIG&LOCK_ENABLED)
    handle_lock();
    #endif

    // Handle mqtt connection ========================================================================
    #if (DEVICE_CONFIG&MQTT_ENABLED)
    if (!mqtt_client.connected()) {
      reconnect_mqtt();
      ReconnectCounter++;
    }
    else
    {
      mqtt_client.loop();

      if (ReconnectCounter > 0) {
        ReconnectCounter--;
      }
    }
    #endif

    ArduinoOTA.handle();
  }

  if (ReconnectCounter > 100) {
    ESP.restart();
  }
}

void reconnect_wifi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  delay(500);
}

#if (DEVICE_CONFIG&BUTTON1_ENABLED)
void handle_button1() {
  int button1_in;
  static int button1_in_prev = HIGH;
  static unsigned long button1_time = 0u;
  static bool button1 = false;
  static bool button1_prev = false;
  #if ((DEVICE_CONFIG&CHIME1_ENABLED)||((DEVICE_CONFIG&MQTTBELL1_ENABLED)&&(!(DEVICE_CONFIG&SIP1_ENABLED))))
  static unsigned long button1Time = 0u;
  #endif
  
  button1_in = digitalRead(IN1);
  if (button1_in == LOW)
  {
    if (button1_in_prev == HIGH)
    {
      button1_time = millis();
    }
    if ((millis() - button1_time) > BUTTON_DEBOUNCE_TIME)
    {
      button1 = true;
    }
  }
  else
  {
    if (button1_in_prev == LOW)
    {
      button1_time = millis();
    }
    if ((millis() - button1_time) > BUTTON_DEBOUNCE_TIME)
    {
      button1 = false;
    }
  }
  button1_in_prev = button1_in;

  #if (DEVICE_CONFIG&MQTTBUTTON1_ENABLED)
  if ((button1)&&(!button1_prev)&&(button1_enabled))
  #else
  if ((button1)&&(!button1_prev))
  #endif
  {
    #if (DEVICE_CONFIG&SIP1_ENABLED)
    if ((!bInDial)&&(!aSip.IsBusy()))
    {
      digitalWrite(LED, LOW);
      digitalWrite(OUT1, HIGH);
      aSip.Dial(sipdialnr1, sipsenderdesc);
      bInDial = true;
    }
    #endif
    
    #if (DEVICE_CONFIG&CHIME1_ENABLED)
    #if (!(DEVICE_CONFIG&SIP1_ENABLED))
    digitalWrite(OUT1, HIGH);
    #endif
    digitalWrite(OUT3, HIGH);
    #endif

    #if (DEVICE_CONFIG&MQTTBELL1_ENABLED) 
    #if ((!(DEVICE_CONFIG&CHIME1_ENABLED))&&(!(DEVICE_CONFIG&SIP1_ENABLED)))
    digitalWrite(OUT1, HIGH);
    #endif
    mqtt.publish(bell1_action_topic, BELL_ACTION_CONTENT, false);
    #endif
    
    #if ((DEVICE_CONFIG&CHIME1_ENABLED)||((DEVICE_CONFIG&MQTTBELL1_ENABLED)&&(!(DEVICE_CONFIG&SIP1_ENABLED))))
    button1Time = millis();
    #endif
    Serial.println("Button 1 pressed");
  }
  #if ((DEVICE_CONFIG&CHIME1_ENABLED)||((DEVICE_CONFIG&MQTTBELL1_ENABLED)&&(!(DEVICE_CONFIG&SIP1_ENABLED))))
  if ((button1Time > 0u)&&((millis() - button1Time) > BUTTON_ACTION_TIME))
  {
    button1Time = 0u;
    #if (!(DEVICE_CONFIG&SIP1_ENABLED))
    digitalWrite(OUT1, LOW);
    #endif
    #if (DEVICE_CONFIG&CHIME1_ENABLED)
    digitalWrite(OUT3, LOW);
    #endif
    Serial.println("Button 1 end");
  }
  #endif
  button1_prev = button1;
}
#endif

#if (DEVICE_CONFIG&BUTTON2_ENABLED)
void handle_button2() {
  int button2_in;
  static int button2_in_prev = HIGH;
  static unsigned long button2_time = 0u;
  static bool button2 = false;
  static bool button2_prev = false;
  #if ((DEVICE_CONFIG&CHIME2_ENABLED)||((DEVICE_CONFIG&MQTTBELL2_ENABLED)&&(!(DEVICE_CONFIG&SIP2_ENABLED))))
  static unsigned long button2Time = 0u;
  #endif
  
  button2_in = digitalRead(IN2);
  if (button2_in == LOW)
  {
    if (button2_in_prev == HIGH)
    {
      button2_time = millis();
    }
    if ((millis() - button2_time) > BUTTON_DEBOUNCE_TIME)
    {
      button2 = true;
    }
  }
  else
  {
    if (button2_in_prev == LOW)
    {
      button2_time = millis();
    }
    if ((millis() - button2_time) > BUTTON_DEBOUNCE_TIME)
    {
      button2 = false;
    }
  }
  button2_in_prev = button2_in;

  #if (DEVICE_CONFIG&MQTTBUTTON2_ENABLED)
  if ((button2)&&(!button2_prev)&&(button2_enabled))
  #else
  if ((button2)&&(!button2_prev))
  #endif
  {
    #if (DEVICE_CONFIG&SIP2_ENABLED)
    if ((!bInDial)&&(!aSip.IsBusy()))
    {
      digitalWrite(LED, LOW);
      digitalWrite(OUT2, HIGH);
      aSip.Dial(sipdialnr2, sipsenderdesc);
      bInDial = true;
    }
    #endif

    #if (DEVICE_CONFIG&MQTTBELL2_ENABLED) 
    mqtt.publish(bell2_action_topic, BELL_ACTION_CONTENT, false);
    #endif
    
    #if (DEVICE_CONFIG&CHIME2_ENABLED)
    #if (!(DEVICE_CONFIG&SIP2_ENABLED))
    digitalWrite(OUT2, HIGH);
    #endif
    digitalWrite(OUT4, HIGH);
    #endif
    
    #if ((DEVICE_CONFIG&CHIME2_ENABLED)||((DEVICE_CONFIG&MQTTBELL2_ENABLED)&&(!(DEVICE_CONFIG&SIP2_ENABLED))))
    button2Time = millis();
    #endif
    Serial.println("Button 2 pressed");
  }
  #if ((DEVICE_CONFIG&CHIME2_ENABLED)||((DEVICE_CONFIG&MQTTBELL2_ENABLED)&&(!(DEVICE_CONFIG&SIP2_ENABLED))))
  if ((button2Time > 0u)&&((millis() - button2Time) > BUTTON_ACTION_TIME))
  {
    button2Time = 0u;
    #if (!(DEVICE_CONFIG&SIP2_ENABLED))
    digitalWrite(OUT2, LOW);
    #endif
    #if (DEVICE_CONFIG&CHIME2_ENABLED)
    digitalWrite(OUT4, LOW);
    #endif
    Serial.println("Button 2 end");
  }
  #endif
  button2_prev = button2;
}
#endif

#if (DEVICE_CONFIG&SIP_ENABLED)
void handle_sip()
{
  int packetSize = aSip.Udp.parsePacket();
  if (packetSize > 0)
  {
    caSipIn[0] = 0;
    packetSize = aSip.Udp.read(caSipIn, sizeof(caSipIn));
    if (packetSize > 0)
    {
      caSipIn[packetSize] = 0;
    }
  }
  aSip.HandleUdpPacket((packetSize > 0) ? caSipIn : 0);

  if ((bInDial)&&(!aSip.IsBusy()))
  {
    digitalWrite(LED, HIGH);
    #if (DEVICE_CONFIG&SIP1_ENABLED)
    digitalWrite(OUT1, LOW);
    #endif
    #if (DEVICE_CONFIG&SIP2_ENABLED)
    digitalWrite(OUT2, LOW);
    #endif
    bInDial = false;
    Serial.println("SIP end");
  }
}
#endif

#if (DEVICE_CONFIG&LOCK_ENABLED)
void handle_lock() {
  static unsigned long doorOpenTime = 0u;

  #if ((DEVICE_CONFIG&SIPLOCK_ENABLED)&&(DEVICE_CONFIG&MQTTLOCK_ENABLED))
  if (((aSip.iSignal == sipdigit) || (mqtt_lock_trigger)) && (doorOpenTime == 0u))
  #elif (DEVICE_CONFIG&SIPLOCK_ENABLED)
  if ((aSip.iSignal == sipdigit) && (doorOpenTime == 0u))
  #else
  if ((mqtt_lock_trigger) && (doorOpenTime == 0u))
  #endif
  {
    doorOpenTime = millis();
    digitalWrite(OUT3, HIGH);
    Serial.println("Door unlocked");
  }
  #if (DEVICE_CONFIG&MQTTLOCK_ENABLED)
  mqtt_lock_trigger = false;
  #endif

  if ((doorOpenTime > 0u) && ((millis() - doorOpenTime) > DOOR_UNLOCKED_TIME))
  {
    doorOpenTime = 0u;
    digitalWrite(OUT3, LOW);
    #if (DEVICE_CONFIG&SIPLOCK_ENABLED)
    aSip.iSignal = 0;
    #endif
    Serial.println("Door locked");
  }
}
#endif

#if (DEVICE_CONFIG&MQTT_ENABLED)
void reconnect_mqtt() {
  if (mqtt_client.connect(unique_id.c_str(), MQTT_USER, MQTT_PASSWORD, avty_topic.c_str(), 0, true, "offline")) {
    mqtt_init_publish_and_subscribe();
  } else {
    delay(500);
  }
}

void mqtt_init_publish_and_subscribe() {
  mqtt.publish(avty_topic, "online", true);
  #if (DEVICE_CONFIG&MQTTLOCK_ENABLED)
  mqtt.subscribe(lock_cmd_topic, lock_cmd_subscriber);
  #endif
  #if (DEVICE_CONFIG&MQTTBUTTON1_ENABLED)
  mqtt.subscribe(button1_cmd_topic, button1_cmd_subscriber);
  if(button1_enabled)
  {
    mqtt.publish(button1_state_topic, "ON", true);
  }
  else
  {
    mqtt.publish(button1_state_topic, "OFF", true);
  }
  #endif
  #if (DEVICE_CONFIG&MQTTBUTTON2_ENABLED)
  mqtt.subscribe(button2_cmd_topic, button2_cmd_subscriber);
  if(button2_enabled)
  {
    mqtt.publish(button2_state_topic, "ON", true);
  }
  else
  {
    mqtt.publish(button2_state_topic, "OFF", true);
  }
  #endif
}

void setup_mqtt_topics() {
  avty_topic = "homeassistant/button/" + unique_id + "_door1/availability";
  Serial.print("avty_topic: ");
  Serial.println(avty_topic);
  #if (DEVICE_CONFIG&MQTTLOCK_ENABLED)
  lock_cmd_topic = "homeassistant/button/" + unique_id + "_door1/unlock";
  Serial.print("lock_cmd_topic: ");
  Serial.println(lock_cmd_topic);
  #endif
  #if (DEVICE_CONFIG&MQTTBELL1_ENABLED)  
  bell1_action_topic = "homeassistant/device_automation/" + unique_id + "_bell1/action";
  Serial.print("bell1_action_topic: ");
  Serial.println(bell1_action_topic);
  #endif
  #if (DEVICE_CONFIG&MQTTBELL2_ENABLED)  
  bell2_action_topic = "homeassistant/device_automation/" + unique_id + "_bell2/action";
  Serial.print("bell2_action_topic: ");
  Serial.println(bell2_action_topic);
  #endif
  #if (DEVICE_CONFIG&MQTTBUTTON1_ENABLED)
  button1_state_topic = "homeassistant/switch/" + unique_id + "_button1/state";
  Serial.print("button1_state_topic: ");
  Serial.println(button1_state_topic);
  button1_cmd_topic = "homeassistant/switch/" + unique_id + "_button1/set";
  Serial.print("button1_cmd_topic: ");
  Serial.println(button1_cmd_topic);
  #endif
  #if (DEVICE_CONFIG&MQTTBUTTON2_ENABLED)
  button2_state_topic = "homeassistant/switch/" + unique_id + "_button2/state";
  Serial.print("button2_state_topic: ");
  Serial.println(button2_state_topic);
  button2_cmd_topic = "homeassistant/switch/" + unique_id + "_button2/set";
  Serial.print("button2_cmd_topic: ");
  Serial.println(button2_cmd_topic);
  #endif
}

void publish_mqtt_autodiscovery() {
  String topic;
  String payload;

  String device = "\"dev\":{\"ids\":\"" + unique_id + "\", \"name\":\"" + HOSTNAME + "\", \"mdl\":\"DOORBELL v2\", \"mf\":\"stephan192\", \"hw\":\"" + String(HW_VERSION) + "\", \"sw\":\"" + String(SW_VERSION) + "\"}";
  String obj_id = String(HOSTNAME);
  obj_id.toLowerCase();
  
  #if (DEVICE_CONFIG&MQTTBELL1_ENABLED)
  topic = "homeassistant/device_automation/" + unique_id + "_bell1/config";
  payload = "{\"atype\":\"trigger\", \"t\":\"homeassistant/device_automation/" + unique_id + "_bell1/action\", \"type\":\"button_short_press\", \"stype\":\"button_1\", " + device + "}";
  publish_oversize_payload(topic, payload, true);
  #endif
  #if (DEVICE_CONFIG&MQTTBELL2_ENABLED)
  topic = "homeassistant/device_automation/" + unique_id + "_bell2/config";
  payload = "{\"atype\":\"trigger\", \"t\":\"homeassistant/device_automation/" + unique_id + "_bell2/action\", \"type\":\"button_short_press\", \"stype\":\"button_2\", " + device + "}";
  publish_oversize_payload(topic, payload, true);
  #endif
  #if (DEVICE_CONFIG&MQTTLOCK_ENABLED)
  topic = "homeassistant/button/" + unique_id + "_door1/config";
  payload = "{\"cmd_t\":\"homeassistant/button/" + unique_id + "_door1/unlock\", \"avty_t\":\"" + avty_topic + "\", \"uniq_id\":\"" + unique_id + "_door1\", \"obj_id\":\"" + obj_id + "_door1\", \"name\":\"Door\", \"icon\":\"mdi:door\", " + device + "}";
  publish_oversize_payload(topic, payload, true);
  #endif
  #if (DEVICE_CONFIG&MQTTBUTTON1_ENABLED)
  topic = "homeassistant/switch/" + unique_id + "_button1/config";
  payload = "{\"~\":\"homeassistant/switch/" + unique_id + "_button1\", \"cmd_t\":\"~/set\", \"stat_t\":\"~/state\", \"avty_t\":\"" + avty_topic + "\", \"uniq_id\":\"" + unique_id + "_button1\", \"obj_id\":\"" + obj_id + "_button1\", \"name\":\"Button 1\", \"icon\":\"mdi:bell\", " + device + "}";
  publish_oversize_payload(topic, payload, true);
  #endif
  #if (DEVICE_CONFIG&MQTTBUTTON2_ENABLED)
  topic = "homeassistant/switch/" + unique_id + "_button2/config";
  payload = "{\"~\":\"homeassistant/switch/" + unique_id + "_button2\", \"cmd_t\":\"~/set\", \"stat_t\":\"~/state\", \"avty_t\":\"" + avty_topic + "\", \"uniq_id\":\"" + unique_id + "_button2\", \"obj_id\":\"" + obj_id + "_button2\", \"name\":\"Button 2\", \"icon\":\"mdi:bell\", " + device + "}";
  publish_oversize_payload(topic, payload, true);
  #endif
  Serial.println("MQTT autodiscovery sent");
}

void publish_oversize_payload(String topic, String payload, bool retain)
{
  String substr;
  unsigned int index = 0u;
  unsigned int payload_len = payload.length();
  unsigned int count;

  mqtt_client.beginPublish(topic.c_str(), payload.length(), retain);
  while(index < payload_len)
  {
    count = payload_len - index;
    if(count > MQTT_MAX_PACKET_SIZE) count = MQTT_MAX_PACKET_SIZE;
    
    substr = payload.substring(index, (index+count));
    mqtt_client.write((byte*)substr.c_str(), count);
    index += count;
  }
  mqtt_client.endPublish();
}

#if (DEVICE_CONFIG&MQTTLOCK_ENABLED)
void lock_cmd_subscriber(String topic, String message)
{
  if (message == "PRESS")
  {
    mqtt_lock_trigger = true;
  }
}
#endif

#if (DEVICE_CONFIG&MQTTBUTTON1_ENABLED)
void button1_cmd_subscriber(String topic, String message)
{
  if (message == "ON")
  {
    button1_enabled = true;
    mqtt.publish(button1_state_topic, "ON", true);
  }
  else if (message == "OFF")
  {
    button1_enabled = false;
    mqtt.publish(button1_state_topic, "OFF", true);
  }
}
#endif

#if (DEVICE_CONFIG&MQTTBUTTON2_ENABLED)
void button2_cmd_subscriber(String topic, String message)
{
  if (message == "ON")
  {
    button2_enabled = true;
    mqtt.publish(button2_state_topic, "ON", true);
  }
  else if (message == "OFF")
  {
    button2_enabled = false;
    mqtt.publish(button2_state_topic, "OFF", true);
  }
}
#endif
#endif
