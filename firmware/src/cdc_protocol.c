#include "cdc_protocol.h"
#include "macro_config.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// ==================== PRYWATNE ZMIENNE ====================
static char cmd_buffer[CDC_MAX_COMMAND_LEN];
static uint16_t cmd_buffer_pos = 0;

// ==================== WYSYŁANIE ODPOWIEDZI ====================
void cdc_send_response(const char* response) {
    if (tud_cdc_connected()) {
        tud_cdc_write_str(response);
        tud_cdc_write_str("\r\n");
        tud_cdc_write_flush();
        
        uint32_t timeout = 100; // 100ms timeout
        uint32_t start = time_us_32();
        while (tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE && 
               (time_us_32() - start) < (timeout * 1000)) {
            tud_task(); 
        }
    }
}

void cdc_send_response_fmt(const char* format, ...) {
    if (!tud_cdc_connected()) return;
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    cdc_send_response(buffer);
}

// ==================== PARSOWANIE KOMEND ====================
static void process_command(const char* cmd_input) {
    printf("[CDC] Command received: %s\n", cmd_input);

    // lokalna kopia do modyfikacji 
    char cmd[CDC_MAX_COMMAND_LEN];
    strncpy(cmd, cmd_input, CDC_MAX_COMMAND_LEN - 1);
    cmd[CDC_MAX_COMMAND_LEN - 1] = '\0';
    
    // usuniecie whitespace z poczatku  
    char* cmd_ptr = cmd;
    while (*cmd_ptr == ' ' || *cmd_ptr == '\t' || *cmd_ptr == '\r' || *cmd_ptr == '\n') {
        cmd_ptr++;
    }
    
    // usuniecie whitespace z konca 
    size_t len = strlen(cmd_ptr);
    while (len > 0 && (cmd_ptr[len-1] == ' ' || cmd_ptr[len-1] == '\t' || 
                       cmd_ptr[len-1] == '\r' || cmd_ptr[len-1] == '\n')) {
        cmd_ptr[len-1] = '\0';
        len--;
    }
    
    // debug log: TODO usunac pozniej
    printf("[CDC] Processed command: '%s' (len=%zu)\n", cmd_ptr, len);
    for (size_t i = 0; i < len && i < 20; i++) {
        printf("[CDC]   byte[%zu] = 0x%02x ('%c')\n", i, (unsigned char)cmd_ptr[i], 
               (cmd_ptr[i] >= 32 && cmd_ptr[i] < 127) ? cmd_ptr[i] : '?');
    }
    
    if (len == 0) {
        printf("[CDC] Empty command, ignoring\n");
        return;
    }
    
    config_data_t* config = config_get();
    
    // GET_CONF
    if (strcmp(cmd_ptr, "GET_CONF") == 0) {
        printf("[CDC] GET_CONF command received\n");
        
        cdc_send_response("CONF_START");
        printf("[CDC] Sent CONF_START\n");
        tud_cdc_write_flush();
        
        // nazwy warstw
        for (int layer = 0; layer < MAX_LAYERS; layer++) {
            cdc_send_response_fmt("LAYER_NAME|%d|%s", layer, config->layer_names[layer]);
            tud_cdc_write_flush();
        }
        printf("[CDC] Sent %d layer names\n", MAX_LAYERS);
        
        // wszystkie makra
        int macro_count = 0;
        for (int layer = 0; layer < MAX_LAYERS; layer++) {
            for (int btn = 0; btn < NUM_BUTTONS; btn++) {
                macro_entry_t* macro = &config->macros[layer][btn];
                cdc_send_response_fmt("MACRO|%d|%d|%d|%d|%s|%s",
                    layer,
                    btn,
                    macro->type,
                    macro->value,
                    macro->macro_string,
                    macro->name
                );
                tud_cdc_write_flush();
                macro_count++;
            }
        }
        printf("[CDC] Sent %d macros\n", macro_count);
        
        printf("[CDC] About to send CONF_END...\n");
        cdc_send_response("CONF_END");
        printf("[CDC] Sent CONF_END\n");
        
        tud_cdc_write_flush();
        sleep_ms(10);
        tud_cdc_write_flush();
        printf("[CDC] Configuration sent complete\n");
        return;
    }
    
    // SET_MACRO|layer|button|type|value|string|name
    if (strncmp(cmd_ptr, "SET_MACRO|", 10) == 0) {
        int layer, button, type, value;
        char macro_string[MACRO_STRING_LEN] = {0};
        char name[MAX_NAME_LEN] = {0};
        
        char* token = cmd_ptr + 10; // pomijanie SET_MACRO
        
        layer = atoi(token);
        token = strchr(token, '|');
        if (!token) { cdc_send_response("ERROR|Invalid format"); return; }
        token++;
        
        button = atoi(token);
        token = strchr(token, '|');
        if (!token) { cdc_send_response("ERROR|Invalid format"); return; }
        token++;
        
        type = atoi(token);
        token = strchr(token, '|');
        if (!token) { cdc_send_response("ERROR|Invalid format"); return; }
        token++;
        
        value = atoi(token);
        token = strchr(token, '|');
        if (!token) { cdc_send_response("ERROR|Invalid format"); return; }
        token++;
        
        // macro_string (do nastepnego |)
        char* next_pipe = strchr(token, '|');
        if (next_pipe) {
            size_t str_len = next_pipe - token;
            if (str_len >= MACRO_STRING_LEN) str_len = MACRO_STRING_LEN - 1;
            strncpy(macro_string, token, str_len);
            macro_string[str_len] = '\0';
            token = next_pipe + 1;
        } else {
            cdc_send_response("ERROR|Invalid format");
            return;
        }
        
        // name (reszta do konca linii)
        strncpy(name, token, MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';
        
        // walidacja
        if (layer >= 0 && layer < MAX_LAYERS && 
            button >= 0 && button < NUM_BUTTONS) {
            
            macro_entry_t* macro = &config->macros[layer][button];
            macro->type = type;
            macro->value = value;
            strncpy(macro->macro_string, macro_string, MACRO_STRING_LEN - 1);
            strncpy(macro->name, name, MAX_NAME_LEN - 1);
            
            cdc_send_response("OK");
            printf("[CDC] Macro set: L%d B%d Type=%d Value=%d String='%s' Name='%s'\n", 
                   layer, button, type, value, macro_string, name);
        } else {
            cdc_send_response("ERROR|Invalid parameters");
        }
        return;
    }
    
    // SET_LAYER_NAME|layer|name
    if (strncmp(cmd_ptr, "SET_LAYER_NAME|", 15) == 0) {
        int layer;
        char name[MAX_NAME_LEN] = {0};
        
        if (sscanf(cmd_ptr, "SET_LAYER_NAME|%d|%15[^\n]", &layer, name) == 2 &&
            layer >= 0 && layer < MAX_LAYERS) {
            
            strncpy(config->layer_names[layer], name, MAX_NAME_LEN - 1);
            cdc_send_response("OK");
            printf("[CDC] Layer name set: L%d = %s\n", layer, name);
        } else {
            cdc_send_response("ERROR|Invalid parameters");
        }
        return;
    }
    
    // SAVE_FLASH
    if (strcmp(cmd_ptr, "SAVE_FLASH") == 0) {
        if (config_save()) {
            cdc_send_response("OK");
            printf("[CDC] Configuration saved to flash\n");
        } else {
            cdc_send_response("ERROR|Flash write failed");
        }
        return;
    }
    
    // Unknown command
    printf("[CDC] Unknown command: '%s'\n", cmd_ptr);
    cdc_send_response("ERROR|Unknown command");
}
// ==================== INICJALIZACJA ====================
void cdc_protocol_init(void) {
    cmd_buffer_pos = 0;
    memset(cmd_buffer, 0, sizeof(cmd_buffer));
    printf("[CDC] Protocol initialized\n");
}

// ==================== GŁÓWNA PĘTLA ====================
void cdc_protocol_task(void) {
    if (!tud_cdc_connected()) {
        return;
    }
    
    // available characters
    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();
        
        // line endings
        if (c == '\n' || c == '\r') {
            if (cmd_buffer_pos > 0) {
                cmd_buffer[cmd_buffer_pos] = '\0';
                process_command(cmd_buffer);
                cmd_buffer_pos = 0;
                memset(cmd_buffer, 0, sizeof(cmd_buffer));
            }
        }
        // buffer character
        else if (cmd_buffer_pos < CDC_MAX_COMMAND_LEN - 1) {
            cmd_buffer[cmd_buffer_pos++] = c;
        }
        // buffer overflow
        else {
            cdc_send_response("ERROR|Command too long");
            cmd_buffer_pos = 0;
            memset(cmd_buffer, 0, sizeof(cmd_buffer));
        }
    }
}
