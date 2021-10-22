#ifndef LCD_H
#define LCD_H

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

#include <stdint.h>

typedef struct {
  int header;
  int pin;
} Pin;

void lcd_line(int x, int y, int len, int thickness);
void lcd_init(char* vop, char* bias);
void lcd_close();
int lcd_write(int mode, uint8_t const* wr, int len);
void lcd_set_backlight(int state);
void lcd_clear();
int lcd_set_cursor(int row, int col);
void lcd_print(char* strn);
void lcd_print_char(char* strn);

#endif
