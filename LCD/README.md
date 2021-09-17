# Nokia 5110 SPI display communication

Currently available:
- void lcd_init();
    - Initializes LCD
- char *lcd_write(int mode, char *wr);
    - Writes to LCD registers
        - mode: writing mode (0 for data, 1 for command)
        - wr: data to write
- void lcd_set_backlight(int state);
    - Changes backlight state
        - state: 1 for on, 0 for off 
- void lcd_clear();
    - Clears LCD screen
- int lcd_set_cursor(int row, int col);
    - Sets cursor position
- void lcd_print(char* strn);
    - Prints string to LCD screen
        - strn: string to print
- void lcd_line(int x, int y, int len, int thickness);
    - Prints line to LCD screen
        - x: Starting point on the x axis
        - y: Starting point on the y axis
        - len: Line length
	    - thickness: Line thickness (in pixels)