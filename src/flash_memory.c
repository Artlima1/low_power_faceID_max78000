#include <stdio.h>
#include "flash_memory.h"
#include "icc.h"
#include "flc.h"
#include "flc_regs.h"
#include "gcr_regs.h"

/***** Definitions *****/
#define APP_PAGE_CNT 40                                                         ///< Flash memory blocks reserved for the app code
#define APP_SIZE     (MXC_FLASH_PAGE_SIZE * APP_PAGE_CNT)                       ///< The app code flash memory area size
#define TOTAL_FLASH_PAGES (MXC_FLASH_MEM_SIZE / MXC_FLASH_PAGE_SIZE)            ///< Flash memory blocks reserved for internal storage
#define FLASH_STORAGE_START_PAGE APP_PAGE_CNT                                   ///< Internal storage first flash memory block
#define FLASH_STORAGE_START_ADDR MXC_FLASH_PAGE_ADDR(FLASH_STORAGE_START_PAGE)  ///< Internal storage start address

uint8_t flash_storage_page_cont;
uint64_t flash_storage_size;

static int flash_write_bytes(uint32_t startaddr, uint32_t size, uint8_t* data, uint8_t verify);

uint8_t flash_memory_init(uint32_t size){
    flash_storage_page_cont = (size % MXC_FLASH_PAGE_SIZE)==0 ? (size / MXC_FLASH_PAGE_SIZE) : (size / MXC_FLASH_PAGE_SIZE)+1;
    flash_storage_size = flash_storage_page_cont * MXC_FLASH_PAGE_SIZE;

    printf("Allocated flash from %08x to %08x\n", FLASH_STORAGE_START_ADDR, FLASH_STORAGE_START_ADDR+flash_storage_size);

    if(flash_storage_page_cont + APP_PAGE_CNT > TOTAL_FLASH_PAGES){
        flash_storage_page_cont = 0;
        flash_storage_size = 0;
        return FM_RET_ERROR;
    }

    MXC_FLC_Init();

    int i;
    for(i=0; i<flash_storage_page_cont; i++){
        int addr = MXC_FLASH_PAGE_ADDR((FLASH_STORAGE_START_PAGE+i));
        MXC_FLC_PageErase(addr);
    }

    return FM_RET_SUCCESS;
}

uint8_t flash_memory_write(uint8_t * data, uint32_t size, uint32_t offset){

    if(offset + size > flash_storage_size){
        return FM_RET_ERROR;
    }

    if(flash_write_bytes(FLASH_STORAGE_START_ADDR+offset, size, data, 1)!=E_NO_ERROR){
        return FM_RET_ERROR;
    }

    return FM_RET_SUCCESS;
}

uint8_t flash_memory_read(uint8_t * dest, uint32_t size, uint32_t offset){

    if(offset + size > flash_storage_size){
        return FM_RET_ERROR;
    }

    MXC_FLC_Read(FLASH_STORAGE_START_ADDR+offset, (void*) dest, size);

    return FM_RET_SUCCESS;
}

static int flash_write_bytes(uint32_t startaddr, uint32_t size, uint8_t* data, uint8_t verify){
    uint32_t i;
    uint8_t verify_byte;

    MXC_ICC_Disable(MXC_ICC0);

    for (i=0; i<size; i+=8) {
        // Write a byte
        int error_status = MXC_FLC_Write(startaddr+i, 8, (uint32_t*) &data[i]);
        if (error_status != E_NO_ERROR) {
            printf("Failure in writing a byte : error %i addr: 0x%08x\n", error_status, startaddr+i);
            return error_status;
        }

        if (verify) {
            // Verify that word is written properly
            MXC_FLC_Read(startaddr+i, &verify_byte, 1);
            if (verify_byte != data[i]) {
                printf("Word is not written properly.\n");
                return E_UNKNOWN;
            }
        }
    }

    MXC_ICC_Enable(MXC_ICC0);

    return E_NO_ERROR;
}