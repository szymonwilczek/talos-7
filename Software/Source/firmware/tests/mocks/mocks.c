/*
 * mock definitions - global variables for mocks
 */

#include "mock_flash.h"
#include "mock_tusb.h"

// flash mock storage
uint8_t mock_flash_storage[MOCK_FLASH_SIZE];

// CDC mock buffers
char mock_cdc_output[4096] = {0};
int mock_cdc_output_pos = 0;
char mock_cdc_input[4096] = {0};
int mock_cdc_input_pos = 0;
int mock_cdc_input_len = 0;
