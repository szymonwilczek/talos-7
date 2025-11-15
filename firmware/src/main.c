#include "pico/stdlib.h"
#include "tusb.h"
#include "macro_config.h"
#include "cdc_protocol.h"
#include "mock_hardware.h"
#include "pin_definitions.h"
#include <stdio.h>

// ==================== BOOT MESSAGE ====================
static void print_boot_message(void) {
    printf("\n");
    printf("========================================\n");
    printf("  MACRO KEYBOARD FIRMWARE v1.0\n");
    printf("========================================\n");
    printf("Device: Raspberry Pi Pico (RP2040)\n");
    printf("USB VID:PID = 0x%04X:0x%04X\n", USB_VID, USB_PID);
    printf("Buttons: %d (GP%d-GP%d)\n", NUM_BUTTONS, BTN_PIN_1, BTN_PIN_7);
    printf("Layers: %d\n", MAX_LAYERS);
    printf("========================================\n");
    printf("\n");
}

// ==================== GŁÓWNA FUNKCJA ====================
int main(void) {
    // SDK
    stdio_init_all();
    
    // stabilizacja USB
    sleep_ms(500);
    
    // boot message
    print_boot_message();
    
    // USB
    printf("[MAIN] Initializing USB stack...\n");
    tusb_init();
    
    // konfiguracja (flash)
    printf("[MAIN] Loading configuration...\n");
    config_init();
    
    // protokol CDC
    printf("[MAIN] Initializing CDC protocol...\n");
    cdc_protocol_init();
    
    // mock hardware - NA RAZIE TYLKO SYMULACJA
    printf("[MAIN] Initializing mock hardware...\n");
    mock_hardware_init();
    
    printf("[MAIN] System ready! Waiting for USB connection...\n");
    printf("[MAIN] Connect via Serial (115200 baud) and send 'GET_CONF' to test.\n");
    printf("\n");
    
    // periodic updates
    uint32_t last_update_time = 0;
    
    // ==================== GŁÓWNA PĘTLA ====================
    while (1) {
        // USB stack task
        tud_task();
        
        // CDC protocol task (incoming commands)
        cdc_protocol_task();
        
        // mock hardware periodic update
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_update_time >= 1000) {
            mock_hardware_update();
            last_update_time = current_time;
        }
        
        sleep_ms(1);
    }
    
    return 0;
}

// ==================== USB CDC CALLBACKS ====================

// invoked when CDC interface received data
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
    // data handled in cdc_protocol_task()
}

// invoked when line state changes (DTR, RTS)
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void) itf;
    (void) rts;
    
    if (dtr) {
        printf("[USB] CDC connected\n");
    } else {
        printf("[USB] CDC disconnected\n");
    }
}
