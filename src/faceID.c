#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc.h"
#include "camera.h"
#include "faceID.h"
#include "embedding_process.h"
#include "MAXCAM_Debug.h"
#include "cnn.h"

#include "led.h"
#include "lp.h"

#define S_MODULE_NAME "faceID"
#define PRINT_TIME 1

extern uint32_t ticks_1;
extern uint32_t ticks_2;
extern mxc_wut_cfg_t cfg;

/************************************ VARIABLES ******************************/
volatile uint32_t cnn_time; // Stopwatch
static int8_t prev_decision = -2;
static int8_t decision = -2;
static char *name;

/********************************* Static Functions Declarations **************************/
static void draw_frame(void);
static void run_cnn(int x_offset, int y_offset);
static void initialize_cnn(void);
static void load_data(uint8_t *raw, int x_offset, int y_offset);
/********************************* Exported Functions **************************/

faceID_decision_t faceid_run(void)
{
    faceID_decision_t ret;
    uint32_t run_count = 0;
    int ret_c = 0;

    initialize_cnn();

    uint8_t i;
    for(i=0; i<5; i++){
        camera_start_capture_image();
        while (!camera_is_image_rcv())
            ;
        draw_frame();

        /* Run CNN three times on original and shifted images */
        run_cnn(0, 0);

        if(decision > 0){
            break;
        }

        // if ((run_count % 2) == 0) {
        //     run_cnn(-IMG_SHIFT_ANALYSIS, -IMG_SHIFT_ANALYSIS);
        //     run_cnn(IMG_SHIFT_ANALYSIS, IMG_SHIFT_ANALYSIS);
        // } else {
        //     run_cnn(-IMG_SHIFT_ANALYSIS, IMG_SHIFT_ANALYSIS);
        //     run_cnn(IMG_SHIFT_ANALYSIS, -IMG_SHIFT_ANALYSIS);
        // }
        // run_count++;

        MXC_Delay(MSEC(10));
    }

    ret.decision = decision;
    ret.name = name;

    return ret;
}

/********************************* Static Functions **************************/
/* Draws a frame in the image */
static void draw_frame(void)
{
    uint32_t imgLen;
    uint32_t w, h;
    uint16_t *image;
    uint8_t *raw;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);

    image = (uint16_t *)raw; // 2bytes per pixel RGB565

    // left line
    image += ((IMG_Y_RES - (WIDTH + 2 * THICKNESS)) / 2) * IMG_X_RES;

    for (int i = 0; i < THICKNESS; i++)
    {
        image += ((IMG_X_RES - (HEIGHT + 2 * THICKNESS)) / 2);

        for (int j = 0; j < HEIGHT + 2 * THICKNESS; j++)
        {
            *(image++) = FRAME_COLOR; // color
        }

        image += ((IMG_X_RES - (HEIGHT + 2 * THICKNESS)) / 2);
    }

    // right line
    image = ((uint16_t *)raw) +
            (((IMG_Y_RES - (WIDTH + 2 * THICKNESS)) / 2) + WIDTH + THICKNESS) * IMG_X_RES;

    for (int i = 0; i < THICKNESS; i++)
    {
        image += ((IMG_X_RES - (HEIGHT + 2 * THICKNESS)) / 2);

        for (int j = 0; j < HEIGHT + 2 * THICKNESS; j++)
        {
            *(image++) = FRAME_COLOR; // color
        }

        image += ((IMG_X_RES - (HEIGHT + 2 * THICKNESS)) / 2);
    }

    // top + bottom lines
    image = ((uint16_t *)raw) + ((IMG_Y_RES - (WIDTH + 2 * THICKNESS)) / 2) * IMG_X_RES;

    for (int i = 0; i < WIDTH + 2 * THICKNESS; i++)
    {
        image += ((IMG_X_RES - (HEIGHT + 2 * THICKNESS)) / 2);

        for (int j = 0; j < THICKNESS; j++)
        {
            *(image++) = FRAME_COLOR; // color
        }

        image += HEIGHT;

        for (int j = 0; j < THICKNESS; j++)
        {
            *(image++) = FRAME_COLOR; // color
        }

        image += ((IMG_X_RES - (HEIGHT + 2 * THICKNESS)) / 2);
    }
}

/* Loads image, enables CNN Accelerator and tget the results */
static void run_cnn(int x_offset, int y_offset)
{
    uint32_t imgLen;
    uint32_t w, h;
    static uint32_t noface_count = 0;
    uint8_t *raw;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);

    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CNN); // Enable CNN clock

    printf("Starting CNN...\n");
    cnn_start();

    printf("CNN Started! Loading data...\n");
    load_data(raw, x_offset, y_offset);

    printf("Data Loaded! Waiting for CNN to process...\n");
    // CNN interrupt wakes up CPU from sleep mode
    while (cnn_time == 0)
    {
        asm volatile("wfi"); // Sleep and wait for CNN interrupt
    }
    printf("Result ready!\n");

    cnn_unload((uint32_t *)(raw));

    cnn_stop();

    // Disable CNN clock to save power
    MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_CNN);

    int pResult = calculate_minDistance((uint8_t *)(raw));

    printf("Result = %d \n", pResult);

    if (pResult == 0)
    {
        uint8_t *counter;
        uint8_t counter_len;
        get_min_dist_counter(&counter, &counter_len);

        name = "";
        prev_decision = decision;
        decision = -5;

        printf("counter_len: %d,  %d,%d,%d\n", counter_len, counter[0], counter[1], counter[2]);

        for (uint8_t id = 0; id < counter_len; ++id)
        {
            if (counter[id] >= (uint8_t)(closest_sub_buffer_size * 0.8))
            { // >80%  detection
                name = get_subject(id);
                decision = id;
                noface_count = 0;
                printf("Status: %s \n", name);
                printf("Detection: %s: %d\n", name, counter[id]);
                break;
            }
            else if (counter[id] >= (uint8_t)(closest_sub_buffer_size * 0.4))
            { // >%40 adjust
                name = "Adjust Face";
                decision = -2;
                noface_count = 0;
                printf("Status: %s \n", name);
                printf("Detection: %s: %d\n", name, counter[id]);
                break;
            }
            else if (counter[id] > closest_sub_buffer_size * 0.2)
            { //>>20% unknown
                name = "Unknown";
                decision = -1;
                noface_count = 0;
                printf("Status: %s \n", name);
                printf("Detection: %s: %d\n", name, counter[id]);
                break;
            }
            else if (counter[id] > closest_sub_buffer_size * 0.1)
            { //>> 10% transition
                name = "";
                decision = -3;
                noface_count = 0;
                printf("Status: %s \n", name);
                printf("Detection: %s: %d\n", name, counter[id]);
            }
            else
            {
                noface_count++;

                if (noface_count > 10)
                {
                    name = "No face";
                    decision = -4;
                    noface_count--;
                    printf("Detection: %s: %d\n", name, counter[id]);
                }
            }
        }

        printf("Decision: %d Name:%s \n", decision, name);
    }
}

/* Function to load the image */
static void load_data(uint8_t *raw, int x_offset, int y_offset)
{
    uint8_t *data;
    for (int i = y_offset; i < HEIGHT + y_offset; i++)
    {
        data = raw + ((IMG_Y_RES - (WIDTH)) / 2) * IMG_X_RES * BYTE_PER_PIXEL;
        data += (((IMG_X_RES - (HEIGHT)) / 2) + i) * BYTE_PER_PIXEL;

        for (int j = x_offset; j < WIDTH + x_offset; j++)
        {
            uint8_t ur, ug, ub;
            int8_t r, g, b;
            uint32_t number;

            ub = (uint8_t)(data[j * BYTE_PER_PIXEL * IMG_X_RES + 1] << 3);
            ug = (uint8_t)((data[j * BYTE_PER_PIXEL * IMG_X_RES] << 5) |
                           ((data[j * BYTE_PER_PIXEL * IMG_X_RES + 1] & 0xE0) >> 3));
            ur = (uint8_t)(data[j * BYTE_PER_PIXEL * IMG_X_RES] & 0xF8);

            b = ub - 128;
            g = ug - 128;
            r = ur - 128;

#ifndef FAST_FIFO
            // Loading data into the CNN fifo
            while (((*((volatile uint32_t *)0x50000004) & 1)) != 0)
                ; // Wait for FIFO 0

            number = 0x00FFFFFF & ((((uint8_t)b) << 16) | (((uint8_t)g) << 8) | ((uint8_t)r));
            *((volatile uint32_t *)0x50000008) = number; // Write FIFO 0
#else

            // Loading data into the CNN fifo
            while (((*((volatile uint32_t *)0x400c0404) & 2)) != 0)
                ; // Wait for FIFO 0

            number = 0x00FFFFFF & ((((uint8_t)b) << 16) | (((uint8_t)g) << 8) | ((uint8_t)r));
            *((volatile uint32_t *)0x400c0410) = number; // Write FIFO 0
#endif
        }
    }
}

/* Calls all the necessary functions to initialized CNN */
static void initialize_cnn(void)
{
    // Enable peripheral, enable CNN interrupt, turn on CNN clock
    // CNN clock: 50 MHz div 1
    cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);
    cnn_boost_enable(MXC_GPIO2, MXC_GPIO_PIN_5); // Configure P2.5, turn on the CNN Boost
    cnn_init();                                  // Bring CNN state machine into consistent state
    cnn_load_weights();                          // Load CNN kernels
    cnn_load_bias();                             // Load CNN bias
    cnn_configure();                             // Configure CNN state machine

    __enable_irq();
    /* Enable CNN Interrupt */
    NVIC_EnableIRQ(CNN_IRQn);
    /* Enable CNN wakeup event */
    NVIC_EnableEVENT(CNN_IRQn);

    if (init_database() < 0)
    {
        printf("Could not initialize the database");
        return;
    }
}