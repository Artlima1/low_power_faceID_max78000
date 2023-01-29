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
 * @brief      Write the full data structure
 * @details    The details about it's size were specified in flash_memory_init().
 * 
 * @param      data   Address to the structre
 * @return     #FM_RET_SUCCESS If function is successful.
 * @return     #FM_RET_ERROR If there's an error writing data.
 *
 */
uint8_t flash_memory_write_full(uint8_t * data);

/**
 * @brief      Update just part of the full structre
 * @details    Allows to modify a specific field of your data without rewriting everything
 * 
 * @param      data   Address to copy from
 * @param[in]  size   Size of the information updated
 * @param[in]  offset Offset from the beggining of data structre
 * @return     #FM_RET_SUCCESS If function is successful.
 * @return     #FM_RET_ERROR If there's an error writing data or the params passed
 * make data out of range.
 *
 */
uint8_t flash_memory_write_field(uint8_t * data, uint32_t size, uint32_t offset);

/**
 * @brief      Read the full data structure
 * @details    The details about it's size were specified in flash_memory_init().
 * 
 * @param      dest   Address to write to
 *
 */
void flash_memory_read_full(uint8_t * dest);

/**
 * @brief      Read just part of the full structre
 * @details    Allows to read a specific field of your data without reading everything
 * 
 * @param      data   Address to copy to
 * @param[in]  size   Size of the information required
 * @param[in]  offset Offset from the beggining of data structre
 * @return     #FM_RET_SUCCESS If function is successful.
 * @return     #FM_RET_ERROR If there's an error reading data or the params passed
 * make data out of range.
 *
 */
uint8_t flash_memory_read_field(uint8_t * dest, uint32_t size, uint32_t offset);