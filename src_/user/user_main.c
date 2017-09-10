#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "driver/uart.h"

void ICACHE_FLASH_ATTR  wifi_handle_event_cb(System_Event_t *evt) {
    os_printf("event %x\n", evt->event);
	int res;
    switch (evt->event) {
         case EVENT_STAMODE_GOT_IP:

            break;
         default:
             break;
 }
}

LOCAL os_timer_t taskServiceTimer;
void ICACHE_FLASH_ATTR taskService(){
	os_printf("Task Service\r\n");

	
}

unsigned int counter = 0;

void ICACHE_FLASH_ATTR gpioInterruptTask(int * arg){

	os_timer_arm(&taskServiceTimer, 0, 0);

	gpio_output_set(BIT14, 0, BIT14, 0);// Output Set &= 1
	os_delay_us(65535);
	gpio_output_set(BIT14, 0, 0, BIT14);// Input Set

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

	if ((++counter % 256) == 0){
		
		wifi_set_sleep_type(NONE_SLEEP_T);
		os_delay_us(100);
		wifi_set_sleep_type(LIGHT_SLEEP_T);

		os_printf("Wake Count :%d\n", counter);
	}
}


void ICACHE_FLASH_ATTR user_init(void) {
	
    os_printf("SDK version:%s\n", system_get_sdk_version());
	
	// Configure the Wifi connection
    struct station_config stationConf;
	wifi_set_opmode_current(STATION_MODE);
	os_memset(&stationConf, 0, sizeof(struct station_config));
	os_sprintf(stationConf.ssid, WIFI_SSID);
	os_sprintf(stationConf.password, WIFI_PASS);
	wifi_station_set_config_current(&stationConf);
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	wifi_station_connect();

	// Configure GPIO14 to act as an interrupt to wake the IC and to run a task.
	// Running a task on interrupt instead of timers enabled the IC to go back to sleep sooner.
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U); // Or else the capacitor on the pin won't discharge.

	// Output Set &= 0
	gpio_output_set(0, BIT14, BIT14, 0);

	os_timer_disarm(&taskServiceTimer);
	os_timer_setfn(&taskServiceTimer, (os_timer_func_t *) taskService, NULL);

	gpio_pin_wakeup_enable(14, GPIO_PIN_INTR_LOLEVEL);// Attach wake from light sleep when low to GPIO14
	gpio_pin_intr_state_set(14, GPIO_PIN_INTR_LOLEVEL);// Attach interrupt task to run on GPIO14
	ets_isr_attach(ETS_GPIO_INUM, (ets_isr_t)gpioInterruptTask, NULL);// ^

	os_delay_us(65535);

	ETS_GPIO_INTR_ENABLE();// Enable interrupts
	
	wifi_set_sleep_type(LIGHT_SLEEP_T);
}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void){
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
        	rf_cal_sec = 128 - 5;
        	break;

        case FLASH_SIZE_8M_MAP_512_512:
        	rf_cal_sec = 256 - 5;
        	break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}