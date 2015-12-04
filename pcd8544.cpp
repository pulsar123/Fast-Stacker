#include <stdint.h>
#include "pcd8544.h"

#ifdef MAPLE
#include <HardwareSPI.h>
#include <maple.h>
#define PROGMEM __attribute__ ((section (".USER_FLASH")))
#define pgm_read_byte(adr) *(adr)

HardwareSPI spi(SPI_NUM);

#else  // Arduino stuff
#include <pins_arduino.h>
#include <avr/pgmspace.h>
#endif


// LCD commands, Table 1, page 14
#define PCD8544_FUNCTION_SET (1<<5)
#define PCD8544_FUNCTION_PD  (1<<2)
#define PCD8544_FUNCTION_V   (1<<1)
#define PCD8544_FUNCTION_H   (1<<0)

// Normal instructions, H = 0
#define PCD8544_DISPLAY_CONTROL (1<<3)
#define PCD8544_DISPLAY_CONTROL_D (1<<2)
#define PCD8544_DISPLAY_CONTROL_E (1<<0)
#define PCD8544_DISPLAY_CONTROL_BLANK 0
#define PCD8544_DISPLAY_CONTROL_NORMAL_MODE  PCD8544_DISPLAY_CONTROL_D
#define PCD8544_DISPLAY_CONTROL_ALL_ON       PCD8544_DISPLAY_CONTROL_E
#define PCD8544_DISPLAY_CONTROL_INVERSE      (PCD8544_DISPLAY_CONTROL_D|PCD8544_DISPLAY_CONTROL_E)

#define PCD8544_SET_Y_ADDRESS (1<<6)
#define PCD8544_Y_ADRESS_MASK 0b111
#define PCD8544_SET_X_ADDRESS (1<<7)
#define PCD8544_X_ADRESS_MASK 0b01111111

// Extended instructions. H = 1
#define PCD8544_TEMP_CONTROL (1<<2)
#define PCD8544_TEMP_TC1     (1<<1)
#define PCD8544_TEMP_TC0     (1<<0)

#define PCD8544_BIAS     (1<<4)
#define PCD8544_BIAS_BS2 (1<<2)
#define PCD8544_BIAS_BS1 (1<<1)
#define PCD8544_BIAS_BS0 (1<<0)

#define PCD8544_VOP (1<<7)


const unsigned char PROGMEM small_num[][4] = {
        {0x0e,0x15,0x0e,0x00}, // 48, zero
        {0x12,0x1f,0x10,0x00}, // 49, one
        {0x12,0x19,0x16,0x00}, // 50, two
        {0x11,0x15,0x0b,0x00}, // 51, three
        {0x07,0x04,0x1f,0x00}, // 52, four
        {0x17,0x15,0x09,0x00}, // 53, five
        {0x0e,0x15,0x09,0x00}, // 54, six
        {0x19,0x05,0x03,0x00}, // 55, seven
        {0x1a,0x15,0x0b,0x00}, // 56, eight
        {0x12,0x15,0x0e,0x00}, // 57, nine
        {0x00,0x10,0x00,0x00}, // 46, period
};


const unsigned char PROGMEM font6x8 [][5]  =  {
	{0x00,0x00,0x00,0x00,0x00,}, // ' ' 32
	{0x00,0x00,0x5F,0x00,0x00,}, // '!' 33
	{0x00,0x07,0x00,0x07,0x00,}, // '"' 34
	{0x14,0x7F,0x14,0x7F,0x14,}, // '#' 35
	{0x24,0x2A,0x7F,0x2A,0x12,}, // '$' 36
	{0x23,0x13,0x08,0x64,0x62,}, // '%' 37
	{0x36,0x49,0x55,0x22,0x50,}, // '&' 38
	{0x00,0x05,0x03,0x00,0x00,}, // ''' 39
	{0x00,0x1C,0x22,0x41,0x00,}, // '(' 40
	{0x00,0x41,0x22,0x1C,0x00,}, // ')' 41
	{0x14,0x08,0x3E,0x08,0x14,}, // '*' 42
	{0x08,0x08,0x3E,0x08,0x08,}, // '+' 43
	{0x00,0x50,0x30,0x00,0x00,}, // ',' 44
	{0x08,0x08,0x08,0x08,0x08,}, // '-' 45
	{0x00,0x60,0x60,0x00,0x00,}, // '.' 46
	{0x20,0x10,0x08,0x04,0x02,}, // '/' 47
	{0x3E,0x51,0x49,0x45,0x3E,}, // '0' 48
	{0x00,0x42,0x7F,0x40,0x00,}, // '1' 49
	{0x42,0x61,0x51,0x49,0x46,}, // '2' 50
	{0x21,0x41,0x45,0x4B,0x31,}, // '3' 51
	{0x18,0x14,0x12,0x7F,0x10,}, // '4' 52
	{0x27,0x45,0x45,0x45,0x39,}, // '5' 53
	{0x3C,0x4A,0x49,0x49,0x30,}, // '6' 54
	{0x03,0x01,0x71,0x09,0x07,}, // '7' 55
	{0x36,0x49,0x49,0x49,0x36,}, // '8' 56
	{0x06,0x49,0x49,0x29,0x16,}, // '9' 57
	{0x00,0x36,0x36,0x00,0x00,}, // ':' 58
	{0x00,0x56,0x36,0x00,0x00,}, // ';' 59
	{0x08,0x14,0x22,0x41,0x00,}, // '<' 60
	{0x14,0x14,0x14,0x14,0x14,}, // '=' 61
	{0x00,0x41,0x22,0x14,0x08,}, // '>' 62
	{0x02,0x01,0x51,0x09,0x06,}, // '?' 63
	{0x32,0x49,0x79,0x41,0x3E,}, // '@' 64
	{0x7E,0x11,0x11,0x11,0x7E,}, // 'A' 65
	{0x7F,0x49,0x49,0x49,0x36,}, // 'B' 66
	{0x3E,0x41,0x41,0x41,0x22,}, // 'C' 67
	{0x7F,0x41,0x41,0x22,0x1C,}, // 'D' 68
	{0x7F,0x49,0x49,0x49,0x41,}, // 'E' 69
	{0x7F,0x09,0x09,0x01,0x01,}, // 'F' 70
	{0x3E,0x41,0x49,0x49,0x3A,}, // 'G' 71
	{0x7F,0x08,0x08,0x08,0x7F,}, // 'H' 72
	{0x00,0x41,0x7F,0x41,0x00,}, // 'I' 73
	{0x20,0x41,0x41,0x3F,0x01,}, // 'J' 74
	{0x7F,0x08,0x14,0x22,0x41,}, // 'K' 75
	{0x7F,0x40,0x40,0x40,0x40,}, // 'L' 76
	{0x7F,0x02,0x0C,0x02,0x7F,}, // 'M' 77
	{0x7F,0x04,0x08,0x10,0x7F,}, // 'N' 78
	{0x3E,0x41,0x41,0x41,0x3E,}, // 'O' 79
	{0x7F,0x09,0x09,0x09,0x06,}, // 'P' 80
	{0x3E,0x41,0x51,0x21,0x5E,}, // 'Q' 81
	{0x7F,0x09,0x19,0x29,0x46,}, // 'R' 82
	{0x26,0x49,0x49,0x49,0x32,}, // 'S' 83
	{0x01,0x01,0x7F,0x01,0x01,}, // 'T' 84
	{0x3F,0x40,0x40,0x40,0x3F,}, // 'U' 85
	{0x1F,0x20,0x40,0x20,0x1F,}, // 'V' 86
	{0x3F,0x40,0x38,0x40,0x3F,}, // 'W' 87
	{0x63,0x14,0x08,0x14,0x63,}, // 'X' 88
	{0x07,0x08,0x70,0x08,0x07,}, // 'Y' 89
	{0x61,0x51,0x49,0x45,0x43,}, // 'Z' 90
	{0x00,0x7F,0x41,0x41,0x00,}, // '[' 91
	{0x02,0x04,0x08,0x10,0x20,}, // '\' 92
	{0x00,0x41,0x41,0x7F,0x00,}, // ']' 93
	{0x04,0x02,0x01,0x02,0x04,}, // '^' 94
	{0x40,0x40,0x40,0x40,0x40,}, // '_' 95
	{0x00,0x01,0x02,0x04,0x00,}, // '`' 96
	{0x20,0x54,0x54,0x54,0x78,}, // 'a' 97
	{0x7F,0x48,0x44,0x44,0x38,}, // 'b' 98
	{0x38,0x44,0x44,0x44,0x20,}, // 'c' 99
	{0x38,0x44,0x44,0x48,0x3F,}, // 'd' 100
	{0x38,0x54,0x54,0x54,0x18,}, // 'e' 101
	{0x08,0x7E,0x09,0x01,0x02,}, // 'f' 102
	{0x0C,0x52,0x52,0x52,0x3E,}, // 'g' 103
	{0x7F,0x08,0x04,0x04,0x78,}, // 'h' 104
	{0x00,0x44,0x7D,0x40,0x00,}, // 'i' 105
	{0x20,0x40,0x45,0x3C,0x00,}, // 'j' 106
	{0x7F,0x10,0x28,0x44,0x00,}, // 'k' 107
	{0x00,0x41,0x7F,0x40,0x00,}, // 'l' 108
	{0x7C,0x04,0x18,0x04,0x78,}, // 'm' 109
	{0x7C,0x08,0x04,0x04,0x78,}, // 'n' 110
	{0x38,0x44,0x44,0x44,0x38,}, // 'o' 111
	{0x7C,0x14,0x14,0x14,0x08,}, // 'p' 112
	{0x08,0x14,0x14,0x18,0x7C,}, // 'q' 113
	{0x7C,0x08,0x04,0x04,0x08,}, // 'r' 114
	{0x48,0x54,0x54,0x54,0x20,}, // 's' 115
	{0x04,0x3F,0x44,0x40,0x20,}, // 't' 116
	{0x3C,0x40,0x40,0x20,0x7C,}, // 'u' 117
	{0x1C,0x20,0x40,0x20,0x1C,}, // 'v' 118
	{0x3C,0x40,0x30,0x40,0x3C,}, // 'w' 119
	{0x44,0x28,0x10,0x28,0x44,}, // 'x' 120
	{0x0C,0x50,0x50,0x50,0x3C,}, // 'y' 121
	{0x44,0x64,0x54,0x4C,0x44,}, // 'z' 122
	{0x00,0x08,0x36,0x41,0x00,}, // '{' 123
	{0x00,0x00,0x7F,0x00,0x00,}, // '|' 124
	{0x00,0x41,0x36,0x08,0x00,}, // '}' 125
	{0x10,0x08,0x08,0x10,0x08,}, // '~' 126
	{0x08,0x1C,0x2A,0x08,0x08,} //  <-  127
};




pcd8544::pcd8544(uint8_t dc_pin, uint8_t reset_pin, uint8_t cs_pin, uint8_t hardware_spi)
{
	dc = dc_pin;
	cs = cs_pin;
	reset = reset_pin;
	hardware_spi_num = hardware_spi;
	if (hardware_spi_num > 2)
		hardware_spi_num = 2;
#ifndef MAPLE
	sdin = MOSI;
	sclk = SCK;
#else
	sdin = 11;  // Change to maple names
	sclk = 13;
	if (hardware_spi_num  == 2) {
		sdin = 32;
		sclk = 34;
	}
#endif
}

pcd8544::pcd8544(uint8_t dc_pin, uint8_t reset_pin, uint8_t cs_pin, uint8_t sdin_pin, uint8_t sclk_pin)
{
	dc = dc_pin;
	cs = cs_pin;
	reset = reset_pin;
	sdin = sdin_pin;
	sclk = sclk_pin;
	hardware_spi_num = 0;
}

void pcd8544::begin(void)
{
//!!!! stacker
//	pinMode(cs,   OUTPUT);
//	pinMode(reset, OUTPUT);
	pinMode(dc,    OUTPUT);
	pinMode(sdin,  OUTPUT);
	pinMode(sclk,  OUTPUT);

#ifdef MAPLE
	timer_set_mode(TIMER3, 2, TIMER_DISABLED);
        timer_set_mode(TIMER3, 1, TIMER_DISABLED);
#endif


	if (hardware_spi_num > 0) {
#ifdef MAPLE
		spi.begin(SPI_4_5MHZ, MSBFIRST, 0);
		//spi_init(hardware_spi_num, SPI_PRESCALE_16, SPI_MSBFIRST, 0);
#else
		pinMode(SS, OUTPUT); // To ensure master mode
		SPCR |= (1<<SPE) | (1<<MSTR);
#endif
	}
 //!!!! stacker
//	digitalWrite(reset, LOW);
//	delay(1);
//	digitalWrite(reset, HIGH);

  
	// Extenden instructions and !powerdown
	// and horizontal adressing (autoincrement of x-adress)
	command(PCD8544_FUNCTION_SET | PCD8544_FUNCTION_H);
	// Set Vop to 0x3F
	command(PCD8544_VOP | 0x3F);  // 0x3F
	// Vlcd temp. coeff. 0
	command(PCD8544_TEMP_CONTROL);
	// Bias system 4, 1:48
	command(PCD8544_BIAS | PCD8544_BIAS_BS1 | PCD8544_BIAS_BS0);
	// Set H = 0 for normal instructions
	command(PCD8544_FUNCTION_SET);  
	// Normal mode
	command(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_CONTROL_NORMAL_MODE);

}


void pcd8544::clear(void)
{
	int i;
	for (i = 0; i < PCD8544_WIDTH*PCD8544_LINES; i++)
		data(0);
}

WRITE_RESULT pcd8544::write(uint8_t ch)
{
	uint8_t i;

	if (ch == '\r')
		gotoRc(current_row, 0);
	if (ch == '\n')
		gotoRc(current_row+1, current_column);
	if (ch >= ' ' && ch <= 127) {
		for (i = 0; i < 5; i++)
			data(pgm_read_byte(&font6x8[ch-' '][i]) <<1);
		data(0);
	}
	
	WRITE_RETURN;
}


void pcd8544::data(uint8_t data)
{
	send(1, data);
}

void pcd8544::command(uint8_t data)
{
	send(0, data);
}

void pcd8544::send(uint8_t data_or_command, uint8_t data)
{
	digitalWrite(dc, data_or_command);
//!!!!
//	digitalWrite(cs, LOW);
	if (hardware_spi_num == 0) {
		shiftOut(sdin, sclk, MSBFIRST, data);
	} else {
#ifdef MAPLE
		//spi_tx_byte(hardware_spi_num, data);
		spi.transfer(data);
#else
		SPDR = data;
		while(!(SPSR & (1<<SPIF))) ;
#endif
	}
//!!!!
//	digitalWrite(cs, HIGH);
	if(data_or_command)
		inc_row_column();
}


void pcd8544::setCursor(uint8_t column, uint8_t row)
{
	gotoRc(row, 6*column);
}


void pcd8544::gotoRc(uint8_t row, uint8_t column)
{
	if (row >= PCD8544_LINES)
		row %= PCD8544_LINES;
	if (column >= PCD8544_WIDTH)
		row %= PCD8544_WIDTH;
	command(PCD8544_SET_X_ADDRESS | column);
	command(PCD8544_SET_Y_ADDRESS | row);
	current_row = row;
	current_column = column;
}

void pcd8544::inc_row_column(void)
{
	if (++current_column >= PCD8544_WIDTH) {
		current_column = 0;
		if (++current_row >= PCD8544_LINES)
			    current_row = 0;
	}
}


void pcd8544::smallNum(uint8_t num, uint8_t shift)
{
	uint8_t i;
	for (i = 0; i < 4; i++)
		data(pgm_read_byte(&small_num[num][i])<<shift);
}


void pcd8544::clearRestOfLine(void)
{
	while (current_column != 0)
		data(0);
}


void pcd8544::bitmap(uint8_t bdata[], uint8_t rows, uint8_t columns)
{
	uint8_t row, column, i;
	uint8_t toprow = current_row;
	uint8_t startcolumn = current_column;
	for (row = 0, i = 0; row < rows; row++) {
		gotoRc(row+toprow, startcolumn);
		for (column = 0; column < columns; column++) {
			data(pgm_read_byte(&bdata[i++]));
		}
	}
}
