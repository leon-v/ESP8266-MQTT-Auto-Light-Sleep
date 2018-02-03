/*
/* config.c
/* https://esp8266.vn/nonos-sdk/http-server/http-server/
*/

#include "http_config.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"

LOCAL struct espconn esp_conn;
LOCAL esp_tcp esptcp;

LOCAL void HTTPConfig_TCPServerSendResponse(struct espconn *pespconn, int error, char *html_txt) {
	char *buffer = NULL;
	int html_length = 0;
	buffer = (char *) os_malloc(256 * sizeof(char));
	if (buffer != NULL) {
		if (html_txt != NULL) {
			html_length = os_strlen(html_txt);
		} else {
			html_length = 0;
		}

		os_sprintf(buffer, "HTTP/1.1 %d OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n"
			"Connection: Closed\r\n"
			"\r\n"
		,error, html_length);

		if(html_length > 0) {
			buffer = (char *)os_realloc(buffer, (256 + html_length) *sizeof(char));
			os_strcat(buffer, html_txt);
		}

		espconn_sent(pespconn, buffer, strlen(buffer));
		os_free(buffer);
	}
}



LOCAL void HTTPConfig_TCPServerRecieveCallback(void *arg, char *pusrdata, unsigned short length) {

	char *ptr = 0;
	//received some data from tcp connection

	struct espconn *pespconn = arg;

	os_printf("tcp recv : %s \r\n", pusrdata);
	ptr = (char *)os_strstr(pusrdata, "\r\n");

	ptr[0] = '\0';
	if (os_strcmp(pusrdata, "GET / HTTP/1.1") == 0) {
		HTTPConfig_TCPServerSendResponse(pespconn, 200, "<!DOCTYPE html>\
<html>\
<head>\
	<title>ESP8266 V-Config</title>\
</head>\
<body>\
	<form method=\"post\">\
		<input type=\"submit\">\
	</form>\
</body>\
</html>");
	}
	
	else if (os_strcmp(pusrdata, "POST / HTTP/1.1") == 0) {
		
	}
}


LOCAL void HTTPConfig_TCPServerConnectionError(void *arg, sint8 err) {
    //error occured , tcp connection broke. 
    os_printf("reconnect callback, error code %d !!! \r\n",err);
}

LOCAL void HTTPConfig_TCPServerDisconnected(void *arg) {
    //tcp disconnect successfully
    os_printf("HTTP Server Disconnected Cleanly\r\n");
}

LOCAL void HTTPConfig_TCPServerSent(void *arg) {
    //data sent successfully

    os_printf("HTTP Server Sent Data\r\n");
}


LOCAL void HTTPConfig_TCPPortConnected(void *arg) {

	struct espconn *pesp_conn = arg;
	os_printf("HTTP Server Opening Connection \r\n");

	espconn_regist_recvcb(pesp_conn, HTTPConfig_TCPServerRecieveCallback);
	espconn_regist_reconcb(pesp_conn, HTTPConfig_TCPServerConnectionError);
	espconn_regist_disconcb(pesp_conn, HTTPConfig_TCPServerDisconnected);

	espconn_regist_sentcb(pesp_conn, HTTPConfig_TCPServerSent);
}


void HTTPConfig_Init(void){

	struct softap_config config;

	// wifi_softap_get_config(&config); // Get config first.

	// os_memset(config.ssid, 0, 32);
	// os_memset(config.password, 0, 64);
	// os_memcpy(config.ssid, "ESP8266 V-Sensor Config Mode", 28);
	// os_memcpy(config.password, "", 0);
	// config.authmode = AUTH_OPEN;
	// config.ssid_len = 28;// or its actual length
	// config.beacon_interval = 100;
	// config.max_connection = 1; // how many stations can connect to ESP8266 softAP at most.

	// wifi_softap_set_config(&config);// Set ESP8266 softap config .

	// os_printf("WiFi AP Started \r\n");

	// uint8 mode = 0;
	// wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);

	// os_printf("DHCP Started \r\n");





	os_printf("WIFI_INIT\r\n");
	wifi_set_opmode_current(STATION_MODE);

	struct station_config stationConf;
	os_memset(&stationConf, 0, sizeof(struct station_config));

	os_sprintf(stationConf.ssid, "%s", "valkenorton2.4Ghz_Fritz");
	os_sprintf(stationConf.password, "%s", "ubuntugrl7");

	wifi_station_set_config_current(&stationConf);

	wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();




	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = 80;
	espconn_regist_connectcb(&esp_conn, HTTPConfig_TCPPortConnected);

	sint8 ret = espconn_accept(&esp_conn);

	os_printf("HTTP Server Listen on port 80 State: %d\r\n", ret);
}