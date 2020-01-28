#include "_ffmc16.h"
#include "extern.h"
#include "monitor.h"

#define RS IO_PDR2.bit.P24		//RS at P24
#define RW IO_PDR5.bit.P53		//RW at P53
#define E IO_PDR5.bit.P52		//E at P52
#define BF IO_PDR2.bit.P23		//Busy Flag at P23
#define LCD_DATA_IN IO_DDR2.byte = 0xB0		//sets LCD data bus <P23:P20> to input (reading from LCD)
#define LCD_DATA_OUT IO_DDR2.byte = 0xBF	//sets LCD data bus <P23:P20> to input (reading from LCD)
#define LCD_DATA IO_PDR2.byte	//LCD Data Bus (4-bit)

#define DATA IO_PDR5.byte //keypad lines
#define DAVBL IO_PDR2.bit.P26 //DAVBL



// Function prototypes 
void init_LCD();
void inst_Ctrl(unsigned char data);
void data_Ctrl(unsigned char data);
void inst_Ctrl_4bit(unsigned char data);
void read_BF();
void delay(long int);
void LCD_Print(unsigned char line_num);

void main(void)
{
	
	__set_il(7);
	__EI();				//enable interrupt(for Accemic)
	//setting up reload timer
	//IO_DDR2.byte = 0xBF;		//sets all Port 2 to output except P26
	//IO_DDR5.byte = 0x0C;		//sets all Port 5 to input except P52 & P53
	//IO_ADER.byte = 0x03;		//set Port P52-P57 as digital I/O (not analog input) -- very important
	IO_DDR1.bit.D10 = 1;		//set P10 as output
	IO_PDR1.bit.P10 = 1;		//set LED to off
	
	
	IO_ICR03.byte = 0x00;		//set interrupt priority to highest
	IO_TMR[0] = 46875;			//set reload value (underflow timeout: 0.75s)
	IO_TMCSR0.bit.CSL = 2;		//use 25T count clock cycle; trigger disabled
	IO_TMCSR0.bit.RELD = 1;		//reload mode
	IO_TMCSR0.bit.INTE = 1;		//underflow interrupt enable
	IO_TMCSR0.bit.CNTE = 1;		//enable timer operation
	IO_TMCSR0.bit.TRG = 1;		//start counting after reload;start counter
	
	while(1);					//infinite loop

}

		
//write 8-bit instruction to Instruction Register (4-bit interface)
void inst_Ctrl(unsigned char data)
{
	LCD_DATA = data >> 4;		//upper 4-bit transfer
	RS = 0; RW = 0; E = 1;
	delay(1);					//16 us delay
	E = 0;
	LCD_DATA = data;			//lower 4-bit transfer
	RS = 0; RW = 0; E = 1;
	delay(1);					//16 us delay
	E = 0;
	//delay(500);
	read_BF();					//reading busy flag
}


// write 4-bit instruction to Instruction Register (4-bit interface)
void inst_Ctrl_4bit(unsigned char data)
{
	LCD_DATA = data;			//lower 4-bit transfer
	RS = 0; RW = 0; E = 1;
	delay(100);					//1.6 ms delay
	E = 0;
}


//write 8-bit data to LCD Data Register (4-bit interface)
void data_Ctrl(unsigned char data)
{
	LCD_DATA = data >> 4;		//upper 4-bit transfer
	RS = 1; RW = 0; E = 1;
	delay(10);					//16 us delay
	E = 0;
	LCD_DATA = data;			//lower 4-bit transfer
	RS = 1; RW = 0; E = 1;
	delay(10);					//16 us delay
	E = 0;
	read_BF();					//reading busy flag
}


//initialize LCD routine
void init_LCD()
{
	delay(1000);				//15 ms LCD startup delay
	inst_Ctrl_4bit(0x03);		
	delay(246);					//4.1 ms delay
	inst_Ctrl_4bit(0x03);		
	delay(6);					//100 us delay
	inst_Ctrl_4bit(0x03);		
	delay(246);					//4.1 ms delay
	inst_Ctrl_4bit(0x02);		
	delay(100);					//wait for previous inst to finish
	inst_Ctrl(0x28);			//funciton set: 4-bit; dual line
	inst_Ctrl(0x08);			//display off
	inst_Ctrl(0x01);			//display clear
	inst_Ctrl(0x06);			//entry mode: increment; shift off
	inst_Ctrl(0x0C);			//display on: cursor on; blink off
}

//adjustable delay routine
void delay(long int t)
{
	long int i;
	
	for(i=0;i<t;i++);			//1 loop is equal to 16 us
}


//reading busy flag
void read_BF()
{
	unsigned char busy_flag;
	
	LCD_DATA_IN;
	RS = 0;						//select IR
	RW = 1;						//set LCD mode to read
	E = 1;						//set enable to high
	
	do{
		busy_flag = BF;			//read busy flag
	}while(busy_flag);
	
	E = 0;
	LCD_DATA_OUT;
}


/* Ext intterupt routine */
__interrupt void reload(void)
{
	IO_TMCSR0.bit.UF = 0;	//interrupt flag cleared
	IO_PDR1.bit.P10 ^=1;				//turn on/off LED; complement current status
}

/* Vector Table */
#pragma section INTVECT,locate=0xfffc00
#pragma intvect _start			0x8 0x0 /* Reset Vector */	
#pragma intvect reload		17			//reload timer underflow interrupt vector