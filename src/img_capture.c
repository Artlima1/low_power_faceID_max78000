#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "MAXCAM_Debug.h"
#include "camera.h"
#include "img_capture.h"
#include "led.h"
#include "mxc_delay.h"
#include "dma.h"
#include "icc.h"
#include "esp32.h"
#include "flash_memory.h"

#define S_MODULE_NAME "img_capture"
// #define PRINT_DEBUG

/* **** Globals **** */
#define DIF(a, b) (a>b) ? (a-b) : (b-a)

#define RED_MASK 0xF800 /* 1111 1000 0000 0000 */
#define RED_OFFSET 11
#define GREEN_MASK 0x7E0 /* 0000 0111 1110 0000 */
#define GREEN_OFFSET 5
#define BLUE_MASK 0x1F /* 0000 0000 0001 1111 */

#define BLOCKS_X (IMG_X_RES / BLOCK_PIXEL_W)
#define BLOCKS_Y (IMG_Y_RES / BLOCK_PIXEL_H)
#define BLOCKS_IN_CLUSTER (BLOCKS_X * BLOCKS_Y)

#define RGB_PIXEL_SIZE sizeof(rgb_t)
#define CLUSTER_SIZE (BLOCKS_IN_CLUSTER * RGB_PIXEL_SIZE)

#define CHECK_TOLERANCE(rgb) ((DIF(rgb.r, rgb.g)<LIGHT_TOLERANCE) && (DIF(rgb.r, rgb.b)<LIGHT_TOLERANCE ) && (DIF(rgb.g, rgb.b)<LIGHT_TOLERANCE))

/************************************ VARIABLES ******************************/

rgb_t * base_img;
int dma_channel;

/************************************ Static Functions Declarations ******************************/
static uint8_t take_base();
static uint8_t img_compare(void);
static void clusterize_image(uint16_t * img, rgb_t * dest);

/************************************ Extern Functions ******************************/
uint8_t img_capture(uint8_t capture_mode)
{
    uint8_t ret = IMG_CAP_RET_ERROR;

    camera_start_capture_image();

    while (1)
    {
        /* Check if camera image is ready to process */
        if (camera_is_image_rcv())
        {
            switch (capture_mode)
            {
            case IMAGE_CAPTURE_BASE:
            {
                ret = take_base();
                break;
            }
            case IMAGE_CAPTURE_COMPARE:
            {
                ret = img_compare();
                break;
            }
            default:
                break;
            }
            break;
        }
    }

    return ret;
}

uint8_t img_capture_init(void)
{
    int ret = 0;
    int slaveAddress;
    int id;

    // Initialize DMA for camera interface
    MXC_DMA_Init();
    dma_channel = MXC_DMA_AcquireChannel();

    /* Enable camera power */
    Camera_Power(1);
    MXC_Delay(300000);

    // Initialize the camera driver.
    camera_init(CAMERA_FREQ);

#ifdef PRINT_DEBUG
    printf("IMG_CAP: Init Camera");
#endif

    // Obtain the I2C slave address of the camera.
    slaveAddress = camera_get_slave_address();

#ifdef PRINT_DEBUG
    printf("IMG_CAP: Camera I2C slave address is %02x\n", slaveAddress);
#endif

    // Obtain the product ID of the camera.
    ret = camera_get_product_id(&id);

    if (ret != STATUS_OK)
    {
#ifdef PRINT_DEBUG
        printf("IMG_CAP: Error returned from reading camera id. Error %d\n", ret);
#endif
        return IMG_CAP_RET_ERROR;
    }

#ifdef PRINT_DEBUG
    printf("IMG_CAP: Camera Product ID is %04x\n", id);
#endif

    // Obtain the manufacture ID of the camera.
    ret = camera_get_manufacture_id(&id);

    if (ret != STATUS_OK)
    {
#ifdef PRINT_DEBUG
        printf("IMG_CAP: Error returned from reading camera id. Error %d\n", ret);
#endif
        return IMG_CAP_RET_ERROR;
    }

#ifdef PRINT_DEBUG
    printf("IMG_CAP: Camera Manufacture ID is %04x\n", id);
#endif

    ret = camera_setup(IMG_X_RES, IMG_Y_RES, PIXFORMAT_RGB565, FIFO_FOUR_BYTE, USE_DMA, dma_channel);
    if(ret != STATUS_OK){
        #ifdef PRINT_DEBUG
        printf("Error in camera setup %d\n", ret);
        #endif
        return IMG_CAP_RET_ERROR;
    }


    ret = flash_memory_init(CLUSTER_SIZE);
    if(ret != FM_RET_SUCCESS){
        // #ifdef PRINT_DEBUG
        printf("Error in flash memory setup\n");
        // #endif
        return IMG_CAP_RET_ERROR;
    }

    camera_write_reg(0x0c, 0x56); // camera vertical flip=0

    return IMG_CAP_RET_SUCCESS;
}

void img_capture_send_img(void){
    uint8_t *raw;
    uint32_t size, w, h;

    camera_get_image(&raw, &size, &w, &h);

    esp32_send_img(raw, size, w, h, PIXFORMAT_RGB565);
}

uint8_t img_capture_free_base(void){
    uint8_t ret = flash_memory_write((uint8_t *) base_img, CLUSTER_SIZE, 0);
    if(ret != FM_RET_SUCCESS){
        return IMG_CAP_RET_ERROR;
    }

    free(base_img);

    return IMG_CAP_RET_SUCCESS;
}

uint8_t img_capture_rec_base(void){
    if(base_img == NULL){
        base_img = malloc(CLUSTER_SIZE);
    }

    if(base_img==NULL){
        printf("Error in allocating memory for base\n");
        return IMG_CAP_RET_ERROR;
    }

    flash_memory_read((uint8_t *) base_img, CLUSTER_SIZE, 0);

    return IMG_CAP_RET_SUCCESS;

}
/************************************ Static Functions ******************************/

static uint8_t take_base()
{
    uint8_t *raw;
    uint32_t size, w, h;

    camera_get_image(&raw, &size, &w, &h);

    if(base_img == NULL){
        base_img = malloc(CLUSTER_SIZE);
    }

    if(base_img==NULL){
        printf("Error in allocating memory for base\n");
        return IMG_CAP_RET_ERROR;
    }

    clusterize_image((uint16_t *) raw, base_img);

    /* TODO - Store original in Flash */

    return IMG_CAP_RET_SUCCESS;
}

static uint8_t img_compare(void)
{
    uint8_t *raw;
    uint32_t size, w, h;
    rgb_t comp_img[CLUSTER_SIZE];

    camera_get_image(&raw, &size, &w, &h);
    clusterize_image((uint16_t *) raw, comp_img);

    uint32_t SAD = 0, i, dif;
    rgb_t rgb_dif;
    for (i = 0; i < BLOCKS_IN_CLUSTER; i++)
    {
        rgb_dif.r = DIF(base_img[i].r, comp_img[i].r);
        rgb_dif.g = DIF(base_img[i].g, comp_img[i].g);
        rgb_dif.b = DIF(base_img[i].b, comp_img[i].b);

        dif = rgb_dif.r + rgb_dif.g + rgb_dif.b;

        if(dif < MIN_BLOCK_DIF){
            continue;
        }

        SAD = SAD + dif;
    }

#ifdef PRINT_DEBUG
    printf("%u\n", SAD);
#endif

    if (SAD < SAD_THRESHOLD)
    {
        return IMG_CAP_RET_NO_CHANGE;
    }

#ifdef SEND_UART
    esp32_send_img(raw, size, w, h, PIXFORMAT_RGB565);
#endif

    return IMG_CAP_RET_CHANGE;
}

static void clusterize_image(uint16_t * img, rgb_t * dest){
    memset(dest, 0, CLUSTER_SIZE);

    uint8_t bx, by, px, py;
    rgb_t block_value;
    uint16_t * pixel;
    for(bx = 0; bx < BLOCKS_X; bx++){
        for(by=0; by < BLOCKS_Y; by++){
            memset(&block_value, 0, sizeof(rgb_t));

            for(px=0; px < BLOCK_PIXEL_W; px++){
                for(py=0; py < BLOCK_PIXEL_H; py++){
                    pixel = &img[(by*BLOCK_PIXEL_H + py)*IMG_X_RES + (bx*BLOCK_PIXEL_W + px)];

                    block_value.r += ((*pixel) & RED_MASK) >> RED_OFFSET;
                    block_value.g += ((*pixel) & GREEN_MASK) >> GREEN_OFFSET;
                    block_value.b += ((*pixel) & BLUE_MASK);
                }
            }

            memcpy(&dest[by*BLOCKS_X + bx], &block_value, sizeof(rgb_t));
        }
    }
}