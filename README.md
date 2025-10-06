# TM1638 Library for STM32

A comprehensive C library for controlling TM1638-based display modules using STM32 microcontrollers with HAL drivers.

## 📋 Overview

This library provides a complete interface for the **TM1638** LED controller chip, commonly found in affordable display modules. The TM1638 module typically features:

- **8× Seven-Segment Displays** (arranged as 2 groups of 4 digits)
- **8× Red-Color LEDs** (red)
- **8× Push Buttons** (S1-S8)
- **Simple 3-wire interface** (CLK, DIO, STB)

The library is designed specifically for **STM32** microcontrollers using the **HAL (Hardware Abstraction Layer)** libraries.

## ✨ Features

- ✅ Display text and numbers on 7-segment displays
- ✅ Individual LED control
- ✅ Button scanning (blocking and non-blocking)
- ✅ Configurable brightness (8 levels: 0-7)
- ✅ Decimal point support
- ✅ Custom segment patterns
- ✅ Right-aligned text display
- ✅ Comprehensive character set (digits, letters, symbols)
- ✅ Well-documented and easy to use

## 🔧 Hardware Requirements

- STM32 microcontroller (tested with NUCLEO-F401RE)
- TM1638 display module
- 3 GPIO pins for communication (CLK, DIO, STB)

## 📦 Installation

1. Copy `TM1638.h` and `TM1638.c` to your STM32 project
2. Include the header file in your main code:
```c
#include "TM1638.h"
```
3. Configure 3 GPIO pins as outputs (Push-Pull mode) in STM32CubeMX or manually

## 🚀 Quick Start

### Basic Initialization

```c
#include "TM1638.h"

// Create TM1638 instance
TM1638 display;

// Configure pins
display.clk_port = GPIOA;
display.clk_pin = GPIO_PIN_0;
display.dio_port = GPIOA;
display.dio_pin = GPIO_PIN_1;
display.stb_port = GPIOA;
display.stb_pin = GPIO_PIN_2;

// Initialize with brightness level 5 (0-7)
tm1638_init(&display, 5);
```

### Display Text

```c
// Display a string (right-aligned)
tm1638_display_txt(&display, "HELLO");

// Display a number
tm1638_display_txt(&display, "12345678");

// Display with decimal point
tm1638_display_txt(&display, "12.34");
```

### Display Individual Characters

```c
// Display 'A' on position 1 (leftmost)
tm1638_display_char(&display, 1, 'A', false);

// Display '5' with decimal point on position 4
tm1638_display_char(&display, 4, '5', true);
```

### Control LEDs

```c
// Turn on LED 1
tm1638_set_led(&display, 1, true);

// Turn off LED 3
tm1638_set_led(&display, 3, false);

// Turn on all LEDs
for (uint8_t i = 1; i <= 8; i++) {
    tm1638_set_led(&display, i, true);
}
```

### Read Buttons

```c
// Non-blocking button scan
uint8_t buttons = tm1638_scan_buttons(&display);
if (buttons & (1 << 0)) {
    // Button S1 is pressed
}

// Blocking button read (waits for press and release)
uint8_t key = tm1638_read_key_blocking(&display);
// Returns 1-8 for single key press
```

### Brightness Control

```c
// Set brightness (0 = dimmest, 7 = brightest)
tm1638_set_brightness(&display, 7);
```

### Clear Display

```c
// Clear all displays and turn off all LEDs
tm1638_display_clear(&display);
```

## 📚 API Reference

### Initialization

```c
void tm1638_init(TM1638 *tm, uint8_t brightness);
```
Initializes the TM1638 module. Must be called before any other function.

### Display Functions

```c
void tm1638_display_txt(TM1638 *tm, const char *str);
void tm1638_display_char(TM1638 *tm, uint8_t position, char c, bool dot);
void tm1638_set_segment(TM1638 *tm, uint8_t position, uint8_t data);
void tm1638_display_clear(TM1638 *tm);
```

### LED Control

```c
void tm1638_set_led(TM1638 *tm, uint8_t position, bool on);
```

### Button Input

```c
uint8_t tm1638_scan_buttons(TM1638 *tm);
uint8_t tm1638_read_key_blocking(TM1638 *tm);
```

### Configuration

```c
void tm1638_set_brightness(TM1638 *tm, uint8_t brightness);
```

## 🎨 Supported Characters

### Digits
`0 1 2 3 4 5 6 7 8 9`

### Uppercase Letters
`A B C E F H I J L O P S U`

### Lowercase Letters
`a b c d f g h i n o r t u y`

### Symbols
`space _ - .`

> **Note:** Unsupported characters will be displayed as blank.

## 🔌 Pin Configuration Example (STM32CubeMX)

1. Configure 3 GPIO pins as **GPIO_Output**
2. Set GPIO output level to **High** (default)
3. Set GPIO mode to **Push Pull**
4. Set GPIO pull-up/pull-down to **No pull-up and no pull-down**
5. Set GPIO speed to **Low** or **Medium**

## 📖 Example Projects

### Simple Counter

```c
int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    TM1638 display;
    display.clk_port = GPIOA;
    display.clk_pin = GPIO_PIN_0;
    display.dio_port = GPIOA;
    display.dio_pin = GPIO_PIN_1;
    display.stb_port = GPIOA;
    display.stb_pin = GPIO_PIN_2;
    
    tm1638_init(&display, 5);
    
    uint32_t counter = 0;
    char buffer[16];
    
    while (1) {
        sprintf(buffer, "%lu", counter);
        tm1638_display_txt(&display, buffer);
        counter++;
        HAL_Delay(1000);
    }
}
```

### Button-Controlled LED

```c
while (1) {
    uint8_t buttons = tm1638_scan_buttons(&display);
    
    for (uint8_t i = 0; i < 8; i++) {
        if (buttons & (1 << i)) {
            tm1638_set_led(&display, i + 1, true);
        } else {
            tm1638_set_led(&display, i + 1, false);
        }
    }
    
    HAL_Delay(50);
}
```

## 🐛 Troubleshooting

### Display shows nothing
- Check that GPIO pins are correctly configured as outputs
- Verify power supply to the TM1638 module (usually 5V)
- Ensure proper connections between MCU and module
- Try increasing brightness: `tm1638_set_brightness(&display, 7);`

### Buttons not responding
- Check that DIO pin can be reconfigured as input
- Verify button connections on the module
- Try adding a small delay in the button scanning loop

### Garbled display
- Ensure proper grounding between MCU and module
- Check for loose connections
- Reduce clock speed if communication is unreliable

## 📄 License

This project is open source. Feel free to use, modify, and distribute as needed.

## 👤 Author

**BobbyESP**

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!

## 📝 Version History

- **v1.1** (2025-10-05) - Current version with full feature set
- Complete button scanning implementation
- Comprehensive character support
- Improved documentation

## 🔗 References

- [TM1638 Datasheet](https://futuranet.it/futurashop/image/catalog/data/Download/TM1638_V1.3_EN.pdf)
- STM32 HAL Documentation

> This library is a rewritten and improved version of the one originally provided to me during my time at university. For privacy and licensing reasons, I will not be sharing the original file.

---

⭐ If you find this library useful, please consider giving it a star!
