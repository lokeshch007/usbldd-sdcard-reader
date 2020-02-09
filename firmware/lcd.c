#include "lcd.h"

void lcd_write_nibble(uint8_t rs_flag, uint8_t val)
{
	//1. set RS=0 (instruction) or RS=1 (data)
	if(rs_flag == LCD_CMD)
		LCD_CTRL_PORT->FIOCLR = BV(LCD_RS);
	else
		LCD_CTRL_PORT->FIOSET |= BV(LCD_RS);

	//2. RW=0
	LCD_CTRL_PORT->FIOCLR = BV(LCD_RW);
	//3. Write nibble data
	LCD_DATA_PORT->FIOCLR = BV(LCD_D4) | BV(LCD_D5) | BV(LCD_D6) | BV(LCD_D7);
	LCD_DATA_PORT->FIOSET |= (uint32_t)val << LCD_D4;
	//4. EN=1 ... EN=0
	LCD_CTRL_PORT->FIOSET |= BV(LCD_EN);
	SW_DELAY_MS(1);
	LCD_CTRL_PORT->FIOCLR = BV(LCD_EN);
}

void lcd_busy_wait(void)
{
	//0. Make LCD data port pin as input.
	LCD_DATA_PORT->FIODIR &= ~BV(LCD_D7);
	//1. RS=0
	LCD_CTRL_PORT->FIOCLR = BV(LCD_RS);
	//2. RW=1	//3. EN=1
	LCD_CTRL_PORT->FIOSET |= BV(LCD_RW) | BV(LCD_EN);
	//4. read LCD data & check D7 bit; if bit=1, repeat (this line).
	while( (LCD_DATA_PORT->FIOPIN & BV(LCD_D7)) != 0 )
		;
	//5. EN=0 //6. RW=0
	LCD_CTRL_PORT->FIOCLR = BV(LCD_EN) | BV(LCD_RW);
	//7. Make LCD data port pin as output.
	LCD_DATA_PORT->FIODIR |= BV(LCD_D7);
}

void lcd_write_byte(uint8_t rs_flag, uint8_t val)
{
	uint8_t hi = val >> 4;
	uint8_t lo = val & 0x0F;
	//1. write high nibble
	lcd_write_nibble(rs_flag, hi);
	//2. write low nibble
	lcd_write_nibble(rs_flag, lo);
	//3. check busy flag
	lcd_busy_wait();
	SW_DELAY_MS(3);
}

void lcd_puts(uint8_t line, char *str)
{
	int i;
	//1. set line address:
	lcd_write_byte(LCD_CMD, line);
	//2. write char data one by one.
	for(i=0; str[i]!='\0'; i++)
		lcd_write_byte(LCD_DATA, str[i]);
}

void lcd_init(void)
{
	//1. set [data pins, control pins] io port pins as output.
	LCD_DATA_PORT->FIODIR |= BV(LCD_D4) | BV(LCD_D5) | BV(LCD_D6) | BV(LCD_D7); 
	LCD_CTRL_PORT->FIODIR |= BV(LCD_RW) | BV(LCD_RS) | BV(LCD_EN);
	LCD_DATA_PORT->FIOCLR = BV(LCD_D4) | BV(LCD_D5) | BV(LCD_D6) | BV(LCD_D7); 
	LCD_CTRL_PORT->FIOCLR = BV(LCD_RW) | BV(LCD_RS) | BV(LCD_EN);
	SW_DELAY_MS(200);
	//2. lcd func set: send instru 0x2C [4-bit mode, 2 line, 5x10 font].
	lcd_write_byte(LCD_CMD, 0x2C);
	//3. lcd display:  send instru 0x0C [disp on, cursor off, blink off]
	lcd_write_byte(LCD_CMD, 0x0C);
	//4. lcd entry:    send instru 0x06 [incr addr, no shift].
	lcd_write_byte(LCD_CMD, 0x06);
	//5. lcd clear:    send instru 0x01 [display clear]
	lcd_write_byte(LCD_CMD, 0x01);
	SW_DELAY_MS(200);
}

void lcd_clear(void)
{
	lcd_write_byte(LCD_CMD, 0x01);
}




