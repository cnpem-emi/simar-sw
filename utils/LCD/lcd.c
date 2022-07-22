/// @file lcd.c

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../../SPI/common.h"
#include "lcd.h"

#define AUTOCS 0

static uint32_t spi_mode = 0;
static uint8_t bits = 8;
static uint32_t speed = 400000;
static uint16_t tx_delay;
static const char* device = "/dev/spidev0.0";
static int fd;

static gpio_t rst = {.pin = P8_9};
static gpio_t dc = {.pin = P8_8};
static gpio_t bl = {.pin = P8_10};

const char lcd_table[96][5] = {
    "\x00\x00\x00\x00\x00",  // space
    "\x00\x00\x5f\x00\x00",  // !
    "\x00\x07\x00\x07\x00",  // "
    "\x14\x7f\x14\x7f\x14",  // #
    "\x24\x2a\x7f\x2a\x12",  // $
    "\x23\x12\x08\x64\x62",  // %
    "\x36\x49\x55\x22\x50",  // &
    "\x00\x05\x03\x00\x00",  // '
    "\x00\x1c\x22\x41\x00",  // (
    "\x00\x41\x22\x1c\x00",  // )
    "\x14\x08\x3E\x08\x14",  // *
    "\x08\x08\x3E\x08\x08",  // +
    "\x00\x50\x30\x00\x00",  // ,
    "\x08\x08\x08\x08\x08",  // -
    "\x00\x60\x60\x00\x00",  // .
    "\x20\x10\x08\x04\x02",  // /
    "\x3E\x51\x49\x45\x3E",  // 0
    "\x00\x42\x7F\x40\x00",  // 1
    "\x42\x61\x51\x49\x46",  // 2
    "\x21\x41\x45\x4B\x31",  // 3
    "\x18\x14\x12\x7F\x10",  // 4
    "\x27\x45\x45\x45\x39",  // 5
    "\x3C\x4A\x49\x49\x30",  // 6
    "\x01\x71\x09\x05\x03",  // 7
    "\x36\x49\x49\x49\x36",  // 8
    "\x06\x49\x49\x29\x1E",  // 9
    "\x00\x36\x36\x00\x00",  // :
    "\x00\x56\x36\x00\x00",  // ;
    "\x08\x14\x22\x41\x00",  // <
    "\x14\x14\x14\x14\x14",  // =
    "\x00\x41\x22\x14\x08",  // >
    "\x02\x01\x51\x09\x06",  // ?
    "\x32\x49\x59\x51\x3E",  // @
    "\x7E\x11\x11\x11\x7E",  // A
    "\x7F\x49\x49\x49\x36",  // B
    "\x3E\x41\x41\x41\x22",  // C
    "\x7F\x41\x41\x22\x1C",  // D
    "\x7F\x49\x49\x49\x41",  // E
    "\x7F\x09\x09\x09\x01",  // F
    "\x3E\x41\x49\x49\x7A",  // G
    "\x7F\x08\x08\x08\x7F",  // H
    "\x00\x41\x7F\x41\x00",  // I
    "\x20\x40\x41\x3F\x01",  // J
    "\x7F\x08\x14\x22\x41",  // K
    "\x7F\x40\x40\x40\x40",  // L
    "\x7F\x02\x0C\x02\x7F",  // M
    "\x7F\x04\x08\x10\x7F",  // N
    "\x3E\x41\x41\x41\x3E",  // O
    "\x7F\x09\x09\x09\x06",  // P
    "\x3E\x41\x51\x21\x5E",  // Q
    "\x7F\x09\x19\x29\x46",  // R
    "\x46\x49\x49\x49\x31",  // S
    "\x01\x01\x7F\x01\x01",  // T
    "\x3F\x40\x40\x40\x3F",  // U
    "\x1F\x20\x40\x20\x1F",  // V
    "\x3F\x40\x38\x40\x3F",  // W
    "\x63\x14\x08\x14\x63",  // X
    "\x07\x08\x70\x08\x07",  // Y
    "\x61\x51\x49\x45\x43",  // Z
    "\x00\x7F\x41\x41\x00",  // [
    "\x55\x2A\x55\x2A\x55",  // backslash
    "\x00\x41\x41\x7F\x00",  // ]
    "\x04\x02\x01\x02\x04",  // ^
    "\x40\x40\x40\x40\x40",  // _
    "\x00\x01\x02\x04\x00",  // `
    "\x20\x54\x54\x54\x78",  // a
    "\x7F\x48\x44\x44\x38",  // b
    "\x38\x44\x44\x44\x20",  // c
    "\x38\x44\x44\x48\x7F",  // d
    "\x38\x54\x54\x54\x18",  // e
    "\x08\x7E\x09\x01\x02",  // f
    "\x0C\x52\x52\x52\x3E",  // g
    "\x7F\x08\x04\x04\x78",  // h
    "\x00\x44\x7D\x40\x00",  // i
    "\x20\x40\x44\x3D\x00",  // j
    "\x7F\x10\x28\x44\x00",  // k
    "\x00\x41\x7F\x40\x00",  // l
    "\x7C\x04\x18\x04\x78",  // m
    "\x7C\x08\x04\x04\x78",  // n
    "\x38\x44\x44\x44\x38",  // o
    "\x7C\x14\x14\x14\x08",  // p
    "\x08\x14\x14\x18\x7C",  // q
    "\x7C\x08\x04\x04\x08",  // r
    "\x48\x54\x54\x54\x20",  // s
    "\x04\x3F\x44\x40\x20",  // t
    "\x3C\x40\x40\x20\x7C",  // u
    "\x1C\x20\x40\x20\x1C",  // v
    "\x3C\x40\x30\x40\x3C",  // w
    "\x44\x28\x10\x28\x44",  // x
    "\x0C\x50\x50\x50\x3C",  // y
    "\x44\x64\x54\x4C\x44",  // z
    "\x00\x08\x36\x41\x00",  //
    "\x00\x00\x7f\x00\x00",  // |
    "\x00\x41\x36\x08\x00",  //
    "\x10\x08\x08\x10\x08",  // ~
    "\x78\x46\x41\x46\x78"   // DEL
};

/**
 * @brief Transfers to LCD register
 *
 * @param[in] mode State to write to data command pin
 * @param[in] wr TX buffer
 * @returns Write operation result
 * @retval 0 -> Success
 * @retval 1 -> Failure
 */
int lcd_write(int mode, uint8_t const* wr, int len) {
  if (mode)
    mmio_set_high(dc);
  else
    mmio_set_low(dc);

  return spi_transfer(wr, wr, len);
}

/**
 * @brief Turns on or off LCD backlight
 * @param[in] state Desired backlight state (1 for on, 0 for off)
 * @returns void.
 */
void lcd_set_backlight(int state) {
  if (state)
    mmio_set_high(bl);
  else
    mmio_set_low(bl);
}

/**
 * @brief Sets LCD cursor position
 * @param[in] row Cursor Y position
 * @param[in] col Cursor X position
 * @return Result of cursor positioning operation
 * @retval 0 -> Success
 * @retval 1 -> X/Y position out of bounds
 */
int lcd_set_cursor(int row, int col) {
  if (row < 0 || row > LCD_HEIGHT / 8 + 1 || col < 0 || col > LCD_WIDTH / 6 + 1)
    return 1;

  char rowb[1] = {(64 | (row))};
  char colb[1] = {(128 | (col)*6)};

  lcd_write(0, rowb, 1);
  lcd_write(0, colb, 1);

  return 0;
}

/**
 * @brief Clears LCD
 * @returns void.
 */
void lcd_clear() {
  lcd_set_cursor(0, 0);
  for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT / 8; i++)
    lcd_write(1, "\x00", 1);
}

/**
 * @brief Prints string to LCD screen at a previously set position
 * @param[in] row String to write
 * @returns void.
 */
void lcd_print(char* strn) {
  int slen = strlen(strn);
  for (int i = 0; i < slen; i++) {
    int c_index = (int)strn[i] - 32;

    lcd_write(1, lcd_table[c_index], 5);
    lcd_write(1, "\x00", 1);  // Blank vertical line padding
  }
}

void lcd_print_char(char* strn) {
  lcd_write(1, strn, strlen(strn));
}

/**
 * @brief Prints straight horizontal line
 * @param[in] x Horizontal line starting position (x axis)
 * @param[in] y Horizontal line starting position (y axis)
 * @param[in] len Line length (in pixels)
 */
void lcd_line(int x, int y, int len, int thickness) {
  lcd_set_cursor(y / 8, x / 6);
  int mod_x = x % 6;
  int mod_y = y % 8;

  int fix_len = len + 1;

  char c;
  char full_char[fix_len];

  c = 0x01 << mod_y;

  if (thickness > 1) {
    for (int i = 1; i < thickness; i++)
      c |= c << 1;
  }

  for (int i = 0; i < fix_len; i++)
    full_char[i] = i >= mod_x ? c : 0;

  lcd_write(1, full_char, fix_len);
}

/**
 * @brief Initializes LCD
 * @returns void.
 */
void lcd_init(char* vop, char* bias) {
  fd = spi_open(device, &spi_mode, &bits, &speed);

  mmio_get_gpio(&rst);
  mmio_get_gpio(&bl);
  mmio_get_gpio(&dc);

  mmio_set_output(rst);
  mmio_set_output(bl);
  mmio_set_output(dc);

  mmio_set_low(rst);
  nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
  mmio_set_high(rst);
  nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);

  lcd_write(0, "\x21", 1);  // Ext. cmd set
  lcd_write(0, vop, 1);     // LCD contrast (VOP)
  lcd_write(0, "\x04", 1);  // Temperature coefficient
  lcd_write(0, bias, 1);    // Voltage Bias

  lcd_write(0, "\x20", 1);  // Normal cmd set
  lcd_write(0, "\x0c", 1);  // Normal display (0x0d for inverted)

  lcd_clear();

  lcd_set_backlight(0);
  lcd_set_cursor(0, 0);
}

/**
 * @brief Finalizes LCD operations
 * @returns void.
 */
void lcd_close() {
  close(fd);
}
