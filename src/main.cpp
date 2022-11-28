#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "DHT.hpp"

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t myTaskHandle2 = NULL;
TaskHandle_t myTaskHandle3 = NULL;

// Aufruf von DHT lesen
void DHT_task(void *pvParameter)
{
    static const char *TAG = "DHT";
    DHT dht;
    dht.setDHTgpio(GPIO_NUM_4);
    ESP_LOGI(TAG, "Starting DHT Task\n\n");

    while(true)
    {
        ESP_LOGI(TAG, "=== Reading DHT ===\n");
        int ret = dht.readDHT();

        dht.errorHandler(ret);

        ESP_LOGI(TAG, "Hum: %.1f Tmp: %.1f\n", dht.getHumidity(), dht.getTemperature());

        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !!
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

// Funktion LED blinken lassen (GPIO_NUM_2)
void blink_task(void *pvParameter)
{
    static const char *TAG = "BLINK";
    ESP_LOGI(TAG, "Starting BLINK Task\n\n");
    while(true)
    {
        ESP_LOGI(TAG, "LED ON");
        ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_2,1));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "LED OFF");
        ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_2,0));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Einlesen von LDR-Switch (GPIO_NUM_16) -> Ausgang 3 setzen
void ldrSwitch_task(void *pvParameter)
{
    static const char *TAG = "LDRSWITCH";
    ESP_LOGD(TAG, "Starting the LDR switch Task\n\n");
    while (true)
    {
        if (gpio_get_level(GPIO_NUM_16) == 1)
        {
            ESP_LOGD(TAG, "detected.....");
            gpio_set_level(GPIO_NUM_3, 1);
        }
        else
        {
            gpio_set_level(GPIO_NUM_3, 0);
        }
    }
}

// Überladung von Daten zulassen in C++ möglich
extern "C"
{
    // Aufruf des Hauptprogramm
    void app_main() 
    {
        // Initialize the ESP-FlashRAM with NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ret = nvs_flash_init();
        ESP_ERROR_CHECK(ret);

        esp_log_level_set("*", ESP_LOG_INFO);

        /*
        // single GPIO selcetion
        gpio_pad_select_gpio(GPIO_NUM_2);
        gpio_pad_select_gpio(GPIO_NUM_3);
        // input or output or both
        gpio_set_direction(GPIO_NUM_2,GPIO_MODE_OUTPUT);
        // input or output or both with error check
        ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_3,GPIO_MODE_OUTPUT));
        // output set
        gpio_set_level(GPIO_NUM_2,1);
        // output set with error check
        ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_3,1));*/

        // multi GPIO (Template) selection - reihenfolge muss beachtet werden
        gpio_config_t gpioOutputConfig{
            GPIO_SEL_2 | GPIO_SEL_3,
            GPIO_MODE_OUTPUT,
            GPIO_PULLUP_DISABLE,
            GPIO_PULLDOWN_DISABLE,
            GPIO_INTR_DISABLE
        };
        gpio_config_t gpioInputConfig{
            GPIO_SEL_16 | GPIO_SEL_5,
            GPIO_MODE_INPUT,
            GPIO_PULLUP_DISABLE,
            GPIO_PULLDOWN_DISABLE,
            GPIO_INTR_DISABLE
        };
        gpio_config(&gpioOutputConfig);
        gpio_config(&gpioInputConfig);

        // call function blink_task
        xTaskCreate(&blink_task, "Blink", configMINIMAL_STACK_SIZE, NULL, 1, &myTaskHandle);
        // call function ldrSwitch_task
        xTaskCreate(&ldrSwitch_task, "LDR_Switch", configMINIMAL_STACK_SIZE, NULL, 2, &myTaskHandle2);
        // call function DHT_task
        xTaskCreate(&DHT_task, "DHT_task", 2048, NULL, 3, &myTaskHandle3);

    }
}