#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mqtt.h"

unsigned int counter = 0;
unsigned int mqttEnable = 0;
LOCAL MQTT_Client* mqttClient;


void ICACHE_FLASH_ATTR sleepSetEnable(){
	ETS_GPIO_INTR_ENABLE();// Enable interrupts
	wifi_set_sleep_type(LIGHT_SLEEP_T);
}
void ICACHE_FLASH_ATTR sleepSetDisable(){
	ETS_GPIO_INTR_DISABLE();// Disable interrupts
	wifi_set_sleep_type(NONE_SLEEP_T);
}


void ICACHE_FLASH_ATTR sleepWakeOnInterruptHandeler(int * arg){
	ets_intr_lock();
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	

	sleepSetDisable();
	
	gpio_output_set(BIT14, 0, BIT14, 0);// Output Set &= 1

	unsigned int adc_num = 50;
	unsigned int i;
	double sum;
	for(i=0; i < adc_num; i++){
		os_delay_us(1666);
		sum+= (double) system_adc_read();
	}

	unsigned int vBatt;

	vBatt = (unsigned int) ((sum / adc_num) * 5);

	char adcValueString[6];
	os_sprintf(adcValueString, "%d", vBatt);
	os_printf("%d\r\n", vBatt);
	MQTT_Publish(mqttClient, "/sensor/test/current", adcValueString, 4, 1, 1);

	
	gpio_output_set(BIT14, 0, 0, BIT14);// Input Set

	counter++;
	os_printf("Wake Count :%d\n", counter);

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
	ets_intr_unlock();
}



void ICACHE_FLASH_ATTR sleepInit(uint32_t *args){

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
}