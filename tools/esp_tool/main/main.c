#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "max78000_cmds.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define MAX78000_UART UART_NUM_0
#define UART1 UART_NUM_2
#define PATTERN_CHR_NUM    (1)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (10050)
#define RD_BUF_SIZE (BUF_SIZE)

#define HOST_IP_ADDR "192.168.223.206"
#define PORT 60996

#define WIFI_SSID       "Arthur"
#define WIFI_PASS       "p99782959"
#define ESP_MAXIMUM_RETRY  5

static QueueHandle_t uart_queue;

static int          s_retry_num = 0;
static const char   *TAG = "uart_event";
static const char   *UDP_TAG = "UDP_event";
static const char   *WIFI_TAG = "wifi_event";

static uint8_t*     dtmp;
static int          pos = 0;

static max_msg_t active_msg;

static TaskHandle_t xUART_Task = NULL, xUDP_Task = NULL, xLCDHandle = NULL;

static void read_new_msg(){
    uint8_t msg_type = dtmp[0];
    ESP_LOGI(TAG, "Got msg type %d", (int) msg_type);
    switch (msg_type)
    {
    case MSG_TYPE_IMG: {
        active_msg.msg_type = msg_type;
        memcpy(&active_msg.img_msg, &dtmp[1], sizeof(esp_msg_img_t));
        active_msg.packets_recv = 0;
        ESP_LOGI(TAG, "Got MSG_TYPE_IMG, img size %lu", active_msg.img_msg.img_len);
        vTaskNotifyGiveFromISR(xUDP_Task, NULL);
        break;
    }
    default:
        break;
    }

}

static void msg_img_handler(){
    active_msg.packets_recv++;
    vTaskNotifyGiveFromISR(xUDP_Task, NULL);
    
    /* Check if this was the last row */
    if(active_msg.img_msg.height == active_msg.packets_recv){
        active_msg.msg_type = MSG_TYPE_NONE;
        ESP_LOGI(TAG, "Finished recv of image");
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    bzero(dtmp, RD_BUF_SIZE);
    active_msg.msg_type = MSG_TYPE_NONE;
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart_queue, (void * )&event, portMAX_DELAY)) {
            // ESP_LOGI(TAG, "uart[%d] event:", UART1);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    // ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    //uart_read_bytes(MAX78000_UART, dtmp, event.size, portMAX_DELAY);
                    //ESP_LOGI(TAG, "[DATA EVT]:");
                    //uart_write_bytes(MAX78000_UART, (const char*) dtmp, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART1);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART1);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(UART1, &buffered_size);
                    pos = uart_pattern_pop_pos(UART1);
                    // ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(UART1);
                    } else {
                        uart_read_bytes(UART1, dtmp, pos, 100 / portTICK_PERIOD_MS);
                        // ESP_LOGI(TAG, "Got a UART mex of %d characters", pos);

                        switch (active_msg.msg_type)
                        {
                        case MSG_TYPE_NONE:
                            read_new_msg();
                            break;
                        case MSG_TYPE_IMG:
                            msg_img_handler();
                            break;
                        default:
                            ESP_LOGI(TAG, "ERROR in msg handler");
                            break;
                        }

                        uart_flush_input(UART1);

                        //Notifiy UDP task that a new uart message was received.
                        // vTaskNotifyGiveFromISR(xUDP_Task, NULL);

                        //uart_read_bytes(UART1, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                        //uart_write_bytes(MAX78000_UART, "GOT MEX FROM UART\r\n", sizeof("GOT MEX FROM UART\r\n"));
                        //uart_write_bytes(MAX78000_UART, pos, sizeof(int));
                        //sendto(sock, dtmp, pos, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                        //uart_write_bytes(UART1, (const char*) dtmp, buffered_size);
                        //uart_write_bytes(UART1, "\r\n", sizeof("\r"));
                        //ESP_LOGI(TAG, "read data: %s", dtmp);
                    }
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));

    vEventGroupDelete(s_wifi_event_group);

    ESP_LOGI(WIFI_TAG, "WiFi init done");
}

static void udp_client_task(void *pvParameters)
{
    
    int     addr_family = 0;
    int     ip_protocol = 0;
    struct  sockaddr_in dest_addr;

    while (1) {

        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        dest_addr.sin_addr.s_addr = inet_addr("192.168.223.206");
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(60996);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

        if (sock < 0) {
            ESP_LOGE(UDP_TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(UDP_TAG, "Socket created, sending to %s:%d", "192.168.223.206", 60996);

        ESP_LOGI(UDP_TAG, "UDP Client Configured");

        while (1) {

            /* Block indefinitely (without a timeout, so no need to check the function's
                return value) to wait for a notification. */ 
            /* Clear the notification value before exiting. Block indefinitely. */
            ulTaskNotifyTake( pdTRUE, portMAX_DELAY ); 
            
            int err = sendto(sock, dtmp, pos, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

            if (err < 0) {
                ESP_LOGE(UDP_TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            ESP_LOGI(UDP_TAG, "UDP Packet Sent, size %d", pos);
            bzero(dtmp, RD_BUF_SIZE);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
}

void app_main(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());

    esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_log_level_set(WIFI_TAG, ESP_LOG_INFO);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 14400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART1, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);
    uart_param_config(UART1, &uart_config);

    uart_set_pin(UART1, 17, 16, -1, -1);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(UART1, '\r', 3, 9, 0, 0);

    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART1, 20);

    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Initializing UART done");
    
    wifi_init_sta();

    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, &xUART_Task);
    xTaskCreate(udp_client_task, "UDP_TX_task", 2048, NULL, 11, &xUDP_Task);
}
