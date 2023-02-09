#include <stdint.h>

/* 
    This structure can be used to store any structure in flash memory, without interfering with
    program memory. Basically, it allocates 8 pages of memory to the program file and can use the
    rest to store data. 
    Use it wisely, only to backup important data.
    Define the structure of the data to be stored in your program. This app only requires it's size.
*/

enum {
    FM_RET_ERROR,
    FM_RET_SUCCESS
};

/**
 * @brief   Alocates the specified size in flash memory to be used
 * @details Define the structure of the data to be stored in your program, 
 * this app only requires it's size. You'll be responsible by it's manipulation
 * 
 * @param[in]  size  The size flash memory that will be used
 *
 */
uint8_t flash_memory_init(uint32_t size);

/**
 * @brief      Write info in the flash space allocated
 * @details    Allows to modify the data on the allocated memory in flash
 * 
 * @param      data   Address to copy from
 * @param[in]  size   Size of the data to be stored
 * @param[in]  offset Offset from the beggining of data structure alocated in flash
 * @return     #FM_RET_SUCCESS If function is successful.
 * @return     #FM_RET_ERROR If there's an error writing data or the params passed
 * make data out of range.
 *
 */
uint8_t flash_memory_write(uint8_t * data, uint32_t size, uint32_t offset);

/**
 * @brief      Read data stored in flash
 * @details    Allows to read data from the allocated memory in flash
 * 
 * @param      data   Address to copy to
 * @param[in]  size   Size of the information to be read
 * @param[in]  offset Offset from the beggining of data structure alocated in flash
 * @return     #FM_RET_SUCCESS If function is successful.
 * @return     #FM_RET_ERROR If there's an error reading data or the params passed
 * make data out of range.
 *
 */
uint8_t flash_memory_read(uint8_t * dest, uint32_t size, uint32_t offset);