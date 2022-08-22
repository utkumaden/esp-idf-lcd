Generic LCD driver as a ESP32 Component
=======================================

This is a generic LCD driver wrapped as a ESP32 component. You are free to use
the driver component as you wish as long as you follow the license agreement in
[LICENSE.md](LICENSE.md).

Usage Example
-------------

```c

int lcdBusIO(lcdDriver_t *driver, bool rw, bool rs, bool en, uint8_t data)
{
    // Implement for your own setup.
}

int lcdDelay(lcdDriver_t *driver, uint32_t delay)
{
    // Implement for your own setup.
    return usleep((useconds_t)delay);
}

void app_main(void)
{
    lcdDriver_t lcd = {
        .userData = NULL,
        .dimensions = {16, 2},
        .writeOnly = true,
        .fourBits = true
    };

    // Do GPIO initialization.

    // Reset GPIO pins to their default state.
    lcdBusIO(&lcd, false, false, false, 0xFF);

    lcdLoadDefaultTiming(&lcd);     // Load default LCD timings.
    lcdInit(&lcd);                  // Initialize the LCD and driver.

    lcdDirection(&lcd, true);                   // Set the direction to forward.
    lcdSetDisplay(&lcd, true, false, false);    // Enable display.
    lcdHome(&lcd);                              // Home the LCD cursor.

    lcdPutZString(&driver, "Hello World!");
}
```

TO-DO
-----
* Implement read-write mode.
* Implement 8-bit mode.