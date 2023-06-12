#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_phy_init.h"
#include "esp_sntp.h"                                                                                                                               
#include "esp_sleep.h"

#define TAG "SBT:"

// See https://stackoverflow.com/questions/6671698/adding-quotes-to-argument-in-c-preprocessor about quoting in #define  
#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef CHIP_NUMBER_NUM 
#define CHIP_NUMBER_NUM 1
#define CHIP_NUMBER QUOTE(1)
#else
#define CHIP_NUMBER QUOTE(CHIP_NUMBER_NUM)
#endif

#if CHIP_NUMBER_NUM < 3
#define USER_INFO_PIN GPIO_NUM_9
#define USER_LED      GPIO_NUM_21
#define BATTERY_MONITOR ADC_CHANNEL_0
#define BATTERY_GPIO_NUM GPIO_NUM_1 
#else
#define USER_INFO_PIN GPIO_NUM_9
#define USER_LED      GPIO_NUM_8 
#define BATTERY_MONITOR ADC_CHANNEL_2
#define BATTERY_GPIO_NUM GPIO_NUM_2
#endif

#define ADC1_ATTEN           ADC_ATTEN_DB_11	//ADC_ATTEN_DB_0 = No input attenuation, ADC can measure up to Vref which is approx 1100mV (factory test value burnt into eFuse)
												//ADC_ATTEN_DB_2_5 = input attenuated extending the range of measurement by about 2.5 dB (1.33 x)
												//ADC_ATTEN_DB_6 = input attenuated extending the range of measurement by about 6 dB (2 x)
												//ADC_ATTEN_DB_11 = input attenuated extending the range of measurement by about 11 dB (3.55 x)


#define USER_LED_ON 0
#define USER_LED_OFF 1

#define ESP_MAXIMUM_RETRY 3
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""

/* Count the number of boots. */
static RTC_DATA_ATTR unsigned int boot_count = 0;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_HAS_AP_BIT       BIT1
#define WIFI_DISCONNECTED_BIT BIT2
#define WIFI_STOP_BIT         BIT3
#define WIFI_FAIL_BIT         BIT4

#define MQTT_CONNECTED_BIT BIT6
#define MQTT_FAIL_BIT      BIT7
#define MQTT_PUBLISHED_BIT BIT8

#define WIFI_STATUS_START 0
#define WIFI_STATUS_TRY_CONNECT 1
#define WIFI_STATUS_CONNECTED 2
#define WIFI_STATUS_DISCONNECTED 3
#define WIFI_STATUS_STOP 4

/* Count the number of connection tries.*/
static int s_wifi_status = -1;
static int s_retry_num = 0;

/* Specify the deep sleep time */
static unsigned long s_deep_sleep_time = 17000000ULL;

/* FreeRTOS event queue for inter process communication.*/
QueueHandle_t gpio_evt_queue = NULL;
QueueHandle_t gpio_phase_queue = NULL;



//********** A TO D CALIBRATION INITIALISE **********
bool ADCCalInitialise (adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

	if (!calibrated)
	{
		ESP_LOGI(TAG, "AtoD Calibrate - Calibration scheme version is %s", "Curve Fitting");
		adc_cali_curve_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
		if (ret == ESP_OK)
			calibrated = true;
	}

	*out_handle = handle;
	if (ret == ESP_OK)
		ESP_LOGI(TAG, "AtoD Calibrate - Calibration Success");
	else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
		ESP_LOGW(TAG, "AtoD Calibrate - eFuse not burnt, skip software calibration");
	else
		ESP_LOGE(TAG, "AtoD Calibrate - Invalid arg or no memory");

	return calibrated;
}

static void handle_wifi_connection(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    if (event_base == WIFI_EVENT){
        switch(event_id){
            case WIFI_EVENT_STA_START:
                printf("Wfi Event Start, try esp_wifi_connect()\n");
                fflush(stdout);
                s_wifi_status = WIFI_STATUS_TRY_CONNECT;
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_wifi_status == WIFI_STATUS_TRY_CONNECT){
                    if (s_retry_num < ESP_MAXIMUM_RETRY) {
                        printf("retry to connect to the AP\n");
                        fflush(stdout);
                        esp_wifi_connect();
                        s_retry_num++;
                   } else {
                        printf("!!!!! connect to the AP failed !!!!!!\n");
                        fflush(stdout);
                        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                    }
                } else {
                    printf("Wifi disconnecting\n");
                    fflush(stdout);
                    xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
                    s_wifi_status = WIFI_STATUS_DISCONNECTED;
                }
            break;
            case WIFI_EVENT_STA_CONNECTED:
                printf("connected to AP\n");
                fflush(stdout);
                s_wifi_status = WIFI_STATUS_CONNECTED;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
            case WIFI_EVENT_STA_STOP:
                printf("Wifi stopping \n");
                fflush(stdout);
                s_wifi_status = WIFI_STATUS_STOP;
                xEventGroupSetBits(s_wifi_event_group, WIFI_STOP_BIT);
            break;
            default:
                printf("Wifi event %ld\n", event_id);
                fflush(stdout);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("got ip: " IPSTR " \n", IP2STR(&event->ip_info.ip));
        fflush(stdout);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_HAS_AP_BIT);
    } else {
        printf("UNEXPECTED EVENT: %s  id: %ld\n", event_base, event_id);
        fflush(stdout);
    }
}

void wifi_start(void){
    // Start the event loop, and register WiFi callbacks.
    // Can replace the last NULL with a pointer to an esp_event_handler_instance_t.
    esp_event_loop_create_default();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handle_wifi_connection, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_wifi_connection, NULL, NULL);
    
    s_wifi_status = WIFI_STATUS_START;

   wifi_config_t wifi_config = {
    .sta = {
        .ssid = "Hobbit",
        .password = "magdalena",
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg = {
             .capable = true,
             .required = false},
        },
    };

    s_wifi_event_group = xEventGroupCreate();

    esp_netif_init(); // Initialize the underlying TCP/IP stack.
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    esp_wifi_set_mode(WIFI_MODE_STA); // Station mode. 
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    printf("Wifi start finished. \n");
    fflush(stdout);
}

static esp_mqtt_client_handle_t mqtt_client = NULL;

static void handle_mqtt_events(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        printf("MQTT:: mqtt broker connected\n");
        xEventGroupSetBits(s_wifi_event_group, MQTT_CONNECTED_BIT);
        // We are connected.
        break;
    case MQTT_EVENT_DISCONNECTED:
        printf("MQTT:: mqtt broker disconnected\n");
        xEventGroupSetBits(s_wifi_event_group, MQTT_FAIL_BIT);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        printf("MQTT:: mqtt broker subscribed\n");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        printf("MQTT:: mqtt broker unsubscribed\n");
        break;
    case MQTT_EVENT_PUBLISHED:
        printf("MQTT:: mqtt data published\n");
        xEventGroupSetBits(s_wifi_event_group, MQTT_PUBLISHED_BIT);
        break;
    case MQTT_EVENT_DELETED:
        printf("MQTT:: mqtt broker deleted event %d\n",event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        if(!strncmp(event->topic, "sleepbatterytest", event->topic_len) && !strncmp(event->data, "turnoff", event->data_len)){
            printf("Turning system off. Actually, not.\n");
        }else{
            printf("MQTT:: mqtt broker event data not recognized: %s\n",event->data);
        }
        break;
    case MQTT_EVENT_ERROR:
        printf("MQTT:: ERROR: errtype: %d \n", event->error_handle->error_type);
        break;
    
    default:
        printf("MQTT:: Unhandled event: %ld\n", event_id);
        break;
    }
    fflush(stdout);
}

// void time_sync_notification_cb(struct timeval *tv)
// {
//     ESP_LOGI(TAG, "Notification of a time synchronization event");
// }
//
// static void initialize_sntp(void)
// {
//     ESP_LOGI(TAG, "Initializing SNTP");
//     sntp_setoperatingmode(SNTP_OPMODE_POLL);
//     sntp_setservername(0, "pool.ntp.org");
//     sntp_set_time_sync_notification_cb(time_sync_notification_cb);
// #ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
//     sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
// #endif
//     sntp_init();
// }


void app_main() {

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BATTERY_GPIO_NUM),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);
    gpio_config_t io_conf_out = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << USER_INFO_PIN) | (1ULL << USER_LED),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf_out);
    gpio_set_level(USER_INFO_PIN, 1);
    gpio_set_level(USER_LED, USER_LED_ON);  // User LED on.

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    boot_count++;
    printf("Sleep Battery Test " CHIP_NUMBER " start. Boot # %d\n",boot_count);
    printf(" ADC Channel used = %d  %d\n", BATTERY_MONITOR, BATTERY_GPIO_NUM);
    fflush(stdout);
    
   // The NVS partition is used by the WiFi library.
    if (nvs_flash_init() != ESP_OK)
    {
        printf("ERROR: NVS initialization failed.\nErasing all settings.\n");
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_start();

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdTRUE,     // Clear bits if they were set before returning.
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        printf("connected to Wifi\n");
        gpio_set_level(USER_LED, USER_LED_OFF); 
    } else if (bits & WIFI_FAIL_BIT) {
        printf("Failed to connect to Wifi. Erase PHY Calibration, then doing full reboot. \n");
        fflush(stdout);
        esp_phy_erase_cal_data_in_nvs();
        for(int i=0; i<10; i++){
            gpio_set_level(USER_LED, USER_LED_ON); 
            vTaskDelay(200 / portTICK_PERIOD_MS);
            gpio_set_level(USER_LED, USER_LED_OFF); 
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        // esp_wifi_disconnect();
        // bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT, pdTRUE, pdFALSE, 10000/portTICK_PERIOD_MS);
        // printf("Disconnected from Wifi\n");
        // fflush(stdout);
        // esp_wifi_stop();
        // printf("Wifi stopped\n");
        // fflush(stdout);
        // esp_wifi_deinit();
        // esp_deep_sleep(1000ULL);
        esp_restart();  // Hard restart will reset boot count, but also fully reset the WiFi. Chip #2 does allow WiFi after deep sleep :-(.
    } else {
        printf("UNEXPECTED EVENT\n");
    }
    gpio_set_level(USER_INFO_PIN, 0);
    printf("Boot num: %d\n",boot_count);
    fflush(stdout);

    bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_HAS_AP_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_HAS_AP_BIT) {
        printf("AP started\n");
        fflush(stdout);
    } else {
        printf("AP failed to start\n");
        fflush(stdout);
    }

    /* Now we can start up the MQTT system. */
    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = "mqtt://10.0.0.131",
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, handle_mqtt_events, NULL);
    esp_mqtt_client_start(mqtt_client);
    esp_log_level_set("mqtt_client",ESP_LOG_INFO);     // Silence the MQTT client debug, to avoind the timeout messages.
    esp_log_level_set("transport_base",ESP_LOG_ERROR); // Same with the transport_base.

    // Setup to read the battery voltage from A0. 
    // This should have two resistors of ~470k in a divider.

    adc_oneshot_unit_handle_t Adc1Handle;
    adc_cali_handle_t Adc1CalibrationHandle = NULL;
    int AdcRawValueChannel_0 = 0;
    int AdcVoltageChannel_0 = 0;

	//----- SETUP ADC, using ADC1 Channel 0 = A0 -----
	adc_oneshot_unit_init_cfg_t Adc1InitConfig = {
		.unit_id = ADC_UNIT_1,
	};
	adc_oneshot_new_unit(&Adc1InitConfig, &Adc1Handle);

	adc_oneshot_chan_cfg_t Adc1Config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC1_ATTEN,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(Adc1Handle, BATTERY_MONITOR, &Adc1Config));

	//Calibration Init
	ADCCalInitialise(ADC_UNIT_1, ADC1_ATTEN, &Adc1CalibrationHandle);

    gpio_set_level(USER_INFO_PIN, 0);

    // Wait for MQTT to be in connected state.
    bits = xEventGroupWaitBits(s_wifi_event_group,
            MQTT_CONNECTED_BIT,
            pdTRUE,     // Clear bits if they were set before returning.
            pdFALSE,
            portMAX_DELAY);
    printf("MQTT connected, start ADC read.\n");
    fflush(stdout);

    char buff[10];
    snprintf(buff, 10, "%6d", boot_count);
    esp_mqtt_client_publish(mqtt_client, "sleepbatterytest/" CHIP_NUMBER "/boot", buff, 0, 1, 0);

    gpio_set_level(USER_LED, USER_LED_ON); 

    bool keep_reading = true;
    while (keep_reading) 
    {
        gpio_set_level(USER_INFO_PIN, 1);
        
        //----- READ ADC_CHANNEL_0 -----
        if (adc_oneshot_read(Adc1Handle, BATTERY_MONITOR, &AdcRawValueChannel_0) == ESP_OK)		//<<Takes approx 130uS on ESP32S3
        {
            printf("Boot %d -- ADC Raw: %d", boot_count, AdcRawValueChannel_0);
            adc_cali_raw_to_voltage(Adc1CalibrationHandle, AdcRawValueChannel_0, &AdcVoltageChannel_0);
            printf(" Volt: %d mV   Vbat = %d mV \n", AdcVoltageChannel_0, 2*AdcVoltageChannel_0);
            char buffer[24];
            snprintf(buffer, 24, "%5.3f", 2.*AdcVoltageChannel_0/1000.0);
            esp_mqtt_client_publish(mqtt_client, "sleepbatterytest/" CHIP_NUMBER "/vbatt", buffer, 0, 1, 0);
            // If we run with QOS=1, then we can wait for the publish to complete. With QOS=0 there is no published event.
            // On the other hand, the QOS=0 may be faster, and will still work if ther is no broker, so the code continues.
            bits = xEventGroupWaitBits(s_wifi_event_group,
                    MQTT_PUBLISHED_BIT,
                    pdTRUE,
                    pdFALSE,
                    1000/ portTICK_PERIOD_MS);
            if( bits & MQTT_PUBLISHED_BIT){
                keep_reading = false;
            } else {
                keep_reading = true;
                printf("MQTT publish failed?\n");
                fflush(stdout);
            }
            keep_reading = false;
        } else {
            printf("ADC read failed\n");
            keep_reading = true;
        }
        gpio_set_level(USER_INFO_PIN, 0);
       if(boot_count == 1 && !keep_reading){
            // We have an extra wait time on the first boot so that we have a chance to re-flash the device.
            // When it is in deep-sleep, the USB will be disconnected, not permitting a re-flash.
            printf("Extra wait time for first boot. (30sec)\n");
            fflush(stdout);
            for(int i=0; i<30; i++){
                gpio_set_level(USER_LED, (i%2==0));  // User LED on/off.
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                printf(".");
                fflush(stdout);
            }
            printf(".\n");
            fflush(stdout);
            boot_count++;  // Pretend reboot, to break the loop.
            keep_reading = true;
        }else{
            vTaskDelay(1/ portTICK_PERIOD_MS);
        }
    }
    gpio_set_level(USER_LED, USER_LED_ON);
    esp_mqtt_client_stop(mqtt_client);
    esp_wifi_disconnect();
    bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT, pdTRUE, pdFALSE, 10000/portTICK_PERIOD_MS);
    printf("Disconnected from Wifi\n");
    fflush(stdout);
    esp_wifi_stop();
    esp_wifi_deinit();
    printf("Wifi stopped\n");
    fflush(stdout);
    printf("Going to deep sleep now\n");
    gpio_set_level(USER_LED, USER_LED_OFF);  // User LED off.
    esp_deep_sleep(s_deep_sleep_time); /* ~One minute - 3s of deep sleep, then it reboots. */
}