#include "wifi.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mqtt.h"
#include "config.h"
#include "mem.h"


unsigned int counter = 0;
unsigned int readyToSleep = 1;
unsigned int wakeCount = 0;
LOCAL MQTT_Client* mqttClient;

void ICACHE_FLASH_ATTR setADC(unsigned int index){

	if ((index >> 0) & 0x0001){
		gpio_output_set(0, BIT16, BIT16, 0);
	} else{
		gpio_output_set(BIT16, 0, BIT16, 0);
	}

	if ((index >> 1) & 0x0001){
		gpio_output_set(0, BIT12, BIT12, 0);
	} else{
		gpio_output_set(BIT12, 0, BIT12, 0);
	}

	if ((index >> 2) & 0x0001){
		gpio_output_set(0, BIT13, BIT13, 0);
	} else{
		gpio_output_set(BIT13, 0, BIT13, 0);
	}
}

uint16 adcindex = 0;
void ICACHE_FLASH_ATTR sendADCData(){

	os_printf("send ADC Data\r\n");

	
	#define adcValueBuffferLength	32

	uint16 adc;
	uint16 adcValueBufffer[adcValueBuffferLength];
	uint16 adcValue;
	uint16 adcValueIndex;
	uint16 length;
	uint16 clockDivider;
	double adcValueSum;

	length = adcValueBuffferLength;
	clockDivider = 8;

	char *mqttTopic = NULL;
	mqttTopic = (char *) os_calloc(96, sizeof(char));

	char *mqttValue = NULL;
	mqttValue = (char *) os_calloc(8, sizeof(char));

	gpio_output_set(BIT15, 0, BIT15, 0);
	for (adc = 0; adc < 8; adc++){

		setADC(adc);
		os_delay_us(65535);

		ets_intr_lock();		 //close	interrupt
		system_adc_read_fast(adcValueBufffer,	length,	clockDivider);
		ets_intr_unlock();	 	 //open	interrupt

		adcValueSum = 0.0;
		for(adcValueIndex = 0; adcValueIndex < length; adcValueIndex++){
			adcValueSum+= (double) adcValueBufffer[adcValueIndex];;
		}

		adcValue = (unsigned int) (adcValueSum / length);

		os_sprintf(mqttValue, "%d", adcValue);

		mqttTopic = strcpy(mqttTopic, sysCfg.mqtt_topicroot);
		switch (adc){
			case 0:
				mqttTopic = strcat(mqttTopic, sysCfg.mqtt_topicadc0);
				break;
			case 1:
				mqttTopic = strcat(mqttTopic, sysCfg.mqtt_topicadc1);
				break;
			case 2:
				mqttTopic = strcat(mqttTopic, sysCfg.mqtt_topicadc2);
				break;
			case 3:
				mqttTopic = strcat(mqttTopic, sysCfg.mqtt_topicadc3);
				break;
			case 4:
				mqttTopic = strcat(mqttTopic, sysCfg.mqtt_topicadc4);
				break;
			case 5:
				mqttTopic = strcat(mqttTopic, sysCfg.mqtt_topicadc5);
				break;
			case 6:
				mqttTopic = strcat(mqttTopic, "/BatteryVoltage");
				break;
			case 7:
				mqttTopic = strcat(mqttTopic, "/InputVoltage");
				break;
		}

		os_printf("%s = %s\r\n", mqttTopic, mqttValue);

		MQTT_Publish(mqttClient, mqttTopic, mqttValue, strlen(mqttValue), 1, 1);
	}

	gpio_output_set(0, BIT15, BIT15, 0);

	// char valueString[9];
	// char pathString[24];

	// for (adc = 0; adc < 8; adc++){

	// 	adcValue = adcValues[adc];

	// 	os_sprintf(valueString, "%d", adcValue);
	// 	os_sprintf(pathString, "/sensor/adc%d/current", (unsigned int) adc);

	// 	os_printf("ADC:%d = %d\r\n", adc, adcValue);
	// 	MQTT_Publish(mqttClient, pathString, valueString, strlen(valueString), 1, 1);

	// }

	

	counter++;
	os_printf("COUNTER :%d\r\n\r\n\r\n", counter);
}


static os_timer_t goToSleep_timer;
void goToSleep(void *arg){
	os_timer_disarm(&goToSleep_timer);

	// If the queue is not empty
	if (mqttClient->msgQueue.rb.fill_cnt > 0) {

		// Keep the IC awake and let the queue process
		os_timer_arm(&goToSleep_timer, 10, 1);
	}

	// If the queue is empty, put the IC to sleep
	else {
		wifi_set_sleep_type(LIGHT_SLEEP_T);
	}
	
}

uint32 lastWakeTime = 0;
void ICACHE_FLASH_ATTR sleepWakeOnInterruptHandeler(int * arg){
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	uint32 thisWakeTime = system_get_rtc_time();

	// De-bounce interrupt
	if ((thisWakeTime - lastWakeTime) < 10000){
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
		return;
	}

	lastWakeTime = system_get_rtc_time();

	// Recharge and start discharging capacitor
	gpio_output_set(BIT14, 0, BIT14, 0);// Output Set &= 1
	os_delay_us(1000);
	gpio_output_set(BIT14, 0, 0, BIT14);// Input Set

	os_printf("Interrupt\r\n");

	wifi_check_ip();

	wakeCount++;
	if (wakeCount >= 4) {

		wakeCount = 0;

		wifi_set_sleep_type(NONE_SLEEP_T);

		mqtt_timer(mqttClient);

		sendADCData();

		os_timer_disarm(&goToSleep_timer);
		os_timer_arm(&goToSleep_timer, 10, 1);

	}

	

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}



void ICACHE_FLASH_ATTR sleepInit(uint32_t *args){

	//gpio_output_set(BIT15, 0, 0, BIT15);// Input Set
	gpio_output_set(BIT0, 0, 0, BIT0);// Input Set
	gpio_output_set(BIT1, 0, 0, BIT1);// Input Set
	// MTDO, U0TXD and GPIO0


	//Setup 415 pins
	gpio_output_set(0, BIT15, BIT15, 0);

	gpio_output_set(BIT13, 0, BIT13, 0);
	gpio_output_set(BIT12, 0, BIT12, 0);
	gpio_output_set(BIT16, 0, BIT16, 0);


	mqttClient = (MQTT_Client*) args;

	// Configure GPIO14 to act as an interrupt to wake the IC and to run a task.
	// Running a task on interrupt instead of timers enabled the IC to go back to sleep sooner.
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U); // Or else the capacitor on the pin won't discharge.

	// Output Set &= 0
	gpio_output_set(0, BIT14, BIT14, 0);

	gpio_pin_wakeup_enable(14, GPIO_PIN_INTR_LOLEVEL);// Attach wake from light sleep when low to GPIO14
	gpio_pin_intr_state_set(14, GPIO_PIN_INTR_LOLEVEL);// Attach interrupt task to run on GPIO14
	ets_isr_attach(ETS_GPIO_INUM, (ets_isr_t)sleepWakeOnInterruptHandeler, NULL);// ^

	os_timer_disarm(&goToSleep_timer);
	os_timer_setfn(&goToSleep_timer, (os_timer_func_t *)goToSleep, NULL);

	ETS_GPIO_INTR_ENABLE();// Enable interrupts

}