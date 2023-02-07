#include "esp32.h"
#include <string.h>
#include "board.h"
#include "uart.h"

/* **** Globals **** */
#define TX_END_CHAR '\r'
#define TX_END_CHAR_REP 3 /* Sends \r\r\r at end of TX, pattern identified in ESP */
#define TX_BUFF_SIZE 1024

#define ESP32_UART MXC_UART2
#define UART_BAUD 19200

/************************************ VARIABLES ******************************/
static uint8_t tx_buff[TX_BUFF_SIZE];

/************************************ Static Functions Declarations ******************************/
static void send_buff(uint16_t data_size);

/************************************ Extern Functions ******************************/
void esp32_init(void){
    MXC_UART_Init(ESP32_UART, UART_BAUD, MXC_UART_IBRO_CLK);
}

void esp32_send_img(uint8_t *img, uint32_t imgLen, uint16_t w, uint16_t h, uint8_t pixelformat){
    esp_msg_img_t msg;
    msg.img_len = imgLen;
    msg.width = w;
    msg.height = h;
    msg.pixel_format = pixelformat;
    
    /* Send info about image */
    tx_buff[0] = MSG_TYPE_IMG;
    memcpy(&tx_buff[1], &msg, sizeof(esp_msg_img_t));
    send_buff(sizeof(esp_msg_img_t)+1);
    
    /* Send image row by row */
    uint16_t i;
    for(i=0; i<h; i++){
        memcpy(tx_buff, &img[i*w], w);
        send_buff(w);
    }
}

/************************************ Static Functions ******************************/
static void send_buff(uint16_t data_size){
    uint8_t i;
    for(i=0; i<TX_END_CHAR_REP; i++){
        tx_buff[data_size + (uint16_t) i] = (uint8_t) TX_END_CHAR;
    }

    int len = (int) data_size + TX_END_CHAR_REP;
    MXC_UART_Write(ESP32_UART, tx_buff, &len);
}
