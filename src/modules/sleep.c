#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mqtt.h"

unsigned int counter = 0;
unsigned int mqttEnable = 0;
LOCAL MQTT_Client* mqttClient;


void ICACHE_FLASH_ATTR sleepSetEnable(){
	mqttEnable = 1;
}
void ICACHE_FLASH_ATTR sleepSetDisable(){
	mqttEnable = 0;
}


LOCAL os_timer_t taskServiceTimer;
void ICACHE_FLASH_ATTR taskService(){
	char adcValueString[6];
	unsigned int adcValueRaw = system_adc_read();
	os_sprintf(adcValueString, "%d", adcValueRaw * 5);
	os_printf("%d\r\n", adcValueRaw * 5);
	MQTT_Publish(mqttClient, "test", adcValueString, 4, 1, 0);
}

void ICACHE_FLASH_ATTR sleepWakeOnInterruptHandeler(int * arg){
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	wifi_set_sleep_type(NONE_SLEEP_T);

	if (mqttEnable) {
		os_timer_disarm(&taskServiceTimer);
		os_timer_arm(&taskServiceTimer, 1, 0);
	}

	gpio_output_set(BIT14, 0, BIT14, 0);// Output Set &= 1
	os_delay_us(50000);
	gpio_output_set(BIT14, 0, 0, BIT14);// Input Set

	

	counter++;
	os_printf("Wake Count :%d\n", counter);

	if (mqttEnable) {
		wifi_set_sleep_type(LIGHT_SLEEP_T);
	}

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}



void ICACHE_FLASH_ATTR sleepInit(uint32_t *args){

	mqttClient = (MQTT_Client*) args;

	os_timer_disarm(&taskServiceTimer);
	os_timer_setfn(&taskServiceTimer, (os_timer_func_t *) taskService, NULL);

	// Configure GPIO14 to act as an interrupt to wake the IC and to run a task.
	// Running a task on interrupt instead of timers enabled the IC to go back to sleep sooner.
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U); // Or else the capacitor on the pin won't discharge.

	// Output Set &= 0
	gpio_output_set(0, BIT14, BIT14, 0);

	gpio_pin_wakeup_enable(14, GPIO_PIN_INTR_LOLEVEL);// Attach wake from light sleep when low to GPIO14
	gpio_pin_intr_state_set(14, GPIO_PIN_INTR_LOLEVEL);// Attach interrupt task to run on GPIO14
	ets_isr_attach(ETS_GPIO_INUM, (ets_isr_t)sleepWakeOnInterruptHandeler, NULL);// ^

	os_delay_us(65535);

	ETS_GPIO_INTR_ENABLE();// Enable interrupts
	
}