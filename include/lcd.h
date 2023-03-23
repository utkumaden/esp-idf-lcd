#ifndef _LCD_H_
#define _LCD_H_

/**
 * @file lcd.h LCD driver header file.
 *
 * @mainpage Generic C99 LCD Driver.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define WEAK __attribute__((weak))
#elif defined(_MSC_VER)
// Not really what I want but it is what I have.
#define WEAK __declspec(selectany)
#elif defined(__CC_ARM)
#define WEAK __weak
#elif
#warning Unable to infer weak symbol macro.
#define WEAK
#endif

#ifndef EIO
#define EIO 5
#endif

// Raw command definitions.
/** Clear screen */
#define LCD_CMD_CLEAR()         (0x01)
/** Move cursor back to start, cancel display shift. */
#define LCD_CMD_HOME()          (0x02)
/**
 * Set entry mode.
 * @param dir Entry direction, true for right, false for left.
 * @param s  Shift enable. Display RAM contents are shifted in the direction on read or write.
 */
#define LCD_CMD_ENTRY(dir,s)    (0x04 | ((!!(dir)) << 1) | (!!(s)))
/**
 * Set display mode.
 * @param f Disable or enable entire display.
 * @param c Disable or enable underline cursor.
 * @param b Disable or enable blinking block.
 */
#define LCD_CMD_DISPLAY(f,c,b)  (0x08 | ((!!(f)) << 2) | ((!!(c)) << 1) | (!!(b)))
/**
 * Move cursor or shift display.
 * @param s Shift for true and move cursor for false.
 * @param r True for right and false for left.
 */
#define LCD_CMD_CURSOR(s,r)     (0x10 | ((!!(s)) << 3) | ((!!(r)) << 2))
/**
 * Function mode set. Initializes the display.
 * @param b Bus width. false for 4bits and true for 8bits.
 * @param n Display lines. false for one line and true for two lines.
 * @param f Display font. false for small font (5x8) and true for large font (5x10)
 */
#define LCD_CMD_FUNCTION(b,n,f) (0x20 | ((!!(b)) << 4) | ((!!(n)) << 3) | ((!!(f)) << 2))
/**
 * Set character RAM pointer.
 * @param addr Pointer into character RAM.
 */
#define LCD_CMD_CADDR(addr)     (0x40 | (addr & 0x3F))
/**
 * Set display RAM pointer.
 * @param addr Pointer into display RAM.
 */
#define LCD_CMD_DADDR(addr)     (0x80 | (addr & 0x7F))

#define LCD_TIMING_ADDRESS_SETUP    10
#define LCD_TIMING_ENABLE_HOLD      10
#define LCD_TIMING_DATA_HOLD        10
#define LCD_TIMING_BUSY_INTERVAL    50
#define LCD_TIMING_BUSY_HOLD_SHORT  500
#define LCD_TIMING_BUSY_HOLD_LONG   50000

/**
 * Driver structure.
 */
typedef struct lcdDriver_t lcdDriver_t;

/**
 * Write read from the LCD bus.
 * @param driver The driver structure calling, for convenience.
 * @param rw Read/Write pin state.
 * @param rs Register select pin state.
 * @param en Enable pin state.
 * @param data Data bus output, valid if rw is false.
 * @return Non-negative if successful, value of data bus if rw is true, negative on error.
 * @remarks
 * The driver need not be initialized using the function pointers. Implement the weakly
 * linked int lcdBusIO(lcdDriver_t*,bool,bool,bool,uint8_t) to handle IO without function
 * pointers.
 */
typedef int (*lcdBusIOHandler_t)(lcdDriver_t* driver, bool rw, bool rs, bool en, uint8_t data);

/**
 * Suspend until at least delay microseconds.
 * @param driver The driver structure calling, for convenience.
 * @param delay Delay amount in microseconds.
 * @return Zero if successful, non-zero for error.
 * @remarks
 * Precision is not critical, as long as at least delay microseconds pass.
 *
 * The driver need not be initialized using the function pointers. Implement the weakly
 * linked int lcdDelay(lcdDriver_t*,uint32_t) to handle delay without function
 * pointers.
 */
typedef int (*lcdDelayHandler_t)(lcdDriver_t* driver, uint32_t delay);

/**
 * The LCD driver structure.'
 */
struct lcdDriver_t
{
    int   error;                    /** Last error number for LCD driver, using POSIX error numbers. */
    struct {
        uint8_t width;              /** Width of display. */
        uint8_t height;             /** Height of display. */
    } dimensions;                   /** Display dimensions. */
    bool fourBits:1;                /** Operate display in 4-bit mode. */
    bool writeOnly:1;               /** Write only mode of operation. */
    bool largeFont:1;               /** Use large font (5x10). */
    uint8_t padding0:5;             /** Padding for flags */
    void *userData;                 /** Storage for your usage */
    lcdBusIOHandler_t busIO;        /** LCD IO function handler, if not strongly linked. */
    lcdDelayHandler_t delay;        /** Delay function used to ensure bus timing, if not strongly linked. */


    struct {
        uint32_t addressSetup;      /** Time to wait after setting up address lines. */
        uint32_t enableHold;        /** Time to wait after setting enable high. */
        uint32_t dataHold;          /** Time to wait after setting enable low. */
        uint32_t busyInterval;      /** (read-write mode) Busy flag check interval. */
        uint32_t busyHoldShort;     /** (write-only mode) Hold time after write, short version. */
        uint32_t busyHoldLong;      /** (write-only mode) Hold time after write, long version.  */
    } busTiming;                    /** Bus timing variables. Great for tuning for specific displays. See void lcdLoadDefaultTiming(lcdDriver_t*). */

    /* private to implementation, modify at your own risk. */
    struct {
        int8_t x;
        int8_t y;
    } cursor;
    bool direction:1;
    uint8_t padding1:7;
};

/**
 * Function to control the LCD bus.
 * @param driver The driver structure.
 * @param rw Read-Write state.
 * @param rs Register Select.
 * @param en Enable state.
 * @param data Data pins.
 * @return Non-zero when an error occurs. Updates errno.
 * @remarks This function is declared weak. Redefine the function to implement
 * a custom bus controller. By default, it will try to call the related function
 * pointer in the driver structure.
 */
int WEAK lcdBusIO(lcdDriver_t *driver, bool rw, bool rs, bool en, uint8_t data);

/**
 * Function to delay execution in the driver.
 * @param driver The driver structure.
 * @param delay the amount to delay in microseconds.
 * @return Non-negative on success, bus value if reading, negative on error.
 * @remarks This function is declared weak. Redefine the function to implement
 * a custom delay function. By default, it will try to call the related function
 * pointer in the driver structure. The delay need not be exact, as long as the
 * function returns after _at least_ the given delay value.
 */
int WEAK lcdDelay(lcdDriver_t *driver, uint32_t delay);

/**
 * Load default bus timings into the driver structure.
 * @param driver The driver structure.
 * @remarks See LCD_TIMING_* for default values.
 */
inline static void lcdLoadDefaultTiming(lcdDriver_t *driver)
{
    driver->busTiming.addressSetup  = LCD_TIMING_ADDRESS_SETUP;
    driver->busTiming.enableHold    = LCD_TIMING_ENABLE_HOLD;
    driver->busTiming.dataHold      = LCD_TIMING_DATA_HOLD;
    driver->busTiming.busyInterval  = LCD_TIMING_BUSY_INTERVAL;
    driver->busTiming.busyHoldShort = LCD_TIMING_BUSY_HOLD_SHORT;
    driver->busTiming.busyHoldLong  = LCD_TIMING_BUSY_HOLD_LONG;
}

/**
 * Initialize the LCD display.
 * @param driver The driver structure.
 * @return Non-zero if unsuccessful.
 */
int lcdInit(lcdDriver_t *driver);

/**
 * Write a command to the LCD bus.
 * @param driver The driver structure.
 * @param command The command byte.
 * @return Non-zero if unsuccessful.
 */
int lcdCommand(lcdDriver_t *driver, uint8_t command);

/**
 * Write data to display or character RAM.
 * @param driver The driver sturcure.
 * @param data Data to write.
 * @return Non-zero if unsuccessful.
 */
int lcdWrite(lcdDriver_t *driver, uint8_t data);

/**
 * Decode the cursor position in the driver.
 * @param driver The driver structure.
 * @return The address for the current position pointer.
 * @remarks This macro is private to the driver. You should not need
 * to use this.
 */
#define LCD_DECODE_CURSOR(driver)\
    ((driver)->cursor.x +                                                   /* Base position */\
        ( \
            64 * ((driver)->cursor.y % 2) +                         /* Add 64 if the row is even */\
            (driver)->dimensions.width * ((driver)->cursor.y >= 2)  /* Add width if the row is the last two. */\
        ) \
    )
// #include <stdio.h>
// inline static uint8_t _LCD_DECODE_CURSOR(lcdDriver_t* driver) {
//     uint8_t value = LCD_DECODE_CURSOR(driver);
//     printf("%hhd, %hhd = %hhu\n", driver->cursor.x, driver->cursor.y, value);
//     return value;
// }
// #undef LCD_DECODE_CURSOR
// #define LCD_DECODE_CURSOR(driver) _LCD_DECODE_CURSOR(driver)

/**
 * Update the LCD cursor in the driver.
 * @param driver The driver structure.
 * @remarks This is private to the driver implementation. You should not need to call
 * this function on your own.
 */
inline static void lcdUpdateCursor(lcdDriver_t *driver)
{
    if (driver->direction)
    {
        driver->cursor.x++;
        if (driver->cursor.x >= driver->dimensions.width)
        {
            driver->cursor.x = 0;
            driver->cursor.y++;
            if (driver->cursor.y >= driver->dimensions.height)
                driver->cursor.y = 0;
        }
    }
    else
    {
        driver->cursor.x--;
        if (driver->cursor.x < 0)
        {
            driver->cursor.x = driver->dimensions.width - 1;
            driver->cursor.y--;
            if (driver->cursor.y < 0)
                driver->cursor.y = driver->dimensions.height - 1;
        }
    }
}

/**
 * Clear the LCD.
 * @param driver The driver structure.
 * @return Non-zero value on error. Updates errno.
 */
inline static int lcdClear(lcdDriver_t *driver)
{
    assert(driver);
    if (lcdCommand(driver, LCD_CMD_CLEAR()) || lcdDelay(driver, driver->busTiming.busyHoldShort))
    {
        return -1;
    }
    else
    {
        driver->cursor.x = 0;
        driver->cursor.y = 0;
        return 0;
    }
}

/**
 * Put the cursor in the home position.
 * @param driver The driver structure.
 * @return Non-zero value on error. Updates errno.
 */
inline static int lcdHome(lcdDriver_t *driver)
{
    assert(driver);
    if (lcdCommand(driver, LCD_CMD_HOME()) || lcdDelay(driver, driver->busTiming.busyHoldLong))
    {
        return -1;
    }
    else
    {
        driver->cursor.x = 0;
        driver->cursor.y = 0;
        return 0;
    }
}

/**
 * Change the LCD write direction.
 * @param driver The driver structure.
 * @param forward True for forward direction.
 * @return Non-zero value on error. Updates errno.
 */
inline static int lcdDirection(lcdDriver_t *driver, bool forward)
{
    assert(driver);
    if (lcdCommand(driver, LCD_CMD_ENTRY(forward, 0)) || lcdDelay(driver, driver->busTiming.busyHoldShort))
    {
        return -1;
    }
    else
    {
        driver->direction = forward;
        return 0;
    }
}

/**
 * Move the cursor to the next character.
 * @param driver The driver structure.
 * @return Non-zero value on error. Updates errno.
 */
inline static int lcdNext(lcdDriver_t *driver)
{
    assert(driver);
    lcdUpdateCursor(driver);
    if (lcdCommand(driver, LCD_CMD_DADDR(LCD_DECODE_CURSOR(driver))))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * Set the display mode of the LCD.
 * @param driver The driver structure.
 * @param display True to enable character display.
 * @param cursor True to enable the cursor.
 * @param blink True to enable cursor blinking.
 * @return Non-zero value on error. Updates errno.
 */
inline static int lcdSetDisplay(lcdDriver_t *driver, bool display, bool cursor, bool blink)
{
    assert(driver);
    return lcdCommand(driver, LCD_CMD_DISPLAY(display, cursor, blink));
}

/**
 * Set the cursor position on the LCD.
 * @param driver The driver structure.
 * @param column The LCD column.
 * @param row The LCD row.
 * @return Non-zero value on error. Updates errno.
 */
inline static int lcdSetCursor(lcdDriver_t *driver, uint8_t column, uint8_t row)
{
    assert(driver);
    assert(driver->dimensions.width > column);
    assert(driver->dimensions.height > row);

    driver->cursor.x = column;
    driver->cursor.y = row;

    uint8_t address = LCD_DECODE_CURSOR(driver);
    return lcdCommand(driver, LCD_CMD_DADDR(address));
}

/**
 * Store a custom glyph to the LCD character RAM.
 * @param driver The driver structure.
 * @param which The character to substitute.
 * @param bits The bit pattern for the character.
 * @return Non-zero value on error. Updates errno.
 * @remarks When using a large font, there can only be 4 custom characters,
 * otherwise there can be 8. The characters must be in the range of 0-8, and
 * must be even in case of a large font.
 */
inline static int lcdStoreGlyph(lcdDriver_t *driver, char which, const uint8_t *bits)
{
    assert(driver);
    assert((driver->largeFont && (which >> 1) < 4) || (which < 8));
    assert(bits);

    uint8_t address = (driver->largeFont ? which & 0x06 : which) << 3;
    if (lcdCommand(driver, LCD_CMD_CADDR(address)))
        return -1;

    for (int i = 0; i < (driver->largeFont ? 10 : 8); i++)
    {
        if (lcdWrite(driver, bits[i])) return -1;
    }

    return 0;
}

/**
 * Put a single character on the LCD display.
 * @param driver The driver structure.
 * @param chr The character to put.
 * @return Non-zero value on error. Updates errno.
 * @see lcdPutString
 * @see lcdPutZString
 */
inline static int lcdPutChar(lcdDriver_t *driver, char chr)
{
    assert(driver);
    if (lcdCommand(driver, LCD_CMD_DADDR(LCD_DECODE_CURSOR(driver))) || lcdWrite(driver, chr))
        return -1;

    lcdUpdateCursor(driver);

    return lcdCommand(driver, LCD_CMD_DADDR(LCD_DECODE_CURSOR(driver)));
}

/**
 * Put a string of known length on the LCD display.
 * @param driver The driver structure.
 * @param str The string.
 * @param length Length of the string.
 * @return Non-zero value on error. Updates errno.
 * @see lcdPutZString @see lcdPutChar
 */
inline static int lcdPutString(lcdDriver_t *driver, const char *str, size_t length)
{
    assert(driver);
    assert(str);

    uint8_t address = LCD_DECODE_CURSOR(driver);
    if (lcdCommand(driver, LCD_CMD_DADDR(address))) return -1;

    for (int i = 0; i < length; i++)
    {
        if (lcdWrite(driver, str[i])) return -1;

        lcdUpdateCursor(driver);

        {
            uint8_t newaddr = LCD_DECODE_CURSOR(driver);
            if (newaddr != address + 1)
            {
                if (lcdCommand(driver, LCD_CMD_DADDR(address))) return -1;
            }
            address = newaddr;
        }
    }

    return lcdCommand(driver, LCD_CMD_DADDR(address));
}

/**
 * Put a null terminated string on the LCD display.
 * @param driver The driver structure.
 * @param str The string.
 * @return Non-zero value on error.
 * @remarks Uses strlen internally.
 * @see lcdPutString @see lcdPutChar
 */
#define lcdPutZString(driver, str) lcdPutString(driver, str, strlen(str))

// int lcdRead(lcdDriver_t *driver, uint8_t *data); // Not implemented.

#endif
