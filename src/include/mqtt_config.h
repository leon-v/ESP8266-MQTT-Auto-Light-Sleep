#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

#define CFG_HOLDER	0x00000002	/* Change this value to load default configurations */
#define CFG_LOCATION	0x79	/* Please don't change or if you know what you doing */
#define MQTT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

#include "../../../secrets.h"

/* Example of secrets.h
#define WIFI_SSID "Your Wifi SSID"
#define WIFI_PASS "Your Wifi Pass"

#define STA_SSID	WIFI_SSID
#define STA_PASS	WIFI_PASS

#define MQTT_CLIENT_ID		"ESP_%08X"
#define MQTT_USER			"Your MQTT Broker User Name"
#define MQTT_PASS			"Your MQTT Broker Password"

#define MQTT_HOST			"Your MQTT Broker IP or Domain" //or "mqtt.yourdomain.com"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		240	 /*second*/
*/

#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

#define DEFAULT_SECURITY	0
#define QUEUE_BUFFER_SIZE		 		2048

#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif // __MQTT_CONFIG_H__
