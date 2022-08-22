#include "lcd.h"

/**
 * Control the LCD bus, write value and read.
 * @param driver The driver controlling the bus.
 * @param rw Read/Write pin.
 * @param rs Register Select pin.
 * @param en Enable pin.
 * @param data Data bus.
 * @return Non-negative on success, bus value if reading, negative on error.
 */
int WEAK lcdBusIO(lcdDriver_t *driver, bool rw, bool rs, bool en, uint8_t data)
{
    assert(driver->busIO);
    return driver->busIO(driver, rw, rs, en, data);
}

/**
 * Delay with microsecond precision.
 * @param driver The driver delaying.
 * @param delay Delay in microseconds.
 * @return Zero for success, non-zero on failure.
 * @remarks
 * Although the implementation should have microsecond precision, over delay
 * is not critical as long as the condition that the driver suspended for
 * delay microseconds is satisfied.
 */
int WEAK lcdDelay(lcdDriver_t *driver, uint32_t delay)
{
    assert(driver->delay);
    return driver->delay(driver, delay);
}

int lcdCommand(lcdDriver_t *driver, uint8_t command)
{
    // Write command into bus.
    if (
        lcdBusIO(driver, 0, 0, 0, command) < 0                  ||
        lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
        lcdBusIO(driver, 0, 0, 1, command) < 0                  ||
        lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
        lcdBusIO(driver, 0, 0, 0, command) < 0                  ||
        lcdDelay(driver, driver->busTiming.dataHold)
    )
    {
        // IO failed.
        driver->error = EIO;
        return -1;
    }

    if (driver->fourBits)
    {
        // Write bottom nibble of command into bus if in 4bit mode.
        command <<= 4;
        if (
            lcdBusIO(driver, 0, 0, 0, command) < 0                  ||
            lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
            lcdBusIO(driver, 0, 0, 1, command) < 0                  ||
            lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
            lcdBusIO(driver, 0, 0, 0, command) < 0                  ||
            lcdDelay(driver, driver->busTiming.dataHold)
        )
        {
            // IO failed.
            driver->error = EIO;
            return -1;
        }
    }

    if (driver->writeOnly)
    {
        // Just do a dumb delay if in write only mode.
        return lcdDelay(driver, driver->busTiming.busyHoldShort);
    }
    else
    {
        // Setup read from busy flag.
        if (
            lcdBusIO(driver, 1, 0, 0, 0) < 0                        ||
            lcdDelay(driver, driver->busTiming.addressSetup) != 0
        )
        {
            // IO failed.
            driver->error = EIO;
            return -1;
        }

        int value = 0;
        do {
            if (
                lcdDelay(driver, driver->busTiming.busyInterval) != 0   ||  // Delay before reading busy pin.
                lcdBusIO(driver, 1, 0, 1, 0) < 0                        ||
                lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
                (value = lcdBusIO(driver, 1, 0, 1, 0)) < 0              ||  // Read busy value.
                lcdBusIO(driver, 1, 0, 0, 0) < 0                        ||
                ((driver->fourBits) && (                                    // 4-bit mode extra ticks.
                    lcdDelay(driver, driver->busTiming.dataHold) != 0   ||
                    lcdBusIO(driver, 1, 0, 1, 0) < 0                    ||
                    lcdDelay(driver, driver->busTiming.enableHold)      ||
                    lcdBusIO(driver, 1, 0, 0, 0) < 0
                ))
            )
            {
                // IO failed.
                driver->error = EIO;
                return -1;
            }
        } while (value & (1 << 7)); // Busy flag is the 7th bit.

        return lcdBusIO(driver, 0, 0, 0, 0);
    }
}

int lcdWrite(lcdDriver_t *driver, uint8_t data)
{
    // Write data into bus.
    if (
        lcdBusIO(driver, 0, 1, 0, data) < 0                     ||
        lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
        lcdBusIO(driver, 0, 1, 1, data) < 0                     ||
        lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
        lcdBusIO(driver, 0, 1, 0, data) < 0                     ||
        lcdDelay(driver, driver->busTiming.dataHold)
    )
    {
        // IO failed.
        driver->error = EIO;
        return -1;
    }

    if (driver->fourBits)
    {
        // Write bottom nibble of data into bus if in 4bit mode.
        data <<= 4;
        if (
            lcdBusIO(driver, 0, 1, 0, data) < 0                  ||
            lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
            lcdBusIO(driver, 0, 1, 1, data) < 0                  ||
            lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
            lcdBusIO(driver, 0, 1, 0, data) < 0                  ||
            lcdDelay(driver, driver->busTiming.dataHold)
        )
        {
            // IO failed.
            driver->error = EIO;
            return -1;
        }
    }

    if (driver->writeOnly)
    {
        // Just do a dumb delay if in write only mode.
        return lcdDelay(driver, driver->busTiming.busyHoldShort);
    }
    else
    {
        // Setup read from busy flag.
        if (
            lcdBusIO(driver, 1, 0, 0, 0) < 0                        ||
            lcdDelay(driver, driver->busTiming.addressSetup) != 0
        )
        {
            // IO failed.
            driver->error = EIO;
            return -1;
        }

        int value = 0;
        do {
            if (
                lcdDelay(driver, driver->busTiming.busyInterval) != 0   ||  // Delay before reading busy pin.
                lcdBusIO(driver, 1, 0, 1, 0) < 0                        ||
                lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
                (value = lcdBusIO(driver, 1, 0, 1, 0)) < 0              ||  // Read busy value.
                lcdBusIO(driver, 1, 0, 0, 0) < 0                        ||
                ((driver->fourBits) && (                                    // 4-bit mode extra ticks.
                    lcdDelay(driver, driver->busTiming.dataHold) != 0   ||
                    lcdBusIO(driver, 1, 0, 1, 0) < 0                    ||
                    lcdDelay(driver, driver->busTiming.enableHold)      ||
                    lcdBusIO(driver, 1, 0, 0, 0) < 0
                ))
            )
            {
                // IO failed.
                driver->error = EIO;
                return -1;
            }
        } while (value & (1 << 7)); // Busy flag is the 7th bit.

        return lcdBusIO(driver, 0, 0, 0, 0);
    }
}

int lcdInit4Bit(lcdDriver_t *driver)
{
    uint8_t cmd = LCD_CMD_FUNCTION(1, 0, 0);

    // Do operations on the bus as if the display is in 8 bit mode.
    if (
        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
        lcdBusIO(driver, 0, 0, 1, cmd) < 0                      || // Set 8 bit mode.
        lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      ||
        lcdDelay(driver, 5000) != 0                             || // Sleep for 5ms. (from hitachi manual)

        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
        lcdBusIO(driver, 0, 0, 1, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      || // Set 8 bit mode again.
        lcdDelay(driver, 100) != 0                              || // Sleep for 100 uS (from hitachi manual)

        lcdBusIO(driver, 0, 0, 1, cmd) < 0                      || // Set 8 bit mode once again.
        lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.dataHold) != 0
    )
    {
        driver->error = EIO;
        return -1;
    }

    cmd = LCD_CMD_FUNCTION(0, 0, 0);

    if (
        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.addressSetup) != 0   ||
        lcdBusIO(driver, 0, 0, 1, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.enableHold) != 0     ||
        lcdBusIO(driver, 0, 0, 0, cmd) < 0                      ||
        lcdDelay(driver, driver->busTiming.dataHold + 100) != 0    // Request four bit mode, still in 8 bit mode.
    )
    {
        driver->error = EIO;
        return -1;
    }

    return lcdCommand(driver, cmd);                // Final request for four bit mode, LCD is initialized.
}

int lcdInit(lcdDriver_t *driver)
{
    assert(driver);
    driver->cursor.x = 0;
    driver->cursor.y = 0;

    if (
        driver->fourBits &&
        (
            lcdInit4Bit(driver) ||
            lcdCommand(driver, LCD_CMD_FUNCTION(0, driver->dimensions.height > 1, driver->largeFont)) ||
            lcdDelay(driver, driver->busTiming.busyHoldShort)
        )
    )
    {
        return -1;
    }
    else if (!driver->fourBits)
    {
        assert(false);
    }

    return lcdClear(driver);
}
