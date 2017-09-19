#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mqtt.h"

unsigned int counter = 0;
unsigned int readyToSleep = 1;
unsigned int wakeCount = 0;
LOCAL MQTT_Client* mqttClient;

void ICACHE_FLASH_ATTR sendADCData(){

	os_printf("send ADC Data\r\n");

	uint16	adc_addr[100];
	uint16	adc_num		=	100;
	uint32	i;
	double sum;
	unsigned int vBatt;
	char stringBuffer[9];

	ets_intr_lock();		 //close	interrupt
	system_adc_read_fast(adc_addr,	adc_num,	8);
	ets_intr_unlock();	 	 //open	interrupt

	for(i=0; i < adc_num; i++){
		sum+= (double) adc_addr[i];
	}
	
	vBatt = (unsigned int) ((sum / adc_num) * 5);
	
	os_sprintf(stringBuffer, "%d", vBatt);
	os_printf("%d\r\n", vBatt);
	MQTT_Publish(mqttClient, "/sensor/test/current", stringBuffer, strlen(stringBuffer), 1, 1);

	

	counter++;
	os_printf("COUNTER :%d\r\n\r\n\r\n", counter);
}


static os_timer_t goToSleep_timer;
void goToSleep(void *arg){
	os_timer_disarm(&goToSleep_timer);

	// If the queue is not empty
	if (mqttClient->msgQueue.rb.fill_cnt > 0) {

		// Keep the IC awake and let the queue process
		os_timer_arm(&goToSleep_timer, 1, 1);
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

	if ((thisWakeTime - lastWakeTime) < 10000){
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
		return;
	}

	lastWakeTime = system_get_rtc_time();


	gpio_output_set(BIT14, 0, BIT14, 0);// Output Set &= 1
	os_delay_us(20000);
	gpio_output_set(BIT14, 0, 0, BIT14);// Input Set

	os_printf("Interrupt\r\n");

	wakeCount++;
	if (wakeCount >= 4) {

		wakeCount = 0;

		wifi_set_sleep_type(NONE_SLEEP_T);

		sendADCData();

		os_timer_disarm(&goToSleep_timer);
		os_timer_arm(&goToSleep_timer, 10, 1);
	}

	

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}



void ICACHE_FLASH_ATTR sleepInit(uint32_t *args){

	gpio_output_set(BIT15, 0, 0, BIT15);// Input Set
	gpio_output_set(BIT0, 0, 0, BIT0);// Input Set
	gpio_output_set(BIT1, 0, 0, BIT1);// Input Set
	// MTDO, U0TXD and GPIO0


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