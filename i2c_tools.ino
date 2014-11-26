// --------------------------------------
// i2c tools for Arduino
//


#include <Wire.h>

#define DATA_LENGTH 256
#define SERIALBUFFER_LENGTH 64

#define PRINT_ONLY_ASCCI 1
#define DEBUG 0


#define CHAR_SPACE 0x100
#define CHAR_LINE 0x101
#define CHAR_X 0x102

#define COMMAND_DETECT 0
#define COMMAND_DUMP 1
#define COMMAND_GET 2
#define COMMAND_SET 3

#define MODE_PRINT_NORMAL 0
#define MODE_PRINT_SHOW_ASCCI 1

#define MODE_DETECT_FAST 0
#define MODE_DETECT_WRITE_ONLY 1
#define MODE_DETECT_READWRITE 2

#define MODE_DUMP_NORMAL 0
#define MODE_DUMP_AUTOINCREMENT 1

#define MODE_GET_NORMAL 0
#define MODE_GET_AUTOINCREMENT 1

#define MODE_SET_NORMAL 0
#define MODE_SET_WRITEREGISTERONLY 1
#define MODE_SET_SHORTWRITE 2

#define ARG_MAX_COUNT 4

#define ASCCI_NL 0x0A
#define ASCCI_CR 0x0D

int data[DATA_LENGTH + 1];
char serialbuffer[SERIALBUFFER_LENGTH + 1];

void setup()
{
	Wire.begin();

	Serial.begin(9600);
	while(!Serial)Serial.println("\nI2C Tools");
}


void loop()
{
	static int buffer_pos = 0;
	int temp = 0;

	//ARG_MAX_COUNT * (max length of number(bin == 8+2) + space) + command + space + CR + NL + (end of string)
	buffer_pos = Serial.readBytesUntil(ASCCI_NL, serialbuffer, ARG_MAX_COUNT * (10 + 1) + 1 + 1 + 1 + 1 + 1 );
    
	if(buffer_pos > 0)
	{
#if (DEBUG == 1)
                Serial.print("ECHO:");
                Serial.write((byte*)serialbuffer, buffer_pos);//
		Serial.println();                             //terminal echo
#endif		
                temp = handelCommand(serialbuffer, buffer_pos);
		if(temp != 0)
		{
#if (DEBUG == 1)
                        Serial.print("return value=");
                        Serial.println(temp);
                        Serial.println();
#endif
			printHelp();

		}
		buffer_pos = 0;
		serialFlush();
	}
	delay(100);    
}

void serialFlush()
{
	while(Serial.available() > 0) 
	{
		Serial.read();
	}
}  

//return 0 on succses
// > 0 on error
byte handelCommand(char* buffer, int buffer_length)
{
	int command, arg[ARG_MAX_COUNT + 1], arg_count = 0;
	int pos = 0, temp;

	//Find the command
	if(buffer_length <= 0) return 1;
	else if(serialbuffer[pos] == '0') command = COMMAND_DETECT;
	else if(serialbuffer[pos] == '1') command = COMMAND_DUMP;
	else if(serialbuffer[pos] == '2') command = COMMAND_GET;
	else if(serialbuffer[pos] == '3') command = COMMAND_SET;
	else return 2;
	pos++;

	if((buffer_length > 1) && !((serialbuffer[pos] == ' ') || (serialbuffer[pos] == ASCCI_NL) || (serialbuffer[pos] == ASCCI_CR)))
	{
		return 3;
	}
          
	//find all arguments and store this in arg[]
	while((pos < buffer_length) && (arg_count < ARG_MAX_COUNT))
	{
		//if ther is an new line break the loop
		if((serialbuffer[pos] == ASCCI_NL) || (serialbuffer[pos++] == ASCCI_CR))
		{
			break;
		}
		
		temp = read_number(buffer, buffer_length, pos, &(arg[arg_count]), 0xFF); //get number from string
		
		if(temp < 0) return 50 + temp * -1; //return if ther is an error
		pos = temp;
		arg_count++;
	}
	
	//if ther ar more arg. at alowet return an error
	if(arg_count > ARG_MAX_COUNT)
	{
		return 6;
	}
    
	//run the command
	switch(command)
	{
		case COMMAND_DETECT:
		{
			if(arg_count == 0) i2cdetect();
			else if(arg_count == 2) i2cdetect(arg[0], arg[1]);
			else if(arg_count == 3) i2cdetect(arg[0], arg[1], arg[2]);
			else return 7;
			break;//just for tradishen
		}
		case COMMAND_DUMP:
		{
			if(arg_count == 1) i2cdump(arg[0]);
			else if(arg_count == 3) i2cdump(arg[0], arg[1], arg[2]);
			else if(arg_count == 4) i2cdump(arg[0], arg[1], arg[2], arg[3]);
			else return 8;
			break;//just for tradishen
		}
		case COMMAND_GET:
		{
			if(arg_count == 1) i2cget(arg[0]);
			else if(arg_count == 2) i2cget(arg[0], arg[1]);
			else return 9;
			break;//just for tradishen
		}
		case COMMAND_SET:
		{
			if(arg_count == 1) i2cset(arg[0]);
			else if(arg_count == 2) i2cset(arg[0], arg[1]);
			else if(arg_count == 3) i2cset(arg[0], arg[1], arg[2]);
			else return 10;
			break;//just for tradishen
		}
		default:
		{
			return 11;
			break;//just for tradishen
		}
	}
	return 0;
}

//return new position
//< 0 on error
int read_number(char* buffer, int buffer_length, int pos, int* ret, int max)
{
	int number = 0;
	byte numberBase = 10;
	
	//remove all leadings free spaces
	while(buffer[pos] == ' ')
	{
		if(++pos >= buffer_length) return -1;  //remove all leading space or new lines
	}
	
	//run until the rail
	while(pos < buffer_length)
	{
		//if ther is an x, set base to hex
		//if ther are more than one x or b, or x are in the middel of the number, it will fail
		if(((buffer[pos] == 'x') || (buffer[pos] == 'X')) && (number == 0) && (numberBase == 10))
		{
			numberBase = 16;
		}
		//if the is an b, replace base to bin
		else if(((buffer[pos] == 'b') || (buffer[pos] == 'b')) && (number == 0) && (numberBase == 10))
		{
			numberBase = 2;
		}
		//if ther is an free space, end off string, or an new line, break the loop
		else if((buffer[pos] == ' ') || (buffer[pos] == 0) || 
			(buffer[pos] == ASCCI_NL) || (buffer[pos] == ASCCI_CR)) 
		{
			break;
		}
		//if ther is an signe outside the range, return an error
		else if((buffer[pos] < '0') || ((buffer[pos] > '9') && (buffer[pos] < 'A')) || 
			((buffer[pos] > 'F') && (buffer[pos] < 'a')) ||
			(buffer[pos] > 'f'))
		{
			return -2;
		}
		//if base the base is 2, and the is an signe over 1, return an error 
		else if((numberBase == 2) && (buffer[pos] > '1'))
		{
			return -3;
		}
		//if base the base is 10, and the is an signe over 9, return an error 
		else if((numberBase == 10) && (buffer[pos] > '9'))
		{
			return -4;
		}
		else 
		{
			number *= numberBase;
			if(buffer[pos] >= 'a')
			{
				number += buffer[pos] - 'a' + 10;
			}
			else if(buffer[pos] >= 'A')
			{
				number += buffer[pos] - 'A' + 10;
			}
			else
			{
				number += buffer[pos] - '0';
			}
			if(number > max)
			{
				return -5;
			}
		}
		pos++;
	}
	*ret = number;
	return pos;
}

void printHelp(void)
{
	Serial.println("                       i2c-tools Help Page");
	Serial.println("====================================================================");
	Serial.println("Command's :");
	Serial.println("0 = i2cdetect");
	Serial.println("1 = i2cdump");
	Serial.println("2 = i2cget");
	Serial.println("3 = i2cset");
	Serial.println();
	Serial.println("i2cdetect [start stop [mode]]");
	Serial.println("     Mode's");
	Serial.println("     0 = Fast detect, send a start condition");
	Serial.println("     1 = Send a single 0x00 on the bus");
	Serial.println("     2 = Send a single 0x00, and try to read a register");
	Serial.println();
	Serial.println("i2cdump address [start stop [mode]]");
	Serial.println("     Mode's");
	Serial.println("     0 = Read one register at once");
	Serial.println("     1 = use for devices with auto increment");
	Serial.println();
	Serial.println("i2cget address [register]");
	Serial.println();
	Serial.println("i2cset address [register [value]]");
	Serial.println();
	Serial.println("Example: '0 0x20 0x50'");
	Serial.println("Detects devicecs from addres 0x20 to 0x50");
	Serial.println("");
	Serial.println("All number kan types as bin/hex/dec, except the command number");
}

void i2cdetect(void)
{
	i2cdetect(0x03, 0x77);
}
void i2cdetect(byte start, byte stop)
{
	i2cdetect(start, stop, MODE_DETECT_FAST);
}
void i2cdetect(byte start, byte stop, byte mode)
{
	int address = 0;
	boolean detectWrite = false, detectRead = false;
	
	switch(mode)
	{
		case MODE_DETECT_FAST: break;
		case MODE_DETECT_WRITE_ONLY: detectWrite = true;break;
		case MODE_DETECT_READWRITE: detectWrite = true; detectRead = true; break;
		default:break;
	}
 
	//
	while(address < start)
	{
		data[address] = CHAR_SPACE;
		address++;
	}
 
	do 
	{
		Wire.beginTransmission(address);
		
		if(detectWrite == true)
		{
			Wire.write(0x00);
		}
		
		if(Wire.endTransmission() == 0)
		{
			if(detectRead == true)
			{
				delayMicroseconds(3);
				Wire.requestFrom(address, 1);
				if(Wire.available() > 0)
				{
					data[address] = address;
				}
				else
				{
					data[address] = CHAR_LINE;
				}
			}
			else
			{
				data[address] = address;
			}
		}
		else
		{
			data[address] = CHAR_LINE;
		}
		address++;
	} while(address <= stop);
  
	printdata(data, DATA_LENGTH, MODE_PRINT_NORMAL, stop + 1);
}

void i2cdump(byte address)
{
	i2cdump(address, 0, 0xff, MODE_DUMP_NORMAL);
}
void i2cdump(byte address, byte start, byte stop)
{
	i2cdump(address, start, stop, MODE_DUMP_NORMAL);
}
void i2cdump(byte address, byte start, byte stop, byte mode)
{
	int pos = 0;
	boolean autoincrement = false;
	boolean first_read = false;
	byte ret_value = 0;
	
	
  
	switch(mode)
	{
		case MODE_DUMP_AUTOINCREMENT: autoincrement = true; break;
		case MODE_DUMP_NORMAL:
		default: break;
	}
  
	while(pos < start)
	{
		data[pos] = CHAR_SPACE;
		pos++;
	}
  
	do
	{
		if((first_read == false) || (autoincrement == false))
		{
			Wire.beginTransmission(address);
			Wire.write(pos);
			ret_value = Wire.endTransmission();
			first_read = true;
		}
       
		if(ret_value == 0)
		{
			Wire.requestFrom((int)address, 1);
			delayMicroseconds(2);
			if(Wire.available())
			{
				data[pos] = Wire.read();
			}
			else
			{
				data[pos] = CHAR_X;
			}
		}
		else
		{
			data[pos] = CHAR_X;
		}
		pos++; 
	} while(pos <= stop);
  
	printdata(data, DATA_LENGTH, MODE_PRINT_SHOW_ASCCI, (int)stop + 1);
}

void i2cget(byte address)
{
	int ret = 0;
	
	ret = i2cget(address, 0 , MODE_GET_AUTOINCREMENT);
	
	if(ret >= 0)
	{
		Serial.print("0x");
		Serial.println(ret, HEX);
	}
	else
	{
		Serial.println("Error");
	}
}

void i2cget(byte address, byte reg_address)
{
	int ret = 0;
	
	ret = i2cget(address, reg_address , MODE_GET_NORMAL);
	
	if(ret >= 0)
	{
		Serial.print("0x");
		Serial.println(ret, HEX);
	}
	else
	{
		Serial.println("Error");
	}
}

int i2cget(byte address, byte reg_address, byte mode)
{
	int temp = 0, ret = -1;
	
	if(mode == MODE_GET_NORMAL)
	{
		Wire.beginTransmission(address);
		Wire.write(reg_address);
		temp = Wire.endTransmission();
	}
	
	if(temp == 0)
	{
		Wire.requestFrom((int)address, 1);
		delayMicroseconds(2);
		if(Wire.available())
		{
			ret = Wire.read();
		}
		else
		{
			ret = -1;
		}
	}
	return ret;
}

void i2cset(byte address)
{
	int ret;
	
	ret = i2cset(address, 0 , 0, MODE_SET_SHORTWRITE);
	
	if(ret < 0)
	{
		Serial.println("Error");
	}
}

void i2cset(byte address, byte reg_address)
{
	int ret;
	
	ret = i2cset(address, reg_address , 0, MODE_SET_WRITEREGISTERONLY);
	
	if(ret < 0)
	{
		Serial.println("Error");
	}
}

void i2cset(byte address, byte reg_address, byte value)
{
	int ret;
	
	ret = i2cset(address, reg_address , value,  MODE_SET_NORMAL);
	
	if(ret < 0)
	{
		Serial.println("Error");
	}
}

int i2cset(byte address, byte reg_address, byte value, byte mode)
{
	int ret = -1;

	Wire.beginTransmission(address);
	if((mode == MODE_SET_NORMAL) || (mode == MODE_SET_WRITEREGISTERONLY))
	{
		Wire.write(reg_address);
	}
	if(mode == MODE_SET_NORMAL)
	{
		Wire.write(value);
	}
	ret = Wire.endTransmission();
	
	return ret;
}

void printdata(int* buffer, int buffer_length)
{
	printdata(buffer, buffer_length, MODE_PRINT_NORMAL, buffer_length);
}

/*
 * fields in buffer array
 * < 0x100 will be shown as hex
 * 0x100 = '  '
 * 0x101 = '--' in hex field, '.' in ascci field
 * 0x102 = 'XX' in hex field, '.' in ascci field
 * 
 * mode 0, print hex only
 * mode 1, print hex and ascci
 */
void printdata(int* buffer, int buffer_length, byte mode, int stop)
{
	boolean print_ascci = false;
	int pos = 0;
	byte x_data_count = 0;
  
	switch(mode)
	{
		case MODE_PRINT_SHOW_ASCCI: print_ascci = true; break;
		case MODE_PRINT_NORMAL:
		default:break;
	}

	Serial.print("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	if(print_ascci == true)
	{
		Serial.print("  0123456789abcdef");
	}
	Serial.println();
  
	do
	{
		if(pos == 0x00) 
		{
			Serial.print("00: ");
		}
		else if((pos & 0x0F) == 0x00) 
		{ 
			Serial.print(pos, HEX); 
			Serial.print(": "); 
		}
    
		x_data_count = 0;
		do
		{
			if(buffer[pos] <= 0xff)
			{
				if(buffer[pos] < 0x10)
				{
					Serial.write('0');
				}
				Serial.print(buffer[pos], HEX);
			}
			else if(buffer[pos] == CHAR_LINE) 
			{
				Serial.print("--");
			}
			else if(buffer[pos] == CHAR_X) 
			{
				Serial.print("XX");
			}
			else //if(buffer[pos] == CHAR_SPACE)
			{
				Serial.write("  ");
			}
			Serial.write(' ');
			pos++;
			x_data_count++;
		} while(((pos & 0x0f) != 0x00) && (pos < buffer_length) && (pos < stop));
    
		if(print_ascci == true) 
		{
			Serial.write(' ');
			byte i = 0;
      
			while((i+x_data_count) <= 0x0f)
			{
				i++;
				Serial.print("   ");
			}
			
			do
			{
#if (PRINT_ONLY_ASCCI == 1)//ascci < 0x7f
				if((buffer[pos - x_data_count] >= 0x20) && (buffer[pos - x_data_count] < 0x7F))
				{
#else
				if((buffer[pos - x_data_count] >= 0x20) && (buffer[pos - x_data_count] != 0x7F) 
					&& (buffer[pos - x_data_count] < 0xFF))
				{
#endif
					Serial.write(buffer[pos - x_data_count]);
				}
				else
				{
					Serial.write('.');
				}
			} while(--x_data_count > 0);
		}
	Serial.println("");
	} while( (pos <= buffer_length) && (pos < stop));
}
