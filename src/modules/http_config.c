/*
/* config.c
/* https://esp8266.vn/nonos-sdk/http-server/http-server/
*/

#include "http_config.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "config.h"
#include "../../../secrets.h"

LOCAL struct espconn esp_conn;
LOCAL esp_tcp esptcp;
LOCAL char recieveBuffer[2048];
LOCAL unsigned int recieveBufferPosition = 0;

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

LOCAL char* HTTPConfig_HTMLEntityDecode(char * encoded){

	char *decoded = NULL;
	decoded = (char *) os_malloc(64 * sizeof(char));

	
	unsigned int encodedPosition = 0;
	unsigned int decodedPosition = 0;

	while (encodedPosition < strlen(encoded)) {

		if (encoded[encodedPosition] == '+'){

			decoded[decodedPosition] = ' ';
			encodedPosition++;
		}

		else if (encoded[encodedPosition] == '%'){
			decoded[decodedPosition] = encoded[encodedPosition];
			encodedPosition++;
		}

		else{
			decoded[decodedPosition] = encoded[encodedPosition];
			encodedPosition++;
		}

		decodedPosition++;
	}
	decoded[decodedPosition] = '\0';
	return decoded;

}

LOCAL char* HTTPConfig_GetForm(void){
	char *html = NULL;
	html = (char *) os_malloc(2048 * sizeof(char));

	CFG_Load();

	os_sprintf(html, "<!DOCTYPE html>\
		<html>\
		<head>\
			<title>ESP8266 V-Config</title>\
		</head>\
		<body>\
		<form method='post'>\
			<table>\
				<tbody>\
					<tr>\
						<td>WiFi SSID:</td>\
						<td><input name='sta_ssid' value='%s'></td>\
					</tr>\
					<tr>\
						<td>WiFi Password:</td>\
						<td><input name='sta_pwd' value='%s' type='password'></td>\
					</tr>\
					<tr>\
						<td>MQTT Host:</td>\
						<td><input name='mqtt_host' value='%s'></td>\
					</tr>\
					<tr>\
						<td>MQTT Port:</td>\
						<td><input name='mqtt_port' value='%d'></td>\
					</tr>\
					<tr>\
						<td>MQTT Username:</td>\
						<td><input name='mqtt_user' value='%s'></td>\
					</tr>\
					<tr>\
						<td>MQTT Password:</td>\
						<td><input name='mqtt_pass' value='%s' type='password'></td>\
					</tr>\
 					<tr>\
 						<td>\
							<input type='submit' value='Save Settings' >\
						</td>\
					</tr>\
				</tbody>\
			</table>\
		</form>\
		</body>\
	</html>",
		sysCfg.sta_ssid,
		sysCfg.sta_pwd,
		sysCfg.mqtt_host,
		sysCfg.mqtt_port,
		sysCfg.mqtt_user,
		sysCfg.mqtt_pass

	);

	return html;
}


LOCAL void HTTPConfig_SetData(char * pusrdata){

	char *httpData = 0;
	httpData = (char *)os_strstr(pusrdata, "\r\n\r\n");
	httpData+=4;

	os_printf("tcp dat : %s \r\n", httpData);

	char *key = NULL;
	key = (char *) os_malloc(64 * sizeof(char));

	char *value = NULL;
	value = (char *) os_malloc(64 * sizeof(char));

	char *end = 0;
	int length = 0;

	CFG_Load();

	int loop = 1;
	while (loop){

		os_printf("Loop...\r\n");

		end = (char *) os_strstr(httpData, "=");
		if (end == NULL){
			os_printf("Failed to find = when looping through POST vars\r\n");
			break;
		}
		length = end - httpData;
		os_strncpy(key, httpData, length);
		key[length] = '\0';

		httpData = end + 1;

		end = (char *) os_strstr(httpData, "&");
		if (end != NULL){
			length = end - httpData;
		} else{
			loop = 0;
			length = strlen(httpData);
		}
		os_strncpy(value, httpData, length);
		value[length] = '\0';

		httpData = end + 1;

		os_printf("Key: %s, Val: %s \r\n", key, value);

		value = HTTPConfig_HTMLEntityDecode(value);

		if (os_strcmp(key, "sta_ssid") == 0) {
			os_strcpy(sysCfg.sta_ssid, value);
		}
		else if (os_strcmp(key, "sta_pwd") == 0) {
			os_strcpy(sysCfg.sta_pwd, value);
		}
		else if (os_strcmp(key, "mqtt_host") == 0) {
			os_strcpy(sysCfg.mqtt_host, value);
		}
		else if (os_strcmp(key, "mqtt_port") == 0) {
			sysCfg.mqtt_port = atoi(value);
		}
		else if (os_strcmp(key, "mqtt_user") == 0) {
			os_strcpy(sysCfg.mqtt_user, value);
		}
		else if (os_strcmp(key, "mqtt_pass") == 0) {
			os_strcpy(sysCfg.mqtt_pass, value);
		}
		os_printf("Key: %s, Val: %s \r\n", key, value);

	}

	CFG_Save();
}

LOCAL unsigned int HTTPConfig_AllInBuffer(){

	char * start = 0;
	start = os_strstr(recieveBuffer, "Content-Length:");

	// If there is no Content Length header, there will be no data
	if (start == NULL){
		return 1;
	}

	// Offset based on string length
	start+= 15;

	char * end = 0;
	end = os_strstr(start, "\r\n");

	if (end == NULL){
		os_printf("!E");
		return 0;
	}

	char contentLengthStr[6] = {0};
	unsigned int length = end - start;
	os_strncpy(contentLengthStr, start, length);
	contentLengthStr[length] = '\0';

	unsigned int dataLength = 0;
	dataLength = atoi(contentLengthStr);

	char * data = 0;
	data = os_strstr(recieveBuffer, "\r\n\r\n");

	if (data == NULL){
		return 0;
	}

	data+= 4;

	if (strlen(data) < dataLength){
		return 0;
	}

	return 1;
}
//received some data from tcp connection
LOCAL void HTTPConfig_TCPServerRecieveCallback(void *arg, char *pusrdata, unsigned short length) {

	struct espconn *pespconn = arg;


	os_strncpy(recieveBuffer + recieveBufferPosition, pusrdata, length);
	recieveBufferPosition+= length;
	recieveBuffer[recieveBufferPosition] = '\0';

	if (!HTTPConfig_AllInBuffer()){
		return;
	}

	recieveBufferPosition = 0;


	char requestType[16] = {0};
	os_strncpy(requestType, pusrdata, os_strstr(pusrdata, "\r\n") - pusrdata);

	if (os_strcmp(requestType, "GET / HTTP/1.1") == 0) {
		HTTPConfig_TCPServerSendResponse(pespconn, 200, HTTPConfig_GetForm());
	}
	else if (os_strcmp(requestType, "POST / HTTP/1.1") == 0) {
		HTTPConfig_SetData(pusrdata);
		HTTPConfig_TCPServerSendResponse(pespconn, 200, HTTPConfig_GetForm());
	}
	else{
		HTTPConfig_TCPServerSendResponse(pespconn, 404, "");
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

	if (false){
		struct softap_config config;

		wifi_softap_get_config(&config); // Get config first.

		os_memset(config.ssid, 0, 32);
		os_memset(config.password, 0, 64);
		os_memcpy(config.ssid, "ESP8266 V-Sensor Config Mode", 28);
		os_memcpy(config.password, "", 0);
		config.authmode = AUTH_OPEN;
		config.ssid_len = 28;// or its actual length
		config.beacon_interval = 100;
		config.max_connection = 1; // how many stations can connect to ESP8266 softAP at most.

		wifi_softap_set_config(&config);// Set ESP8266 softap config .

		os_printf("WiFi AP Started \r\n");

		uint8 mode = 0;
		wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);

		os_printf("DHCP Started \r\n");
	}

	else {
		os_printf("WIFI_INIT\r\n");
		wifi_set_opmode_current(STATION_MODE);

		struct station_config stationConf;
		os_memset(&stationConf, 0, sizeof(struct station_config));

		os_sprintf(stationConf.ssid, "%s", WIFI_SSID);
		os_sprintf(stationConf.password, "%s", WIFI_PASS);

		wifi_station_set_config_current(&stationConf);

		wifi_station_set_auto_connect(TRUE);
		wifi_station_connect();
	}
	




	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = 80;
	espconn_regist_connectcb(&esp_conn, HTTPConfig_TCPPortConnected);

	sint8 ret = espconn_accept(&esp_conn);

	os_printf("HTTP Server Listen on port 80 State: %d\r\n", ret);
}