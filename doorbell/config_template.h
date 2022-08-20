#define HOSTNAME              "Your hostname"

#define WIFI_SSID             "Your WIFI ssid"
#define WIFI_PASS             "Your WIFI password"

#define MQTT_SERVER           "IP of your MQTT server"
#define MQTT_PORT             1883
#define MQTT_USER             "Your MQTT user"
#define MQTT_PASSWORD         "Your MQTT password"

#define OTA_PASSWORT          "Your OTA password"

#define SIP_IP                "Your SIP server ip"
#define SIP_PORT              5060
#define SIP_USER              "Your SIP server username"
#define SIP_PASS              "Your SIP server password"

#define SIP_DIAL_NR1          "11"
#define SIP_DIAL_NR2          "12"
#define SIP_DIAL_DIGIT        5

#define BUTTON_DEBOUNCE_TIME  100u  /* [ms] */
#define BUTTON_ACTION_TIME    1000u /* [ms] */
#define DOOR_UNLOCKED_TIME    2000u /* [ms] */
#define SIP_MAX_DAIL_TIME     15u   /* [s] */
#define BELL_ACTION_CONTENT   "ding_dong"

#define DEVICE_CONFIG         (SIP1_ENABLED|SIPLOCK_ENABLED|MQTTLOCK_ENABLED|MQTTBELL1_ENABLED|MQTTBUTTON1_ENABLED)
