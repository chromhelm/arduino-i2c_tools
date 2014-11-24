// --------------------------------------
// i2c tools for Arduino
//


#include <Wire.h>

#define DATA_LENGTH 256
#define SERIALBUFFER_LENGTH 64

#define PRINT_ONLY_ASCCI

#define CHAR_SPACE 0x100
#define CHAR_LINE 0x101
#define CHAR_X 0x102

#define MODE_PRINT_NORMAL 0
#define MODE_PRINT_SHOW_ASCCI 1

#define MODE_READ_NORMAL 0
#define MODE_READ_AUTOINCREMENT 1

#define ARG_MAX_COUNT 4

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
	static boolean command_Resived = false;
int temp = 0;

        
          buffer_pos = Serial.readBytesUntil(0x0A, serialbuffer, ARG_MAX_COUNT * 11 + 3);
        
          if(buffer_pos > 0){
            //Serial.write((byte*)serialbuffer, buffer_pos);//
            //Serial.println();                             //terminal echo
          
            temp = handelCommand(serialbuffer, buffer_pos);
            if(temp != 0){
              printHelp();
              Serial.println(temp);
            }
            buffer_pos = 0;
            serialFlush();
          }
delay(100);
        
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}  

//return 0 on succses
// > 0 on error
byte handelCommand(char* buffer, int buffer_length)
{
	int command, arg[ARG_MAX_COUNT], arg_count = 0;
	int pos = 0, temp;

	if(buffer_length <= 0) return 1;
	else if(serialbuffer[pos] == '0')command = 0;
	else if(serialbuffer[pos] == '1')command = 1;
	else return 2;
        pos++;

	while((pos < buffer_length) && (arg_count < ARG_MAX_COUNT))
	{
	  //while(serialbuffer[pos] == ' ')pos++;
          if((serialbuffer[pos] == 0x0A) || (serialbuffer[pos++] == 0x0D))break;
	  temp = read_number(buffer, buffer_length, pos, &(arg[arg_count]));
	  if(temp < 0)return 4;
	  pos = temp;
	  arg_count++;
	}
	
        if(pos >= ARG_MAX_COUNT)return -5;
        
	switch(command)
        {
		case 0:{
			if(arg_count == 0)i2cdetect();
			else if(arg_count == 1)i2cdetect(arg[0]);
			else if(arg_count == 2)i2cdetect(arg[0], arg[1]);
			else return 6;
                        
		        break;
                }
		case 1:{
			if(arg_count == 1)i2cdump(arg[0]);
			else if(arg_count == 4)i2cdump(arg[0], arg[1], arg[2], arg[3]);
			else return 7;
		        break;
                }
		default:{
			return 8;
		        break;
                }
        }
	return 0;
}

//return new pos
//< 0 on error
int read_number(char* buffer, int buffer_length, int pos, int* ret)
{
	int number = 0;
	byte numberBase = 10;
	boolean baseHasChanged = false;
	
	
	while(buffer[pos] == ' '){
		if(++pos >= buffer_length) return -1;  //remove all leading space or new lines
	}
	
	while(pos < buffer_length)
	{		
		if(((buffer[pos] == 'x') || (buffer[pos] == 'X')) && (baseHasChanged == false)){
			numberBase = 16;
			baseHasChanged = true;
		}
		else if(((buffer[pos] == 'b') || (buffer[pos] == 'b')) && (baseHasChanged == false)){
			numberBase = 2;
			baseHasChanged = true;
		}
		else if((buffer[pos] == ' ') || (buffer[pos] == 0) || (buffer[pos] == 0x0A) || (buffer[pos] == 0x0D))break;
		else if((buffer[pos] < '0') || ((buffer[pos] > '9') && (buffer[pos] < 'A')) || 
			((buffer[pos] > 'F') && (buffer[pos] < 'a')) ||
			(buffer[pos] > 'f'))return -2;
		else if((numberBase == 2) && (buffer[pos] > '1'))return -3;
		else if((numberBase == 10) && (buffer[pos] > '9'))return -4;
		else {
			number *= numberBase;
			if(buffer[pos] >= 'a') number += buffer[pos] - 'a' + 10;
			else if(buffer[pos] >= 'A') number += buffer[pos] - 'A' + 10;
			else number += buffer[pos] - '0';
		}
		
		pos++;
	}
	*ret = number;
	return pos;
}

void printHelp(void)
{
  Serial.println("I2C-tools Help Page");
  Serial.println("Commands");
  Serial.println("0: i2cdetect [start [stop]]");
  Serial.println("1: i2cdump address [start stop mode]");
  Serial.println("     Mode 1: send an write for every register");
  Serial.println("     Mode 2: send on write on first register (auto-increment devices)");
}

void i2cdetect(void)
{
  i2cdetect(0x03, 0x77);
}
void i2cdetect(int start)
{
  i2cdetect(start, 0x77);
}
void i2cdetect(int start, int stop)
{
  int address = 0;
  
  while(address < start){
    data[address] = CHAR_SPACE;
    address++;
  }
  do {
    Wire.beginTransmission(address);
    if(Wire.endTransmission() == 0) {
      data[address] = address;
    }
    else {
      data[address] = CHAR_LINE;
    }
    address++;
  } while(address <= stop);
  
  printdata(data, DATA_LENGTH,MODE_PRINT_NORMAL, stop + 1);
  
  return;
}

void i2cdump(byte address)
{
  i2cdump(address, 0, 0xff, MODE_READ_NORMAL);
}
void i2cdump(byte address, int start, int stop, byte mode)
{
  int pos = 0, timer;
  boolean autoincrement = false;
  boolean first_read = false;
  byte ret_value = 0;
  
  switch(mode)
  {
    case MODE_READ_AUTOINCREMENT: autoincrement = true; break;
    case MODE_READ_NORMAL:
    default: break;
  }
  
  while(pos < start){
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
      if(Wire.available()) data[pos] = Wire.read();
      else data[pos] = CHAR_X;
    }
    else data[pos] = CHAR_X;
    pos++; 
  }while(pos <= stop);
  
  printdata(data, DATA_LENGTH, MODE_PRINT_SHOW_ASCCI, stop + 1);
}


void printdata(int* buffer, int buffer_length)
{
  printdata(buffer, buffer_length, MODE_PRINT_NORMAL, buffer_length);
}

/*
 * data filde lengt is 256
 * < 0x100 will be showen as hex
 * 0x100 = '  '
 * 0x101 = '--' in hex fild, '.' in ascci fild
 * 0x102 = 'XX' in hex fild, '.' in ascci fild
 */
//data filde min lengt is 256
//data 
//mode 0 print
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
  if(print_ascci == true)Serial.print("  0123456789abcdef");
  Serial.println();
  
  do
  {
    if(pos == 0x00) {
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
      if(buffer[pos] <= 0xff){
        if(buffer[pos] < 0x10) Serial.write('0');
        Serial.print(buffer[pos], HEX);
      }
      else if(buffer[pos] == CHAR_LINE) {
		  Serial.print("--");
	  }
	  else if(buffer[pos] == CHAR_X) {
		  Serial.print("XX");
	  }
	  else {//if(buffer[pos] == CHAR_SPACE)
		  Serial.write("  ");
	  }
      Serial.write(' ');
      pos++;
      x_data_count++;
    }while(((pos & 0x0f) != 0x00) && (pos < buffer_length) && (pos < stop));
    
    if(print_ascci == true) {
      Serial.write(' ');
      byte i = 0;
      
      while((i+x_data_count) <= 0x0f){
        i++;
        Serial.print("   ");
      }
      
      do
      {
#ifdef PRINT_ONLY_ASCCI
        if((buffer[pos - x_data_count] >= 0x20) && (buffer[pos - x_data_count] < 0x7F)){
#else
        if((buffer[pos - x_data_count] >= 0x20) && (buffer[pos - x_data_count] != 0x7F) && (buffer[pos - x_data_count] < 0xFF)){
#endif
          Serial.write(buffer[pos - x_data_count]);
        }
        else Serial.write('.');
      }while(--x_data_count > 0);
    }
    
    Serial.println("");
  }while( (pos <= buffer_length) && (pos < stop));
}
