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
LOCAL char * recieveBuffer = NULL;

LOCAL void HTTPConfig_TCPServerSendResponse(struct espconn *pespconn, int error, char *html_txt) {

	char *buffer = NULL;
	int html_length = 0;
	buffer = (char *) os_malloc(128 * sizeof(char));
	if (buffer != NULL) {
		if (html_txt != NULL) {
			html_length = strlen(html_txt);
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
			buffer = (char *)os_realloc(buffer, (128 + html_length) *sizeof(char));
			strcat(buffer, html_txt);
			os_free(html_txt);
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
			char tmp[3] = {0};
			strncpy(tmp, encoded + encodedPosition + 1, 2);
			decoded[decodedPosition] = strtol(tmp, NULL, 16);

			encodedPosition+=3;
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

LOCAL char*  HTTPConfig_GetFormInputNumber(char* buffer, char * label, char * name, int value, char * type){

	char *html = NULL;
	html = (char *) os_malloc(256 * sizeof(char));

	os_sprintf(html, "<tr>\
		<td>%s:</td>\
		<td><input name='%s' value='%d' type='%s'></td>\
	</tr>", label, name, value, type);
	
	buffer = (char *) os_realloc(buffer, (strlen(buffer) + strlen(html)) * sizeof(char));

	strcat(buffer, html);
	os_free(html);
	return buffer;
}

LOCAL char*  HTTPConfig_GetFormInputString(char* buffer, char * label, char * name, char * value, int valueLength, char * type){

	char *html = NULL;
	html = (char *) os_malloc(256 * sizeof(char));

	os_sprintf(html, "<tr>\
		<td>%s:</td>\
		<td><input name='%s' value='%s' maxlength='%d' type='%s'></td>\
	</tr>", label, name, value, valueLength, type);
	
	buffer = (char *) os_realloc(buffer, (strlen(buffer) + strlen(html)) * sizeof(char));

	strcat(buffer, html);
	os_free(html);
	return buffer;
}


LOCAL char* HTTPConfig_GetForm(void){
	char *html = NULL;
	html = (char *) os_malloc(4096 * sizeof(char));

	CFG_Load();

strcpy(html, "<!DOCTYPE html>\
<html>\
<head>\
	<title>ESP8266 V-Config</title>\
</head>\
<style>\
body{\
	font-family: sans-serif;\
	color: coral;\
	background-color: aliceblue;\
}\
input{\
	border-radius: 5px;\
	border: 1px solid coral;\
	height: 20px;\
	padding: 5px;\
}\
button{\
	border-radius: 5px;\
	background-color: coral;\
	border: 1px solid coral;\
	color: aliceblue;\
	padding: 10px;\
}\
</style>\
<body>\
<form method='post'>\
	<table>\
		<tbody>");

	html = HTTPConfig_GetFormInputString(html, "WiFi SSID", 		"sta_ssid", 		sysCfg.sta_ssid, 		sizeof(sysCfg.sta_ssid),	"text");
	html = HTTPConfig_GetFormInputString(html, "WiFi Password", 	"sta_pwd", 			sysCfg.sta_pwd, 		sizeof(sysCfg.sta_pwd),"password");
	html = HTTPConfig_GetFormInputString(html, "MQTT Host", 		"mqtt_host", 		sysCfg.mqtt_host, 		sizeof(sysCfg.mqtt_host),"text");
	html = HTTPConfig_GetFormInputNumber(html, "MQTT Port", 		"mqtt_port", 		sysCfg.mqtt_port, 		"number");
	html = HTTPConfig_GetFormInputString(html, "MQTT User Name", 	"mqtt_user", 		sysCfg.mqtt_user, 		sizeof(sysCfg.mqtt_user),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT Password", 	"mqtt_pass", 		sysCfg.mqtt_pass, 		sizeof(sysCfg.mqtt_pass),"password");
	html = HTTPConfig_GetFormInputString(html, "MQTT Root Topic", 	"mqtt_topicroot", 	sysCfg.mqtt_topicroot, 	sizeof(sysCfg.mqtt_topicroot),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT ADC0 Topic", 	"mqtt_topicadc0", 	sysCfg.mqtt_topicadc0, 	sizeof(sysCfg.mqtt_topicadc0),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT ADC1 Topic", 	"mqtt_topicadc1", 	sysCfg.mqtt_topicadc1, 	sizeof(sysCfg.mqtt_topicadc1),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT ADC2 Topic", 	"mqtt_topicadc2", 	sysCfg.mqtt_topicadc2, 	sizeof(sysCfg.mqtt_topicadc2),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT ADC3 Topic", 	"mqtt_topicadc3", 	sysCfg.mqtt_topicadc3, 	sizeof(sysCfg.mqtt_topicadc3),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT ADC4 Topic", 	"mqtt_topicadc4", 	sysCfg.mqtt_topicadc4, 	sizeof(sysCfg.mqtt_topicadc4),"text");
	html = HTTPConfig_GetFormInputString(html, "MQTT ADC5 Topic", 	"mqtt_topicadc5", 	sysCfg.mqtt_topicadc5, 	sizeof(sysCfg.mqtt_topicadc5),"text");

html = (char *) os_realloc(html, (strlen(html) + 130)  *sizeof(char));
strcat(html, "<tr>\
					<td>\
						<button>Save Settings</button>\
					</td>\
				</tr>\
			</tbody>\
		</table>\
	</form>\
	</body>\
</html>");

	return html;
}


LOCAL void HTTPConfig_SetData(){

	char *httpData = NULL;
	httpData = (char *)strstr(recieveBuffer, "\r\n\r\n");
	httpData+=4;

	os_printf("tcp dat : %s \r\n", httpData);

	char *key = NULL;
	char *value = NULL;
	

	char *end = 0;
	int length = 0;

	CFG_Load();

	int loop = 1;
	while (loop){

		key = (char *) os_calloc(64, sizeof(char));
		value = (char *) os_calloc(64, sizeof(char));

		os_printf("Loop...\r\n");

		end = (char *) strstr(httpData, "=");
		if (end == NULL){
			os_printf("Failed to find = when looping through POST vars\r\n");
			break;
		}
		length = end - httpData;
		strncpy(key, httpData, length);

		httpData = end + 1;

		end = (char *) strstr(httpData, "&");
		if (end != NULL){
			length = end - httpData;
		} else{
			loop = 0;
			length = strlen(httpData);
		}
		strncpy(value, httpData, length);

		httpData = end + 1;

		os_printf("Key: %s, Val: %s \r\n", key, value);

		value = HTTPConfig_HTMLEntityDecode(value);

		if (strcmp(key, "sta_ssid") == 0) {
			strcpy(sysCfg.sta_ssid, value);
		}
		else if (strcmp(key, "sta_pwd") == 0) {
			strcpy(sysCfg.sta_pwd, value);
		}
		else if (strcmp(key, "mqtt_host") == 0) {
			strcpy(sysCfg.mqtt_host, value);
		}
		else if (strcmp(key, "mqtt_port") == 0) {
			sysCfg.mqtt_port = atoi(value);
		}
		else if (strcmp(key, "mqtt_user") == 0) {
			strcpy(sysCfg.mqtt_user, value);
		}
		else if (strcmp(key, "mqtt_pass") == 0) {
			strcpy(sysCfg.mqtt_pass, value);
		}
		else if (strcmp(key, "mqtt_topicroot") == 0) {
			strcpy(sysCfg.mqtt_topicroot, value);
		}
		else if (strcmp(key, "mqtt_topicadc0") == 0) {
			strcpy(sysCfg.mqtt_topicadc0, value);
		}
		else if (strcmp(key, "mqtt_topicadc1") == 0) {
			strcpy(sysCfg.mqtt_topicadc1, value);
		}
		else if (strcmp(key, "mqtt_topicadc2") == 0) {
			strcpy(sysCfg.mqtt_topicadc2, value);
		}
		else if (strcmp(key, "mqtt_topicadc3") == 0) {
			strcpy(sysCfg.mqtt_topicadc3, value);
		}
		else if (strcmp(key, "mqtt_topicadc4") == 0) {
			strcpy(sysCfg.mqtt_topicadc4, value);
		}
		else if (strcmp(key, "mqtt_topicadc5") == 0) {
			strcpy(sysCfg.mqtt_topicadc5, value);
		}

	}

	CFG_Save();
}

LOCAL unsigned int HTTPConfig_AllInBuffer(){

	char * start = NULL;
	start = strstr(recieveBuffer, "Content-Length:");

	// If there is no Content Length header, there will be no data
	if (start == NULL){
		return 1;
	}

	// Offset based on string length
	start+= 15;

	char * end = NULL;
	end = strstr(start, "\r\n");

	if (end == NULL){
		os_printf("!E");
		return 0;
	}

	char contentLengthStr[6] = {0};
	unsigned int length = end - start;
	strncpy(contentLengthStr, start, length);
	contentLengthStr[length] = '\0';

	unsigned int dataLength = 0;
	dataLength = atoi(contentLengthStr);

	char * data = NULL;
	data = strstr(recieveBuffer, "\r\n\r\n");

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

	// Append recieved data to buffer
	recieveBuffer = (char *) os_realloc(recieveBuffer, (strlen(recieveBuffer) + length) * sizeof(char));
	strncat(recieveBuffer, pusrdata, length);
	os_free(pusrdata);

	if (!HTTPConfig_AllInBuffer()){
		return;
	}

	char requestType[16] = {0};
	strncpy(requestType, recieveBuffer, strstr(recieveBuffer, "\r\n") - recieveBuffer);

	if (strcmp(requestType, "GET / HTTP/1.1") == 0) {
		recieveBuffer = (char *) os_calloc(1, sizeof(char));
		HTTPConfig_TCPServerSendResponse(pespconn, 200, HTTPConfig_GetForm());
	}
	else if (strcmp(requestType, "POST / HTTP/1.1") == 0) {
		HTTPConfig_SetData();
		recieveBuffer = (char *) os_calloc(1, sizeof(char));
		HTTPConfig_TCPServerSendResponse(pespconn, 200, HTTPConfig_GetForm());
	}
	else{
		recieveBuffer = (char *) os_calloc(1, sizeof(char));
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

LOCAL void HTTPConfig_ScanComplete(){

}

void HTTPConfig_Init(void){

	if (true){
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
	
	recieveBuffer = (char *) os_calloc(1, sizeof(char));


	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = 80;
	espconn_regist_connectcb(&esp_conn, HTTPConfig_TCPPortConnected);

	sint8 ret = espconn_accept(&esp_conn);

	os_printf("HTTP Server Listen on port 80 State: %d\r\n", ret);
}