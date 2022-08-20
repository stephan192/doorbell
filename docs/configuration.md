# Configuration
The configuration is stored in `doorbell\config.h`. If file doesn't exist copy `doorbell\config_template.h` and adopt to your needs.

# Basics
| Define          | Description |
|-----------------|-------------|
| `HOSTNAME`      | Hostname used for WIFI, OTA and MQTT |
| `WIFI_SSID`     | WIFI ssid to which the ESP should connect |
| `WIFI_PASS`     | WIFI password used to connect to `WIFI_SSID` |
| `OTA_PASSWORT`  | Password used for over the air updates from Arduino IDE |

# MQTT
| Define                | Description |
|-----------------------|-------------|
| `MQTT_SERVER`         | IP of the MQTT server to which the ESP should connect |
| `MQTT_PORT`           | Port of the MQTT server to which the ESP should connect (typically 1883) |
| `MQTT_USER`           | Username used to connect to the MQTT server |
| `MQTT_PASSWORD`       | Password used to connect to the MQTT server |
| `BELL_ACTION_CONTENT` | Text which will be published on Button press event | 

# SIP
| Define              | Description |
|---------------------|-------------|
| `SIP_IP`            | IP of the SIP server to which the ESP should connect |
| `SIP_PORT`          | Port of the SIP server to which the ESP should connect (typically 5060) |
| `SIP_USER`          | Username used to connect to the SIP server |
| `SIP_PASSWORD`      | Password used to connect to the SIP server |
| `SIP_DIAL_NR1`      | Phone number which should be dialed when button 1 is pressed (typically "11") |
| `SIP_DIAL_NR2`      | Phone number which should be dialed when button 2 is pressed (typically "12") |
| `SIP_DIAL_DIGIT`    | Digit which, when received, opens the doorlock |
| `SIP_MAX_DAIL_TIME` | Maximum time after which an unanswered call is terminated |

# In/Out
| Define                 | Description |
|------------------------|-------------|
| `BUTTON_DEBOUNCE_TIME` | Debounce time for button inputs |
| `BUTTON_ACTION_TIME`   | Time after which the corresponding output, if configured, gets deactivated |
| `DOOR_UNLOCKED_TIME`   | Time for which the door is open after unlocked via MQTT or SIP |

# Device setup
Combine the following defines by ORing to configure your individual setup. The defines can be found in `doorbell.ino`.

| Define                | Description |
|-----------------------|-------------|
| `SIP1_ENABLED`        | Button 1 dails SIP_DIAL_NR1 |
| `SIP2_ENABLED`        | Button 2 dails SIP_DIAL_NR2 |
| `CHIME1_ENABLED`      | Button 1 activates chime 1 (OUT3) for BUTTON_ACTION_TIME |
| `CHIME2_ENABLED`      | Button 2 activates chime 2 (OUT4) for BUTTON_ACTION_TIME |
| `SIPLOCK_ENABLED`     | Unlock door (OUT3) when SIP_DIAL_DIGIT is received via SIP |
| `MQTTLOCK_ENABLED`    | Unlock door (OUT3) when corresponding command is received via MQTT |
| `MQTTBELL1_ENABLED`   | Button 1 publishes device trigger to corresponding MQTT topic |
| `MQTTBELL2_ENABLED`   | Button 2 publishes device trigger to corresponding MQTT topic |
| `MQTTBUTTON1_ENABLED` | Button 1 can be enabled/disabled via MQTT |
| `MQTTBUTTON2_ENABLED` | Button 2 can be enabled/disabled via MQTT |

Example:
```
#define DEVICE_CONFIG         (SIP1_ENABLED|SIPLOCK_ENABLED|MQTTLOCK_ENABLED|MQTTBELL1_ENABLED|MQTTBUTTON1_ENABLED)
```
