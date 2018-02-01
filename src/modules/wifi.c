/*
 * wifi.c
 *
 *  Created on: Dec 30, 2014
 *      Author: Minh
 */
#include "wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt_msg.h"
#include "debug.h"
#include "user_config.h"
#include "config.h"

WifiCallback wifiCb = NULL;
static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;
void ICACHE_FLASH_ATTR wifi_check_ip() {
	struct ip_info ipConfig;
	/*
	STATION_IDLE	=	0,
	STATION_CONNECTING,
	STATION_WRONG_PASSWORD,
	STATION_NO_AP_FOUND,
	STATION_CONNECT_FAIL,
	STATION_GOT_IP*/

	wifiStatus = wifi_station_get_connect_status();
	switch (wifiStatus) {
		case STATION_GOT_IP:
			os_printf("WiFi STATION_GOT_IP\r\n");
			break;

		case STATION_CONNECTING:
			os_printf("WiFi STATION_CONNECTING\r\n");
			break;

		case STATION_WRONG_PASSWORD:
			os_printf("WiFi STATION_WRONG_PASSWORD\r\n");
			wifi_station_connect();
			break;

		case STATION_NO_AP_FOUND:
			os_printf("WiFi STATION_NO_AP_FOUND\r\n");
			wifi_station_connect();
			break;

		case STATION_CONNECT_FAIL:
			os_printf("WiFi STATION_CONNECT_FAIL\r\n");
			wifi_station_connect();


		case STATION_IDLE:
		default:
			os_printf("WiFi STATION_IDLE\r\n");

	}

	if(wifiStatus != lastWifiStatus){

		lastWifiStatus = wifiStatus;

		if(wifiCb){
			wifiCb(wifiStatus);
		}
	}
}


void wifi_handle_event_cb(System_Event_t *evt){
	os_printf("event	%x\n",	evt->event);
	switch (evt->event)	{
		case EVENT_STAMODE_CONNECTED:
			os_printf("connect to ssid %s, channel %d\n",	
				evt->event_info.connected.ssid,	
				evt->event_info.connected.channel
			);
		break;

		case EVENT_STAMODE_DISCONNECTED:
			os_printf("disconnect from ssid %s, reason %d\n",	
				evt->event_info.disconnected.ssid,	
				evt->event_info.disconnected.reason
			);
			wifi_station_connect();
		break;

		case EVENT_STAMODE_AUTHMODE_CHANGE:
			os_printf("mode: %d -> %d\n",	
				evt->event_info.auth_change.old_mode,	
				evt->event_info.auth_change.new_mode
			);
		break;

		case EVENT_STAMODE_GOT_IP:
			os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
				IP2STR(&evt->event_info.got_ip.ip),
				IP2STR(&evt->event_info.got_ip.mask),
				IP2STR(&evt->event_info.got_ip.gw)
			);
			os_printf("\n");
		break;

		case EVENT_SOFTAPMODE_STACONNECTED:
			os_printf("station:	" MACSTR "join, AID = %d\n",	
				MAC2STR(evt->event_info.sta_connected.mac),	
				evt->event_info.sta_connected.aid
			);
		break;

		case EVENT_SOFTAPMODE_STADISCONNECTED:
			os_printf("station:	" MACSTR "leave, AID = %d\n",
				MAC2STR(evt->event_info.sta_disconnected.mac),	
				evt->event_info.sta_disconnected.aid
			);
		break;

		default:
		break;
	}

	wifi_check_ip();
}

void ICACHE_FLASH_ATTR WIFI_Connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb) {
	struct station_config stationConf;

	os_printf("WIFI_INIT\r\n");
	wifi_set_opmode_current(STATION_MODE);

	// wifi_station_set_auto_connect(FALSE);
	wifiCb = cb;

	wifi_set_event_handler_cb(wifi_handle_event_cb);

	os_memset(&stationConf, 0, sizeof(struct station_config));

	os_sprintf(stationConf.ssid, "%s", ssid);
	os_sprintf(stationConf.password, "%s", pass);

	os_printf("%s", ssid);
	os_printf("%s", pass);

	wifi_station_set_config_current(&stationConf);

	wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();
}

