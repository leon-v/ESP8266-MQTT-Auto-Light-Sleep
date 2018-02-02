/*
/* config.c
/* https://esp8266.vn/nonos-sdk/http-server/http-server/
*/

#include "user_interface.h"
#include "osapi.h"

void HTTPConfig_Init(void){

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


	uint8 mode = 0;
	wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = 80;
	espconn_regist_connectcb(&esp_conn, HTTPConfig_TCPServerListen);

	sint8 ret = espconn_accept(&esp_conn);

	os_printf("espconn_accept [%d] !!! \r\n", ret);
}

LOCAL void ICACHE_FLASH_ATTR HTTPConfig_TCPServerRecieveCallback(void *arg, char *pusrdata, unsigned short length) {

	char *ptr = 0;
	//received some data from tcp connection

	struct espconn *pespconn = arg;

	// os_printf("tcp recv : %s \r\n", pusrdata);
	ptr = (char *)os_strstr(pusrdata, "\r\n");
	ptr[0] = '\0';
	if (os_strcmp(pusrdata, "GET / HTTP/1.1") == 0) {
		http_response(pespconn, 200, index_html);
	}
}


LOCAL void ICACHE_FLASH_ATTR HTTPConfig_TCPServerListen(void *arg) {

    struct espconn *pesp_conn = arg;
    os_printf("tcp_server_listen !!! \r\n");

    espconn_regist_recvcb(pesp_conn, HTTPConfig_TCPServerRecieveCallback);
    espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
    espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);

    espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);
}