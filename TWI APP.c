/*
 * TWI_APP.c
 *
 * Created: 2016-02-20 3:24:27 PM
 *  Edited: 2018-01-12 (Mostly to provide up-to-date commentary)
 *  Author: Timothy Schoonhoven
 *  Author's Notes:  Code copied and slightly modified from:http://www.embedds.com/programming-avr-i2c-interface/
 *  Description:  This chunk of C code is a form embedded software meant to act as a communication protocol.  In this case,
 *  the two wire interface (TWI), where the controller will have several slaves on a single line.  It was designed for the Captstone
 *  Mars Rovers project, and has been placed on this git hub to help demonstrate my ability for clean, concise comments.
 
 *  NOTES FROM PREVIOUS EDITING:
 *  -  Not yet sure how to check the error registration not now.  
 *  -  Program should set the sensor readings to to 32 amp range.  16 amp by default
 *	-  Small functions are simply cogs for the bigger function.  Large function provide the read and write sequences
 *	-  Be sure to check the slave address you're referring to.  For reference, check what resistor values on pints A1 and A0 equate
 *     to which slave address
 *  -  Slave address should be the first argument in either CSread or CSWrite.
 *		PLEASE READ!!!!!:  This program has been updated to use the TWI code inside an interrupt vector, instead of using the
 *							while ((TWCR & (1<<TWINT)) == 0); command in each small function.  This is so the controller doesn't
 *							get hung up while waiting for a response from the devices it's communicating with via TWI (I2C).
 *							Initially, the program should start the send process in the main program, and afterwards, everything
 *							is being handled by the interrupt function below.
 *						
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//For storing data obtained from ACS.  
uint8_t *u8data1;
uint8_t *u8data2;
uint8_t *u8data3;
volatile char current;

volatile char Twi_Flag;

//Start signal function.  Used to start transmission
void TWIStart(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
}

//Stop signal function.  Tells transmission to end
void TWIStop(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

//Function for writing data.  NOTE:  TWEA may not be needed as '1'
void TWIWrite(uint8_t u8data)
{
	TWDR = u8data;
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while ((TWCR & (1<<TWINT)) == 0);
}

//Function sends read and Transmits ACK
uint8_t TWIReadACK(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

//Function reads byte with NACK
uint8_t TWIReadNACK(void)  
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

//Status conversion functions.  Masks specific bits so others are read from TWSR.  For error checking
uint8_t TWIGetStatus(void)  
{
	uint8_t status;
	
	//mask status
	status = TWSR & 0xF8;
	return status;
}

//NOTE:  Below function has been altered, with original code commented out.  
//       May need double checking, but otherwise, it simply takes in address and data
//Function is to send a whole write stream from master to slave
//Transmission Flow: Start>Slave_Address>Slave_Register>Data write>Stop
uint8_t CSWriteByte(uint8_t slave_addr, uint8_t slave_reg, uint8_t u8data)
{
	//STAT WRITING!!!!!
	TWIStart();
	
	//if (TWIGetStatus() != 0x08)
	//return ERROR;
	
	//Send A3 A2 A1 A0 address bits
	TWIWrite(slave_addr);
	
	/*if (TWIGetStatus() != 0x18)
	return ERROR;*/
	
	//write byte ACS register
	TWIWrite(slave_reg);
	/*if (TWIGetStatus() != 0x28)
	return ERROR;*/
	
	//write data to register
	TWIWrite(u8data);
	
	TWIStop();
	//STOP WRITING!!!!!
	//return SUCCESS;
}

//Function for making one whole 'read' session.  
//Transmission Flow:  START>write slave addr>slave Reg>START (Again)>slave addr>read dat>ACK>Read>ack>read>NACK>STOP!!
uint8_t CSReadByte(uint8_t slave_addr, uint8_t slave_reg)
{
	uint8_t current;
	//START READING.  First send write for slave and register address
	TWIStart();
	if (TWIGetStatus() != 0x08)
	//return ERROR;
	
	//Send SLAVE A3 A2 A1 A0 address bits
	TWIWrite(slave_addr);
	/*if (TWIGetStatus() != 0x18)
	return ERROR;*/
	
	//Send Register Address
	TWIWrite(slave_reg);
	
	//RESTART MASTER
	TWIStart();
	/*if (TWIGetStatus() != )
	return ERROR;*/
	
	//Send Address again
	TWIWrite(slave_addr);
	/*if (TWIGetStatus() != slave_addr)
	return ERROR;*/
	
	//Read data in bytes.  3 bytes are sent in a row.  Last byte is current data
	u8data1 = TWIReadACK();  //First byte
	u8data2 = TWIReadACK();  //Second Byte
	current = TWIReadNACK();  //Third Byte (Current)
	
	/*if (TWIGetStatus() != 0x58)
	return ERROR;*/
	TWIStop();
	return current;
}

void led_blink_count(int cc)//This code is for simulation testing
{
	for (int i=0; i<=cc; i++)
	{
		PORTB|=(1<<PORTB0);
		_delay_ms(500);
		PORTB&= ~(1<<PORTB0);
		_delay_ms(500);
	}
}




int main(void)
{
	char sensorArray [6];  //Array for sensors data
	DDRB|=((1<<PINB0)|(1<<PINB1)|(1<<PINB2));
	float currentConv;
	//Initialize ACS
	//Write to register for 32A range (Slave address, slave reg, data)
	//CSWriteByte(0x00, 0x04, 0x00);
    while(1)
    {
       current = CSReadByte(0x00, 0x00);  //Reads current from sensor.  
	   currentConv = current*0.1255;
	   
	   //This checks if there's actual sensor data.  Will light up hardware LED for confirmation
	   if ((currentConv>0)&&(currentConv<1))
			{
			led_blink_count(2); 
			}
	   
	   if ((currentConv>1))
	   	   {
		   	   led_blink_count(5);
	   	   }
	   
    }
}

//Interrupt Function
ISR (TWINT)
{
	switch (Twi_Flag)
	{
		case (0)://Starting point
			TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
			Twi_Flag++;
		break;
		
		case (1): //Address
			TWDR = 0x00;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
			Twi_Flag++;
		break;
		
		case (2): //Register
			TWDR = 0x00;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
			Twi_Flag++;
		break;
		
		case (3): //Restart
			TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
			Twi_Flag++;
		break;
		
		case (4): //Slave Adress
			TWDR = 0x00;
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
			Twi_Flag++;
		break;
		
		case (5): //Read first line (Dud)
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
			Twi_Flag++;
		break;
		
		case (6): //Read second line (Also DUD)
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
			Twi_Flag++;
		break;
		
		case (7): //Read last byte (Needed)
			TWCR = (1<<TWINT)|(1<<TWEN);
			current=TWDR;
			Twi_Flag++;
		break;
		
		case (8)://Stop
			TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
			Twi_Flag++;
		break;
		
		default:
			Twi_Flag=0;
		break;
		
	}
		
}