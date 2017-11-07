/*
 --------------------------------------
 i2c tools for Arduino
 */

#include <Wire.h>

#define DEBUG 0

#define CHAR_SPACE 0x100
#define CHAR_LINE 0x101
#define CHAR_X 0x102

enum Command_t 
{
  COMMAND_DETECT,
  COMMAND_DUMP,
  COMMAND_GET,
  COMMAND_SET,
  COMMAND_HELP,
  COMMAND_SET_VAR,
  COMMAND_GET_VAR,
};
enum Variable_t 
{
  VAR_SPEED,
};
enum PrintMode_t 
{
  MODE_PRINT_NORMAL,
  MODE_PRINT_SHOW_ASCCI
};
enum ModeDetect_t 
{
  MODE_DETECT_FAST,
  MODE_DETECT_READ_ONLY,
  MODE_DETECT_WRITE_ONLY,
  MODE_DETECT_READWRITE
};
enum ModeDump_t 
{
  MODE_DUMP_NORMAL,
  MODE_DUMP_AUTOINCREMENT,
  MODE_DUMP_NORMAL_WITHOUTWRITE,
  MODE_DUMP_AUTOINCREMENT_WITHOUTWRITE
};
enum ModeGet_t 
{
  MODE_GET_NORMAL,
  MODE_GET_WRITE_READ
};
enum ModeSet_t 
{
  MODE_SET_NORMAL,
  MODE_SET_WRITEREGISTERONLY,
  MODE_SET_SHORTWRITE
};
enum PrintHexMode_t 
{
  PRINT_HEX_MODE_LEDING_ZERO = 0x1,
  PRINT_HEX_MODE_WITH_PREFIX = 0x2,
};

#define ARG_MAX_COUNT 4

#define DATA_LENGTH 256
//ARG_MAX_COUNT * (max length of number(bin == 32 + 2) + space) + command + space + CR + NL + (end of string)
#define SERIALBUFFER_LENGTH ARG_MAX_COUNT * ((32 + 2)  + 1)     + 1       + 1     + 1  + 1  + 1

int16_t data[DATA_LENGTH + 1];
char serialbuffer[SERIALBUFFER_LENGTH + 16];

void setup() 
{
  Wire.begin();  
  Serial.begin(115200);
  while(!Serial);
  printHelp();
#if (DEBUG == 1)
  Serial.print(F("SCL=")); Serial.println(digitalRead(SCL));
  Serial.print(F("SDA=")); Serial.println(digitalRead(SDA));
#endif
}

void loop() 
{
  int16_t buffer_pos = 0;
  int16_t temp = 0, i;

  buffer_pos = Serial.readBytesUntil('\n', serialbuffer, SERIALBUFFER_LENGTH - 1);

  // add null termination and trim buffer length if new line found
  i = 0;
  while(i < buffer_pos && serialbuffer[i] != '\n' && serialbuffer[i] != '\r') 
  {
    i++;
  }
  serialbuffer[i] = 0;
  buffer_pos = i;
      
  if(buffer_pos > 0) 
  {
#if (DEBUG == 1)
    //terminal echo
    Serial.print(F("ECHO: ")); Serial.write((byte*)serialbuffer, buffer_pos); Serial.println();
#endif    
    temp = handelCommand(serialbuffer, buffer_pos);
    if(temp != 0) 
    {
#if (DEBUG == 1)
      Serial.print(F("return value= ")); Serial.println(temp);
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

/*
 * expets a zero terminatet string
 * retun on success 0
 * 1 no valid command found
 * 21-24 error while converting arguments
 */
uint8_t handelCommand(char* buffer, int16_t buffer_length) 
{
  Command_t command;
  uint8_t arg_count = 0;
  int32_t arg[ARG_MAX_COUNT + 1];
  int16_t pos = 0;

  //find the command
  pos = getCommand(buffer, buffer_length, pos, &command);
  if(pos < 0) return 1;
          
  //find all arguments and store in arg[]
  pos = getArguments(buffer, buffer_length, pos, arg, &arg_count);
    
  //run the command
  RunCommand(command, arg, arg_count);
  return 0;
}

/*
 * return new position in buffer
 * or -1 on error
 */
int16_t getCommand(char* buffer, int16_t buffer_length, int16_t pos, Command_t *command) 
{
  if((buffer_length > 1) && !serialbuffer[pos] == ' ') return -1;

  if(buffer[pos] == '0')      *command = COMMAND_DETECT;
  else if(buffer[pos] == '1') *command = COMMAND_DUMP;
  else if(buffer[pos] == '2') *command = COMMAND_GET;
  else if(buffer[pos] == '3') *command = COMMAND_SET;
  else if((buffer[pos] == 'h') || (serialbuffer[pos] == 'H')) *command = COMMAND_HELP;
  else if((buffer[pos] == 's') || (serialbuffer[pos] == 'S')) *command = COMMAND_SET_VAR;
  else if((buffer[pos] == 'g') || (serialbuffer[pos] == 'G')) *command = COMMAND_GET_VAR;
  else return -1;
  pos++;

#if (DEBUG == 1)
  Serial.print(F("Command= ")); Serial.println(*command);
#endif
  return pos;
}

/*
 * return new position in buffer
 * or -1 on error
 */
int16_t getArguments(char* buffer, int16_t buffer_length, int16_t pos, int32_t *arg, uint8_t *argc) 
{
  *argc = 0;
  while(pos < buffer_length && *argc < ARG_MAX_COUNT && buffer[pos]) 
  {
    pos = read_number(buffer, buffer_length, pos, &(arg[*argc])); //get number from string
    Serial.print("read number ret:");
    Serial.println(pos);
    if(pos < 0)
    {
      return -1; //return if there is an error
    }
    (*argc)++;
  }
  
#if (DEBUG == 1)
  Serial.print(F("arg count= "));
  Serial.println(*argc);
  
  for(int16_t i = 0; i < *argc;i++) 
  {
          Serial.print(F("arg["));
          Serial.print(i);
          Serial.print(F("]= "));
          Serial.println(arg[i]);
  }
#endif

  //if there to many arg. return an error
  if(*argc > ARG_MAX_COUNT) 
  {
    return -1;
  } 
  else 
  {
  return pos;
  }
}

int16_t RunCommand(Command_t command, int32_t *arg, int8_t argc) 
{
  switch(command) 
  {
    case COMMAND_DETECT: 
    {
      if(argc != 0 && argc != 2 && argc != 3) return -1;
      if(argc >= 2 && !ValidateNumber(arg[0], 0, 255) && ValidateNumber(arg[1], 0, 255) && arg[0] > arg[1]) return -1;
      if(argc == 3 && !ValidateNumber(arg[2], MODE_DETECT_FAST, MODE_DETECT_READWRITE)) return -1;
      
      if(argc == 0) i2cdetect();
      else if(argc == 2) i2cdetect(arg[0], arg[1]);
      else if(argc == 3) i2cdetect(arg[0], arg[1], (ModeDetect_t)arg[2]);
      break;
    }
    case COMMAND_DUMP: 
    {
      if(argc != 1 && argc != 3 && argc != 4) return -1;
      if(!ValidateNumber(arg[0], 0, 255)) return -1;
      if(argc >= 3 && !ValidateNumber(arg[1], 0, 255) && !ValidateNumber(arg[2], 0, 255)) return -1;
      if(argc == 4 && !ValidateNumber(arg[3], MODE_DUMP_NORMAL, MODE_DUMP_AUTOINCREMENT_WITHOUTWRITE)) return -1;
      
      if(argc == 1) i2cdump(arg[0]);
      else if(argc == 3) i2cdump(arg[0], arg[1], arg[2]);
      else if(argc == 4) i2cdump(arg[0], arg[1], arg[2], (ModeDump_t)arg[3]);
      break;
    }
    case COMMAND_GET: 
    {
      if(argc != 1 && argc != 2) return -1;
      if(!ValidateNumber(arg[0], 0, 255)) return -1;
      if(argc == 2 && !ValidateNumber(arg[1], 0, 255)) return -1;

      if(argc == 1) i2cget(arg[0]);
      if(argc == 2) i2cget(arg[0], arg[1]);
      break;
    }
    case COMMAND_SET: 
    {
      if(argc != 3 ) return -1;
      if(!ValidateNumber(arg[0], 0, 255) && 
         !ValidateNumber(arg[1], 0, 255) && 
         !ValidateNumber(arg[2], 0, 255)) return -1;
      
      if(argc == 3) i2cset(arg[0], arg[1], arg[2]);
      break;
    }
    case COMMAND_HELP: 
    {
      printHelp();
      break;
    }
    case COMMAND_SET_VAR: 
    {
      if(argc != 2) return -1;
      if(!ValidateNumber(arg[0], VAR_SPEED, VAR_SPEED)) return -1;
      if(arg[0] == VAR_SPEED && !ValidateNumber(arg[1], 0, 400000)) return -1;
      
      if(argc == 2) VarSet((Variable_t)arg[0], arg[1]);
      break;
    }
    case COMMAND_GET_VAR: 
    {
      if(argc != 1) return -1;
      if(!ValidateNumber(arg[0], VAR_SPEED, VAR_SPEED)) return -1;
    
      if(argc == 1) VarPrint((Variable_t)arg[0]);
      break;
    }
    default: 
    {
      return -1;
    }
  }
  return 0;
}

bool ValidateNumber(int32_t number, int32_t min, int32_t max) 
{
  return number >= min && number <= max;
}
/*
 * return new position
 * on error return
 * -1 end of buffer reached without number found
 * -2 a not alowed charagter in buffer, Allowed sign are [0-9]&[a-f]&[A-F]  and space
 * -3 if base is 2 and found a sign witch is not 0 1
 * -4 if base is 10 and found a sign witch is not[0-9]
 */

int16_t read_number(char* buffer, int16_t buffer_length, int16_t pos, int32_t* number) 
{
  uint8_t numberBase = 10;
  *number = 0;
  
  //remove all leadings free spaces
  while(buffer[pos] == ' ') 
  {
    if(++pos >= buffer_length) return -1;  //remove all leading space or new lines
  }
  
  while(pos < buffer_length) 
  {
    //if there is an x, set base to hex
    //if there are more than one x or b, or x are in the middel of the number, it will fail
    if(*number == 0 && numberBase == 10 && (buffer[pos] == 'x' || buffer[pos] == 'X')) numberBase = 16;
    else if(*number == 0 && numberBase == 10 && (buffer[pos] == 'b' || buffer[pos] == 'b')) numberBase = 2;
    
    // if there is an free space, end off string, or an new line, break the loop
    else if((buffer[pos] == ' ') || (buffer[pos] == 0) || (buffer[pos] == '\n') || (buffer[pos] == '\r')) break;
    
    // if there is an signe outside the range, return an error
    else if(!isHexadecimalDigit(buffer[pos])) return -2;
    
    // if base the base is 2, and the is an signe over 1, return an error 
    else if((numberBase == 2) && (buffer[pos] > '1'))  return -3;
    
    // if base the base is 10, and the is an signe over 9, return an error 
    else if((numberBase == 10) && (buffer[pos] > '9')) return -4;

    // base 16 character are check implice before
    else 
    {
      *number *= numberBase;
      char c = buffer[pos]; 
      if(c >= 'A'/* || c >= 'a'*/) c -= 7;
      *number += c & 0x0f;
    }
    pos++;
  }
  return pos;
}

void printHelp(void) 
{
  Serial.println(F("                       i2c-tools Help Page"));
  Serial.println(F("===================================================================="));
  Serial.println(F("Command's :"));
  Serial.println(F("0 = i2cdetect"));
  Serial.println(F("1 = i2cdump"));
  Serial.println(F("2 = i2cget"));
  Serial.println(F("3 = i2cset"));
  Serial.println(F("g = get a variable"));
  Serial.println(F("s = set a variable"));
  Serial.println(F("h = show this help page"));
  Serial.println();
  Serial.println(F("i2cdetect [start stop [mode]]"));
  Serial.println(F("     Mode's:"));
  Serial.println(F("     0 = Fast detect, send a start condition"));
  Serial.println(F("     1 = Just try to read"));
  Serial.println(F("     2 = Send a single write 0x00 on the bus"));
  Serial.println(F("     3 = Send a single write 0x00, and try to read a register"));
  Serial.println();
  Serial.println(F("i2cdump address [start stop [mode]]"));
  Serial.println(F("     Mode's:"));
  Serial.println(F("     0 = Read one register at once"));
  Serial.println(F("     1 = Use for devices with auto increment"));
  Serial.println(F("     2 = Read one register at once, Do not send write comand"));
  Serial.println(F("     3 = Use for devices with auto increment, Do not send write command"));
  Serial.println();
  Serial.println(F("i2cget address [register]"));
  Serial.println(F("i2cset address register value"));
  Serial.println();
  Serial.println(F("Example: '0 0x20 0x50'"));
  Serial.println(F("Detects devicecs from addresses from 0x20 to 0x50"));
  Serial.println();
  Serial.println(F("get var"));
  Serial.println(F("set var value"));
  Serial.println(F("     Variables:"));
  Serial.println(F("     0 = clk speed 1 - 400000"));
  Serial.println();
  Serial.println(F("All numbers can types as bin/hex/dec, except the command number"));
  Serial.println();
  Serial.println(F("Set your terminal to 'LF' line ending"));
}

void VarSet(Variable_t var, int32_t value) 
{
  switch(var) 
  {
    case VAR_SPEED: 
    {
      //testet on Atmel328p
//      TWPS1 TWPS0 Prescaler Value 
//      0     0     1 
//      0     1     4
//      1     0     16 
//      1     1     64 
//      
//                       CPU Clock frequency
//      SCL frequency = -----------------------
//                       16 + 2(TWBR) ⋅ 4^TWPS
      
      
      uint8_t divider = 0;
      uint16_t speed;
      
      speed = ((F_CPU / value) - 16) / 2;
      
      while(speed >= 256) 
      {
        speed /= 4;
        divider++;;
      }
      
      TWBR = speed;
      if((divider & 0x01) != (TWSR & (1 << TWPS0))) TWSR ^= 1 << TWPS0;
      if((divider & 0x02) != (TWSR & (1 << TWPS1))) TWSR ^= 1 << TWPS1;
      break;
    }
    default:break;
  }
}
int32_t VarGet(Variable_t var) 
{
  switch(var) 
  {
    case VAR_SPEED: 
    {
#if (DEBUG == 1)
      Serial.print(F("TWBR")); Serial.println(TWBR);
      Serial.print(F("TWSR & 3")); Serial.println(TWSR & 0x03);
#endif
      return (F_CPU / (16 + 2*TWBR * pow(4, TWSR & 0x03 )));
    }
    default:
      return 0;
  }
}
void VarPrint(Variable_t var) 
{
  switch(var) 
  {
    case VAR_SPEED: 
    {
      Serial.print(VarGet(var) / 1000); Serial.println(F("kHz"));
      break;
    }
    default:break;
  }
}

void i2cdetect(void) 
{
  i2cdetect(0x03, 0x77);
}
void i2cdetect(uint8_t start, uint8_t stop) 
{
  i2cdetect(start, stop, MODE_DETECT_FAST);
}
void i2cdetect(uint8_t start, uint8_t stop, ModeDetect_t mode) 
{
  uint16_t address = 0;
  boolean detectWrite = false, detectRead = false;
  int16_t ret = -1;
  
  switch(mode) 
  {
    case MODE_DETECT_FAST:        detectWrite = false; detectRead = false; break;
    case MODE_DETECT_READ_ONLY:   detectWrite = false; detectRead = true; break;
    case MODE_DETECT_WRITE_ONLY:  detectWrite = true;  detectRead = false; break;
    case MODE_DETECT_READWRITE:   detectWrite = true;  detectRead = true; break;
    default:break;
  }
 
  //
  while(address < start) 
  {
    data[address] = CHAR_SPACE;
    address++;
  }
 
  do {
#if (DEBUG == 1)
    Serial.print(F("Begin transmistion:")); Serial.println(address);
#endif
    Wire.beginTransmission((uint8_t)address);
    if(detectWrite == true) 
    {
      Wire.write(0x00);
    }
    ret = Wire.endTransmission();
    
    if(ret == 0) 
    {
      if(detectRead == true) 
      {
        delayMicroseconds(1000);
        Wire.requestFrom(address, 1);
        delayMicroseconds(1000);
        if(Wire.available() > 0) 
        {
          while(Wire.available())
            Wire.read();
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
  } while(++address <= stop);
  
  printdata(data, DATA_LENGTH, MODE_PRINT_NORMAL, stop);
}

void i2cdump(uint8_t address) 
{
  i2cdump(address, 0, 0xff, MODE_DUMP_NORMAL);
}
void i2cdump(uint8_t address, uint8_t start, uint8_t stop) 
{
  i2cdump(address, start, stop, MODE_DUMP_NORMAL);
}
void i2cdump(uint8_t address, uint8_t start, uint8_t stop, ModeDump_t mode) 
{
  int16_t pos = 0;
  boolean autoincrement = false;
  boolean write_reg = true;
  uint8_t ret_value = 0;
  
  switch(mode) 
  {
    case MODE_DUMP_NORMAL:                      autoincrement = false;  write_reg = true;  break;
    case MODE_DUMP_AUTOINCREMENT:               autoincrement = true;   write_reg = true;  break;
    case MODE_DUMP_NORMAL_WITHOUTWRITE :        autoincrement = false;  write_reg = false; break;
    case MODE_DUMP_AUTOINCREMENT_WITHOUTWRITE:  autoincrement = true;   write_reg = false; break;
    default: break;
  }
  
  while(pos < start) 
  {
    data[pos] = CHAR_SPACE;
    pos++;
  }
  
  if(autoincrement == false) 
  {
    do 
    {
      if(write_reg == true)
      {
        Wire.beginTransmission(address);
        Wire.write(pos);
        ret_value = Wire.endTransmission();
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
  }
  else if(mode == MODE_DUMP_AUTOINCREMENT) 
  {
    if(write_reg == true)
    {
      Wire.beginTransmission(address);
      Wire.write(pos);
      ret_value = Wire.endTransmission();
    }
    
    if(ret_value == 0) 
    {
      Wire.requestFrom((int)address, stop - start);
      delayMicroseconds(2);
      while(Wire.available())
      {
        data[pos] = Wire.read();
        pos++;
      }
    }
    
    while(pos < stop) 
    {
      data[pos] = CHAR_X;
      pos++; 
    }
  }
  
  printdata(data, DATA_LENGTH, MODE_PRINT_SHOW_ASCCI, stop);
}

void i2cget(uint8_t address) 
{
  int16_t ret = 0;
  
  ret = i2cget(address, 0 , MODE_GET_NORMAL);
  
  if(ret >= 0) 
  {
    printHex(ret, (PrintHexMode_t)(PRINT_HEX_MODE_LEDING_ZERO | PRINT_HEX_MODE_WITH_PREFIX));
    Serial.println();
  } 
  else 
  {
     Serial.print(F("Error:")); Serial.println(ret);
  }
}
void i2cget(uint8_t address, uint8_t reg_address) 
{
  int16_t ret = 0;
  
  ret = i2cget(address, reg_address , MODE_GET_WRITE_READ);
  
  if(ret >= 0) 
  {
    printHex(ret, (PrintHexMode_t)(PRINT_HEX_MODE_LEDING_ZERO | PRINT_HEX_MODE_WITH_PREFIX));
    Serial.println();
  } 
  else 
  {
    Serial.print(F("Error:")); Serial.println(ret);
  }
}
int16_t i2cget(uint8_t address, uint8_t reg_address, ModeGet_t mode) 
{
  int16_t temp = 0, ret = -1;
  
  if(mode == MODE_GET_WRITE_READ) 
  {
    Wire.beginTransmission(address);
    Wire.write(reg_address);
    temp = Wire.endTransmission();
  }
  
  if((mode == MODE_GET_WRITE_READ && temp == 0) || mode == MODE_GET_NORMAL) 
  {
    Wire.requestFrom((int)address, 1);
    delayMicroseconds(2);
    if(Wire.available()) 
    {
      ret = Wire.read();
    }
    else 
    {
      ret = -5;
    }
  }
  else
  {
    ret = -temp;
  }
  return ret;
}

void i2cset(uint8_t address, uint8_t reg_address, uint8_t value) 
{
  int16_t ret;
  
  ret = i2cset(address, reg_address , value,  MODE_SET_NORMAL);
  
  if(ret < 0) 
  {
    Serial.print(F("Error")); Serial.println(ret);
  }
}
int16_t i2cset(uint8_t address, uint8_t reg_address, uint8_t value, ModeSet_t mode) 
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

void printdata(int16_t* buffer, int16_t buffer_length) 
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
void printdata(int16_t* buffer, int16_t buffer_length, PrintMode_t mode, uint8_t stop) 
{
  boolean print_ascci = false;
  int16_t pos = 0;
  uint8_t x_data_count = 0;
  
  switch(mode) 
  {
    case MODE_PRINT_SHOW_ASCCI: print_ascci = true; break;
    case MODE_PRINT_NORMAL:
    default:break;
  }

  // print header
  Serial.print(F("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F"));
  if(print_ascci == true) 
  {
    Serial.print(F("  0123456789abcdef"));
  }
  Serial.println();

  do 
  {
    // if new line print row header
    if((pos & 0x0F) == 0x00) 
    {
      printHex(pos, PRINT_HEX_MODE_LEDING_ZERO);
      Serial.print(F(": "));
    }
        
    x_data_count = 0;
    do 
    {
      if(buffer[pos] <= 0xff) 
      {
        printHex(buffer[pos], PRINT_HEX_MODE_LEDING_ZERO);
      }
      else if(buffer[pos] == CHAR_LINE) 
      {
        Serial.print(F("--"));
      }
      else if(buffer[pos] == CHAR_X) 
      {
        Serial.print(F("XX"));
      }
      else /*if(buffer[pos] == CHAR_SPACE)*/ 
      {
        Serial.print(F("  "));
      }
      Serial.write(' ');
      pos++;
      x_data_count++;
    } while(((pos & 0x0f) != 0x00) && (pos < buffer_length) && (pos <= stop));
    
    if(print_ascci == true) 
    {
      Serial.write(' ');
      uint8_t i = 0;
      
      while((i+x_data_count) <= 0x0f) 
      {
        i++;
        Serial.print(F("   "));
      }
      
      do 
      {
        if((buffer[pos - x_data_count] >= 0x20) && (buffer[pos - x_data_count] != 0x7F) 
          && (buffer[pos - x_data_count] < 0xFF)) 
          {
          Serial.write(buffer[pos - x_data_count]);
        } 
        else 
        {
          Serial.write('.');
        }
      } while(--x_data_count > 0);
    }
  Serial.println();
  } while( (pos <= buffer_length) && (pos <= stop));
}
void printHex(uint8_t value, PrintHexMode_t mode) 
{
   if(mode & PRINT_HEX_MODE_WITH_PREFIX) Serial.print(F("0x"));
   if(mode & PRINT_HEX_MODE_LEDING_ZERO && value < 0x10) 
   { 
         Serial.write('0');
  }
  Serial.print(value, HEX);
}
