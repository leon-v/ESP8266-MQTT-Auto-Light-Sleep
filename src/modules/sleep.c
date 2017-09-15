#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mqtt.h"

unsigned int counter = 0;
unsigned int readyToSleep = 0;
LOCAL MQTT_Client* mqttClient;
static os_timer_t sleepReminderTimer;

static os_timer_t sleepDischargeCapTimer;

void sleepDischargeCapTask(void *arg){
	gpio_output_set(BIT14, 0, 0, BIT14);// Input Set
	readyToSleep = 1;
}

void ICACHE_FLASH_ATTR sleepSetEnable(){

	if (!readyToSleep) {
		os_timer_disarm(&sleepReminderTimer);
		os_timer_arm(&sleepReminderTimer, 10, 0);
		return;
	}

	//system_set_os_print(0);
	ETS_GPIO_INTR_ENABLE();// Enable interrupts
	wifi_set_sleep_type(LIGHT_SLEEP_T);
}
void ICACHE_FLASH_ATTR sleepSetDisable(){

	os_timer_disarm(&sleepReminderTimer);

	//system_set_os_print(1);
	ETS_GPIO_INTR_DISABLE();// Disable interrupts
	wifi_set_sleep_type(NONE_SLEEP_T);
}

void sleepReminderTask(void *arg){
	sleepSetEnable();
}

void ICACHE_FLASH_ATTR sleepWakeOnInterruptHandeler(int * arg){
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	sleepSetDisable();

	uint16	adc_addr[100];
	uint16	adc_num		=	100;
	uint8	adc_clk_div	=	16;
	uint32	i;
	double sum;
	unsigned int vBatt;
	char adcValueString[6];

	system_adc_read_fast(adc_addr,	adc_num,	adc_clk_div);

	for(i=0; i < adc_num; i++){
		sum+= (double) adc_addr[i];
	}
	
	vBatt = (unsigned int) ((sum / adc_num) * 5);
	
	os_sprintf(adcValueString, "%d", vBatt);
	os_printf("%d\r\n", vBatt);
	MQTT_Publish(mqttClient, "/sensor/test/current", adcValueString, 4, 1, 1);

	
	counter++;
	os_printf("Wake Count :%d\n", counter);

	gpio_output_set(BIT14, 0, BIT14, 0);// Output Set &= 1

	readyToSleep = 0;
	os_timer_disarm(&sleepDischargeCapTimer);
	os_timer_arm(&sleepDischargeCapTimer, 10, 0);

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
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

	os_timer_disarm(&sleepReminderTimer);
	os_timer_setfn(&sleepReminderTimer, (os_timer_func_t *)sleepReminderTask, NULL);

	os_timer_disarm(&sleepDischargeCapTimer);
	os_timer_setfn(&sleepDischargeCapTimer, (os_timer_func_t *)sleepDischargeCapTask, NULL);

	ETS_GPIO_INTR_ENABLE();// Enable interrupts
}