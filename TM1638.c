/**
 * @file TM1638.c
 * @brief Source file for the TM1638 LED controller and keypad driver.
 *
 * This file provides the implementation for driving TM1638-based modules,
 * which typically include 8 seven-segment displays, 8 dual-color LEDs, and a keypad.
 * This driver is designed to work with STM32 HAL libraries.
 *
 * @version 1.1
 * @date 2025-10-05
 */
#include "TM1638.h"
#include <string.h>
#include <math.h> // Used in tm1638_Draw, though bit-shifting is preferred.

// --- Private Constant Definitions ---

// TM1638 Command Constants
// See TM1638 datasheet for command definitions.

/** @brief Command to set data write mode.
 * - 0x40: Write data to display register, auto-increment address.
 * - 0x44: Write data to display register, fixed address.
 */
static const uint8_t CMD_DATA_SET_AUTO_INC = 0x40;

/** @brief Command to read key scan data. */
static const uint8_t CMD_DATA_READ = 0x42;

/** @brief Command to set the display address.
 *  The address is OR'd with this value (0xC0 to 0xCF).
 */
static const uint8_t CMD_ADDRESS_SET = 0xC0;

/** @brief Command to control the display.
 * - 0x80: Display OFF.
 * - 0x88 to 0x8F: Display ON with brightness level (0-7).
 */
static const uint8_t CMD_DISPLAY_CTRL = 0x80;
static const uint8_t DISPLAY_ON_MASK = 0x08;
static const uint8_t DISPLAY_BRIGHTNESS_MASK = 0x07;


// --- Private Function Prototypes ---

// Low-level GPIO pin control functions
static void tm1638_sdo_high(TM1638 *tm);
static void tm1638_sdo_low(TM1638 *tm);
static void tm1638_stb_high(TM1638 *tm);
static void tm1638_stb_low(TM1638 *tm);
static void tm1638_clk_high(TM1638 *tm);
static void tm1638_clk_low(TM1638 *tm);

// Communication protocol functions
static void tm1638_start_transmission(TM1638 *tm);
static void tm1638_end_transmission(TM1638 *tm);
static void tm1638_send_data(TM1638 *tm, uint8_t data);
static void tm1638_send_command(TM1638 *tm, uint8_t cmd);

// Helper function to get 7-segment font code
static uint8_t char_to_segment_code(char c);


// --- GPIO Control Implementation ---

static void tm1638_sdo_high(TM1638 *tm) {
    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_SET);
}
static void tm1638_sdo_low(TM1638 *tm) {
    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_RESET);
}
static void tm1638_stb_high(TM1638 *tm) {
    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_SET);
}
static void tm1638_stb_low(TM1638 *tm) {
    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_RESET);
}
static void tm1638_clk_high(TM1638 *tm) {
    HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_SET);
}
static void tm1638_clk_low(TM1638 *tm) {
    HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_RESET);
}


// --- Communication Protocol Implementation ---

/**
 * @brief Initiates a data transmission sequence by pulling STB low.
 * @param tm Pointer to the TM1638 handle.
 */
static void tm1638_start_transmission(TM1638 *tm) {
    tm1638_stb_low(tm);
}

/**
 * @brief Terminates a data transmission sequence by pulling STB high.
 * @param tm Pointer to the TM1638 handle.
 */
static void tm1638_end_transmission(TM1638 *tm) {
    tm1638_stb_high(tm);
}

/**
 * @brief Sends a single command byte to the TM1638.
 * @param tm Pointer to the TM1638 handle.
 * @param cmd The command byte to send.
 */
static void tm1638_send_command(TM1638 *tm, uint8_t cmd) {
    tm1638_start_transmission(tm);
    tm1638_send_data(tm, cmd);
    tm1638_end_transmission(tm);
}

/**
 * @brief Sends a byte of data to the TM1638, LSB first.
 * @param tm Pointer to the TM1638 handle.
 * @param data The byte of data to send.
 */
static void tm1638_send_data(TM1638 *tm, uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        tm1638_clk_low(tm);
        // Set data pin high or low based on the current bit (LSB first)
        if (data & 0x01) {
            tm1638_sdo_high(tm);
        } else {
            tm1638_sdo_low(tm);
        }
        // Right-shift data to process the next bit in the next iteration
        data >>= 1;
        tm1638_clk_high(tm);
    }
}


// --- Public Function Implementation ---

/**
 * @brief Initializes the TM1638 module, clears the display, and sets brightness.
 * @param tm Pointer to the TM1638 handle.
 * @param brightness Initial brightness level (0-7).
 */
void tm1638_init(TM1638 *tm, uint8_t brightness) {
    tm->brightness = brightness & DISPLAY_BRIGHTNESS_MASK; // Ensure brightness is within 0-7
    tm1638_display_clear(tm);
    tm1638_set_brightness(tm, tm->brightness);
}

/**
 * @brief Sets the display brightness.
 * @param tm Pointer to the TM1638 handle.
 * @param brightness The brightness level, from 0 (dimmest) to 7 (brightest).
 */
void tm1638_set_brightness(TM1638 *tm, uint8_t brightness) {
    if (brightness > 7) {
        // Clamp brightness to the maximum value if out of range
        brightness = 7;
    }
    tm->brightness = brightness;
    // Command is 0x88-0x8F for display on with brightness
    uint8_t command = CMD_DISPLAY_CTRL | DISPLAY_ON_MASK | brightness;
    tm1638_send_command(tm, command);
}

/**
 * @brief Clears all 8 segment displays and turns off all LEDs.
 * @param tm Pointer to the TM1638 handle.
 */
void tm1638_display_clear(TM1638 *tm) {
    // Set to auto-increment address mode
    tm1638_send_command(tm, CMD_DATA_SET_AUTO_INC);

    tm1638_start_transmission(tm);
    // Start writing from address 0x00
    tm1638_send_data(tm, CMD_ADDRESS_SET);
    // Write 0x00 to all 16 registers (8 for segments, 8 for LEDs)
    for (uint8_t i = 0; i < 16; i++) {
        tm1638_send_data(tm, 0x00);
    }
    tm1638_end_transmission(tm);
}

/**
 * @brief Displays a single character on a specified segment.
 * @param tm Pointer to the TM1638 handle.
 * @param position The display position (1-8).
 * @param c The character to display.
 * @param dot If true, the decimal point for that segment is turned on.
 */
void tm1638_display_char(TM1638 *tm, uint8_t position, char c, bool dot) {
    if (position < 1 || position > 8) {
        return; // Invalid position
    }
    uint8_t hex_code = char_to_segment_code(c);
    if (dot) {
        hex_code |= 0x80; // The MSB controls the decimal point.
    }
    tm1638_set_segment(tm, position, hex_code);
}

/**
 * @brief Displays a string on the 7-segment displays.
 *
 * The string is right-aligned. Handles decimal points.
 * E.g., "12.34" will be displayed as "  12.34".
 * If the string is too long, it will be truncated from the left.
 *
 * @param tm Pointer to the TM1638 handle.
 * @param str The null-terminated string to display.
 */
void tm1638_display_txt(TM1638 *tm, const char *str) {
    char display_buf[9] = "        "; // 8 chars + null terminator
    bool dots[8] = {false};
    int8_t len = strlen(str);
    int8_t str_idx = len - 1;
    int8_t buf_idx = 7;                // Buffer index for display

    // Parse the string backwards to handle right-alignment and dots easily
    while (str_idx >= 0 && buf_idx >= 0) {
        if (str[str_idx] == '.') {
            if (buf_idx < 7) { // Dot applies to the character to its left
                dots[buf_idx + 1] = true;
            }
        } else {
            display_buf[buf_idx] = str[str_idx];
            buf_idx--;
        }
        str_idx--;
    }

    // Display the parsed buffer
    for (uint8_t i = 0; i < 8; i++) {
        tm1638_display_char(tm, i + 1, display_buf[i], dots[i]);
    }
}


/**
 * @brief Controls a single LED.
 * @param tm Pointer to the TM1638 handle.
 * @param position The LED position (1-8).
 * @param on True to turn the LED on, false to turn it off.
 */
void tm1638_set_led(TM1638 *tm, uint8_t position, bool on) {
    if (position < 1 || position > 8) {
        return; // Invalid position
    }
    // LED addresses are the odd-numbered registers (1, 3, 5, ...)
    uint8_t address = CMD_ADDRESS_SET + (2 * position) - 1;

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, address);
    tm1638_send_data(tm, on ? 0x01 : 0x00);
    tm1638_end_transmission(tm);
}

/**
 * @brief Sets the raw 8-bit value for a single 7-segment display.
 * @param tm Pointer to the TM1638 handle.
 * @param position The display position (1-8).
 * @param data The 8-bit segment data (bit 7 is DP, bits 6-0 are G-A).
 */
void tm1638_set_segment(TM1638 *tm, uint8_t position, uint8_t data) {
    if (position < 1 || position > 8) {
        return; // Invalid position
    }
    // Segment addresses are the even-numbered registers (0, 2, 4, ...)
    uint8_t address = CMD_ADDRESS_SET + (2 * (position - 1));

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, address);
    tm1638_send_data(tm, data);
    tm1638_end_transmission(tm);
}


/**
 * @brief Scans the keypad and returns a bitmask of pressed buttons.
 * @param tm Pointer to the TM1638 handle.
 * @return A bitmask where bit 0 corresponds to S1, bit 1 to S2, etc.
 */
uint8_t tm1638_scan_buttons(TM1638 *tm) {
    uint32_t raw_key_data = 0;
    uint8_t pressed_keys = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    tm1638_start_transmission(tm);
    tm1638_send_data(tm, CMD_DATA_READ);

    // Temporarily set DIO pin as input to read data from TM1638
    GPIO_InitStruct.Pin = tm->dio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP; // Use pull-up to ensure stable line
    HAL_GPIO_Init(tm->dio_port, &GPIO_InitStruct);

    // Read the 4 bytes (32 bits) of key scan data
    for (int8_t i = 0; i < 32; i++) {
        tm1638_clk_low(tm);
        // HAL_Delay(1); // Small delay may be needed on very fast MCUs
        if (HAL_GPIO_ReadPin(tm->dio_port, tm->dio_pin) == GPIO_PIN_SET) {
            raw_key_data |= (1UL << i);
        }
        tm1638_clk_high(tm);
    }
    tm1638_end_transmission(tm);

    // Restore DIO pin to output push-pull mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(tm->dio_port, &GPIO_InitStruct);

    /*
     * The TM1638 returns key data in a specific pattern across the 4 bytes.
     * K1 keys (S1, S5) are in the 1st and 3rd bits of each byte.
     * K2 keys (S2, S6) are in the 2nd and 4th bits.
     * And so on.
     * Bit 1 -> S1, Bit 5 -> S5
     * Bit 9 -> S2, Bit 13 -> S6
     * Bit 17 -> S3, Bit 21 -> S7
     * Bit 25 -> S4, Bit 29 -> S8
     */
    if (raw_key_data & (1UL << 1)) pressed_keys |= (1 << 0);  // S1
    if (raw_key_data & (1UL << 9)) pressed_keys |= (1 << 1);  // S2
    if (raw_key_data & (1UL << 17)) pressed_keys |= (1 << 2); // S3
    if (raw_key_data & (1UL << 25)) pressed_keys |= (1 << 3); // S4
    if (raw_key_data & (1UL << 5)) pressed_keys |= (1 << 4);  // S5
    if (raw_key_data & (1UL << 13)) pressed_keys |= (1 << 5); // S6
    if (raw_key_data & (1UL << 21)) pressed_keys |= (1 << 6); // S7
    if (raw_key_data & (1UL << 29)) pressed_keys |= (1 << 7); // S8

    return pressed_keys;
}

/**
 * @brief Waits until a key is pressed and returns its number (1-8).
 * @param tm Pointer to the TM1638 handle.
 * @return The key number (1-8). Returns 0 if multiple keys are pressed.
 */
uint8_t tm1638_read_key_blocking(TM1638 *tm) {
    uint8_t buttons = 0;
    // Wait for a key press
    while ((buttons = tm1638_scan_buttons(tm)) == 0) {
        HAL_Delay(20); // Polling delay to reduce CPU usage
    }

    // Debounce: wait for key release
    while (tm1638_scan_buttons(tm) != 0) {
        HAL_Delay(20);
    }
    
    // Convert bitmask to key number (1-8)
    for (uint8_t i = 0; i < 8; i++) {
        if ((buttons >> i) & 1) {
            return i + 1;
        }
    }
    return 0; // Should not be reached if one key was pressed
}


// --- Private Helper Function Implementation ---

/**
 * @brief Converts a character to its 7-segment display hexadecimal code.
 *
 * This function maps ASCII characters to the common anode 7-segment display codes.
 * Segments are mapped as: bit 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G.
 *
 * @param c The character to convert.
 * @return The 8-bit segment code. Returns 0x00 (blank) for unsupported characters.
 */
static uint8_t char_to_segment_code(char c) {
    switch (c) {
        // Digits
        case '0': return 0x3f;
        case '1': return 0x06;
        case '2': return 0x5b;
        case '3': return 0x4f;
        case '4': return 0x66;
        case '5': return 0x6d;
        case '6': return 0x7d;
        case '7': return 0x07;
        case '8': return 0x7f;
        case '9': return 0x6f;
        // Letters (uppercase)
        case 'A': return 0x77;
        case 'B': return 0x7f; // Same as '8'
        case 'C': return 0x39;
        case 'D': return 0x3f;
        case 'E': return 0x79;
        case 'F': return 0x71;
        case 'G': return 0x7d;
        case 'H': return 0x76;
        case 'I': return 0x06; // Same as '1'
        case 'J': return 0x0e;
        case 'L': return 0x38;
        case 'O': return 0x3f; // Same as '0'
        case 'P': return 0x73;
        case 'S': return 0x6d; // Same as '5'
        case 'U': return 0x3e;
        // Letters (lowercase)
        case 'a': return 0x5f;
        case 'b': return 0x7c;
        case 'c': return 0x58;
        case 'd': return 0x5e;
        case 'f': return 0x71;
        case 'g': return 0x6f;
        case 'h': return 0x74;
        case 'i': return 0x04;
        case 'n': return 0x54;
        case 'o': return 0x5c;
        case 'r': return 0x50;
        case 't': return 0x78;
        case 'u': return 0x1c;
        case 'y': return 0x6e;
        // Symbols
        case ' ': return 0x00;
        case '_': return 0x08;
        case '-': return 0x40;
        default:  return 0x00; // Blank for unsupported characters
    }
}