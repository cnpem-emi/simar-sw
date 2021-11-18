#include <unistd.h>

#include "lcd.h"

int main() {
  lcd_init("\xb7", "\x13");
  lcd_set_cursor(0, 5);
  lcd_print("Test");

  sleep(1);

  for (int i = 0; i < 7; i++) {
    lcd_line(i * 6, i, 5, 2);
    usleep(300000);
  }

  for (int i = 6; i >= 0; i--) {
    lcd_line(78 - (i * 6), i, 5, 2);
    usleep(300000);
  }

  lcd_close();
}
