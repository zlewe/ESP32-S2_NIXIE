#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#define SPI_HOST        SPI2_HOST
#define PIN_NUM_MOSI    5
#define PIN_NUM_CLK     6
#define PIN_NUM_LATCH   4

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

// #define GPIO_OUTPUT_IO_0    4
// #define GPIO_OUTPUT_IO_1    5
// #define GPIO_OUTPUT_IO_2    6
// #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1) | (1ULL<<GPIO_OUTPUT_IO_2))

static const char* TAG = "main_nixie";

void spi_pre_transfer_callback(spi_transaction_t *t)
{
    esp_err_t ret;
    ret = gpio_set_level(PIN_NUM_LATCH, 0);
    ESP_ERROR_CHECK(ret);
}

void spi_post_transfer_callback(spi_transaction_t *t)
{
    esp_err_t ret;
    ret = gpio_set_level(PIN_NUM_LATCH, 1);
    ESP_ERROR_CHECK(ret);
}

static void init_reg(spi_device_handle_t *spi)
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=16
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=1*1000*1000,
        .mode=0,
        .spics_io_num=-1,
        .queue_size=2,
        .pre_cb=spi_pre_transfer_callback,
        .post_cb=spi_post_transfer_callback
    };
    ret=spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);
    ret=spi_bus_add_device(SPI_HOST, &devcfg, spi);
    ESP_ERROR_CHECK(ret);

    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL<<PIN_NUM_LATCH);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    ret = gpio_config(&io_conf);
    ESP_ERROR_CHECK(ret);


    //set latch to LOW
    ret = gpio_set_level(PIN_NUM_LATCH, 0);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Init 74595 Shift Register");
}

static void send_digits(spi_device_handle_t spi, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4)
{
    esp_err_t ret;
    static spi_transaction_t trans;
    memset(&trans, 0, sizeof(spi_transaction_t));
    trans.length=16;
    trans.flags = SPI_TRANS_USE_TXDATA;

    trans.tx_data[0]= (d4 << 4) + d3;    
    trans.tx_data[1]= (d2 << 4) + d1;

    ESP_LOGV(TAG, "d4: %d, d3: %d\n", d4, d3);
    ESP_LOGV(TAG, "d2: %d, d1: %d\n", d2, d1);
    //ESP_LOGI(TAG, "m: "BYTE_TO_BINARY_PATTERN" \n", BYTE_TO_BINARY(trans.tx_data[0]));
    ESP_LOGV(TAG, "m: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"", BYTE_TO_BINARY(trans.tx_data[0]), BYTE_TO_BINARY(trans.tx_data[1]));
    ret=spi_device_queue_trans(spi, &trans, portMAX_DELAY);
    assert(ret==ESP_OK);
}

void loop_9999(void * parameters)
{
    spi_device_handle_t spi = (spi_device_handle_t) parameters;

    ESP_LOGI(TAG, "Looping 1 - 9999");
    while(1){
        for(int i=0; i < 10000; i++){
            int n = i;
            uint8_t a, b, c, d;
            //9876
            a = n/1000; //9
            n %= 1000; //876
            b = n/100; //8
            n %= 100; //76
            c = n/10; //7
            d = n % 10; //6

            ESP_LOGV(TAG, "Sending %d %d %d %d", a, b, c, d);
            send_digits(spi, a, b, c, d);
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        send_digits(spi, 10, 10, 10, 10);
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void loop_4d()
{

}

void app_main(void)
{
    spi_device_handle_t spi;
    init_reg(&spi);
    vTaskDelay(100 / portTICK_RATE_MS);

    ESP_LOGI(TAG, "Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    xTaskCreate(
        loop_9999,
        "updateNixie",
        15000,
        (void *) spi,
        5,
        NULL
    );

    //loop_9999(spi);
}
