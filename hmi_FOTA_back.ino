#include <SD.h>
#include <SPI.h>
#include "stdio.h"
#include "stdint.h"
#include <SoftwareSerial.h>
#include "stdlib.h"

#define MAX_LINE_SIZE 50
int address, addressCount = 0;
uint16_t addressArray[5]={0};
const int chipSelect = 53;
bool reading_complete, initiate_transmission_flag = false; 
uint16_t byteCounts[5]={0};
bool startFlag, transmit_flag,calculate_lines_flag = false;
bool memory_write_flag = false; 
char currentChar;
bool isAddress = false;
int byteCount, dummy_address_counter = 0;
int counter,i,j,c,d,address_counter_fetch_hex = 0;
int values = 0;
bool looper = false;
bool dumboReceived = false;
char hex[2];
bool fetch_hex_complete = false; 
bool packet_not_received, fetch_hex_while_loop_end_flag = false;
bool address_fetched = false; 
uint8_t packet_hex_counter = 0;
int calculate_lines_counter, ack_counter, pointer_A, pointer_B, dumbo_counter = 0; 
bool parser_runner_flag  = false;
bool free_flag = false; 
//SoftwareSerial stm32(2, 3); //RX,TX
SoftwareSerial nextion(10, 11); //RX,TX

File tiTxtFile,tiTxtFile2,tiTxtFile3,tiTxtFile4,tiTxtFile5;
struct LineCalculationResult
{
  int* number_lines_per_address; 
  int* number_values_last_line;
  int address; 
};
struct LineCalculationResult calc_result;

struct HexData {
  uint8_t* data_packet;
  int length;
};
struct HexData hexdata;
struct fetched_data {
    int* fetched_data_memory_write;
};
void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
  // put your setup code here, to run once:
    Serial3.begin(115200);
    Serial.begin(115200);
    nextion.begin(115200);
    pinMode(chipSelect, OUTPUT);
    digitalWrite(chipSelect, LOW);
 
  if (!SD.begin(chipSelect)) {
      Serial.println("SD card initialization failed!");
      return;
    }
    //readAndPrintHexFile("APP_SI~1.HEX");
     tiTxtFile = SD.open("APP_SI~1.TXT", FILE_READ);
      if (!tiTxtFile) {
          Serial.println("Error opening file");
          return;
      }
      // Display TI-TXT data on Serial monitor
     displayTiTxtFile(tiTxtFile);
     tiTxtFile.close();
     tiTxtFile2 = SD.open("APP_SI~1.TXT", FILE_READ); 
     printAddress(tiTxtFile2);
     tiTxtFile2.close();
     tiTxtFile3 = SD.open("APP_SI~1.TXT", FILE_READ); 
     extractedDatasize(tiTxtFile3);
     
     tiTxtFile3.close();
     tiTxtFile4 = SD.open("APP_SI~1.TXT", FILE_READ); 
     extractedDatasize(tiTxtFile4);
     tiTxtFile4.close();   
     reading_complete = true; 
     tiTxtFile5 = SD.open("APP_SI~1.TXT", FILE_READ);
     calc_result = calculate_lines(addressCount, byteCounts); // this went global once. 
     
      /*Serial.println("Number of lines per address:");
      for (int i = 0; i < addressCount; i++) {
        Serial.print(calc_result.number_lines_per_address[i]);
        Serial.print(" ");
      }
      Serial.println();
         Serial.println("Number of values in the last line:");
      for (int i = 0; i < addressCount; i++) {
        Serial.print(calc_result.number_values_last_line[i]);
        Serial.print(" ");
      }
      Serial.println();
      */
      free(calc_result.number_lines_per_address);
      free(calc_result.number_values_last_line);
      initiate_transmission_flag = true; 
     
}

void loop() {
  while (nextion.available()>= 5 ) {
    // Wait for data
    char receivedChars[6];
    //Serial.print("Marty");
    nextion.readBytes(receivedChars, 5);   
    receivedChars[5] = '\0'; // Null-terminate the string
    if (strcmp(receivedChars, "START") == 0) {
      // Set the flag to indicate the start command
      startFlag = true;      
      //transmit_flag = true;
      Serial.println("9011");      
      //stm32.println("9011");
     
    }else 
    {
      startFlag = false;
    }    
  }
  if(startFlag == true)
  {
    initiate_transmission_protocol(); 
    //delay(100);
  }  
   int* data = fetch_hex(tiTxtFile5);
    if (fetch_hex_complete) {
      
      fetch_hex_complete = false; 
      print_memory(data);
      free(data);
      //free(calc_result.number_lines_per_address);
      //free(calc_result.number_values_last_line);
      delay(30);
    }
}
/*
 * This function prints comlete line till \n. Nothing else. 
 * Not useful in actual context of COMM protocol.. 
 * 
 */
void readAndPrintHexFile(const char* filename)
{
  File file = SD.open(filename);
  if (file) {
    //Serial.println("Reading file:");
    while (file.available()) {
      // Read a line from the file
      String line = file.readStringUntil('\n');
      // Print the line on the Serial Monitor
      //Serial.println(line);
    }
   file.close();
    //Serial.println("File closed.");
  } else {
    // If the file couldn't be opened, print an error message
    //Serial.println("Error opening file!");
  }
}

/*
 * This function reading one character at a time and printing it on Serial Monitor
 * Other than this of character is '@' then addressCount counter is getting
 * incremented. 
 * This to count how many addresses are there in file.
 */
void displayTiTxtFile(File &inputFile) {
    char line[100]; // Adjust the buffer size as needed

    // Read one character at a time and display on Serial monitor
    while (inputFile.available()) {
        char c = inputFile.read();
        //Serial.write(c);
        if(c == '@')
        {
          addressCount++;
        }
    }
    //Serial.println();
    //Serial.print("address count:");
    //Serial.println(addressCount);
}

/*
 * This function is reading total file and extracting addresses and storing them
 * in addressArray. 
 * Size of this array is decided by addressCount which is calculated in previous
 * function. 
 */
int printAddress(File &inputFile) {

    addressArray[addressCount]={0};
    char line[MAX_LINE_SIZE];
    char currentChar;
    int index = 0;
    int lineIndex = 0; 
     while (inputFile.available() && index < MAX_LINE_SIZE - 1) 
     {
        currentChar = inputFile.read();
        if (currentChar == '\n' || currentChar == '\r') {
            // Stop reading when newline or carriage return is encountered
            // Null-terminate the array
            line[index] = '\0';
            // Process the line to extract addresses
            if (line[0] == '@') {
                // If line starts with '@', extract the 16-bit address
                uint16_t address = strtol(line + 1, NULL, 16);
                // Save the address in the array
                if(lineIndex<addressCount)
                {
                  addressArray[lineIndex++] = address;
                  //debug:
                  //Serial.print("Address: 0x");
                  //Serial.println(address, HEX);                
                }
            } else 
            {
                // If line does not start with '@', process the data
                //Serial.println("Data: " + String(line));
            }
            // Reset the index for the next line
            index = 0;
        } else 
        {
            // Store the character in the array
            line[index++] = currentChar;
        }
    }
    // Print the extracted addresses
    /*Serial.println("Extracted Addresses:");
    for (int i = 0; i < lineIndex; i++) {
        Serial.print("0x");
        Serial.println(addressArray[i], HEX);
        
    }
    Serial.print("***********************************");
    Serial.println();*/ 
}

/*
 * This function calculates how many bytes are present under each address section. 
 * this numbers are stores in array byteCounts array. 
 */
void extractedDatasize(File &myFile)
{     
    char currentChar;
    bool isAddress = false;   
    int byteCount = 0;
    int counter = 0;
    
    //Serial.print("Address Count is : ");
    //Serial.println(addressCount); 
    while (myFile.available()) {
        currentChar = myFile.read();

        if (currentChar == '@') {
            // Print the previous memory address and byte count
            if (isAddress) {
              byteCounts[counter++] = byteCount / 2; // Adjusted to divide by 2
              //debug:
                //Serial.println();
                //Serial.print("Number of Bytes: ");
                //Serial.println(byteCount / 2);  // Adjusted to divide by 2
            }
            // Read the next 4 characters as the address
            char hexAddress[5];
            myFile.readBytesUntil('\n', hexAddress, sizeof(hexAddress));
            address = strtol(hexAddress, NULL, 16);
            
            // Print the new memory address
            //Serial.print("Memory Address: ");
            //Serial.println(address, HEX);
            // Reset byte count
            byteCount = 0;
            isAddress = true;
        } else if (isAddress && isHexDigit(currentChar)) {
          //debug
            // Print the current character
            //Serial.print(currentChar);
            //stm32.print(currentChar);
            byteCount++;
             //data[byteCount - 1] = hexToByte(currentChar);
             //delay(200);
        }
    }
    // Print the byte count for the last memory address
    if (isAddress && counter < addressCount)
    {
        byteCounts[counter++] = byteCount / 2; // Adjusted to divide by 2
        //debug:
        //Serial.println();
        //Serial.print("Number of Bytes: ");
        //Serial.println(byteCount / 2);  // Adjusted to divide by 2
        for(int i = 0; i < addressCount; i++)
        {      
          //Serial.println(byteCounts[i]);
        }
    }
    //debug:
    //Serial.print("***********************************");
    //Serial.println();
}

/*
 * This function checks incoming byte is hex or not.
 */
bool isHexDigit(char c) {
    
    bool a =  (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    //Serial.print("a:"); 
    //Serial.println(a); 
    return a; 
}

/*
 * This function converts incoming byte to hex and returns the same. 
 */
uint8_t hexToByte(char hex)
{
    if (hex >= '0' && hex <= '9')
        return hex - '0';
    else if (hex >= 'A' && hex <= 'F')
        return hex - 'A' + 10;
    else if (hex >= 'a' && hex <= 'f')
        return hex - 'a' + 10;
    else
        return 0; // Handle error, non-hex character
}

/*
 * This function calculate CRC16 and returns the value. 
 */
uint16_t calculateCRC16(const uint8_t *data, size_t length)
{
  uint16_t crc = 0xFFFF; 
  for(size_t i = 0; i<length; ++i)
  {
    crc ^= data[i];
    for (size_t j = 0; j < 8; ++j) {
      crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
    }
  }
  return crc;
}

/*
 * this is old function and kept for reference
 * Its not in use for actual protocol. 
 * This function appends CRC value to the data and sends on STM32
 * UART channel. 
 */
void sendDataWithCRC(Stream &stm32, uint8_t data, size_t length)
{ 
  // Send "START32" command
  //stm32.println("START32");
  
  int command = stm32.parseInt();
  // Check if the received command is "100"
  if (1) {
    // Calculate CRC of the data
   uint16_t crc = calculateCRC16(&data, length);
    // Send data packet
    stm32.write(data);
    // Send CRC as hexadecimal string
    stm32.write((uint8_t*)&crc, sizeof(crc));
  }else{
    // Handle unexpected command, request resend
    //Serial.println("Unexpected command received. Resending or taking action...");
    // Send "200" command to STM32 for requesting a resend
    //stm32.println("200");
  }
}

/*
 * This function will calculate total number of lines present in every address 
 * and store the value in array for every address. 
 * First array will determind total number of lines and another array
 * will determine how many values are present in last line. 
 */
struct LineCalculationResult calculate_lines(int addressCount, int* byteCounts)
{
  int looper = 0;
  struct fetched_data object;
  int* number_lines_per_address = malloc(addressCount* sizeof(int));
  int* number_values_last_line = malloc(addressCount* sizeof(int)); 
  while(looper < addressCount)
  {
    int arraySize = byteCounts[looper];
    int valuesPerLine = 16;
    int remainingValues = arraySize % valuesPerLine;
    int numberOfLines = arraySize / valuesPerLine + (remainingValues > 0 ? 1 : 0);
    number_lines_per_address[looper] = numberOfLines;
    number_values_last_line[looper] = remainingValues;
    looper++; 
  }
  struct LineCalculationResult result;
  result.number_lines_per_address = number_lines_per_address;
  result.number_values_last_line = number_values_last_line;
  //free(number_lines_per_address);
  //free(number_values_last_line);

  return result;
  // TODO: Make sure to free the memory after using result elsewhere. 
}


/***************************************Basic Calculation Ends **********************************/
//Part II: Communication Protocal and its mechanism 
/*
 * General Info: 
 * DATA PACKET STRUCTURE: 
 * |START BYTE|START BYTE|ADDRESS BYTE|ADDRESS BYTE|NO.OF DATA BYTES|DATA|CRC|END BYTE|END BYTE|
 * |----A-----|-----A----|------------|------------|----------------|----|---|---F----|----F---|
 */

void initiate_transmission_protocol(void)
{
  //UART will continuously check for the send data command "DUMBO"
  /*if(initiate_transmission_flag == true)
    {
      initiate_transmission_flag = false; 
      memory_write_flag = true;
      transmit_flag = true; 
      calculate_lines_counter++;  
    }*/
  if(Serial3.available()>=5)
  {
    char receivedCommand[6];   
    Serial3.readBytes(receivedCommand, 5);
    receivedCommand[5]='\0';
    //Serial.println(receivedCommand);
    
    if(strcmp(receivedCommand,"ASDFG") == 0)
    {
      transmit_flag = true; 
      memory_write_flag = true;
      packet_not_received = false; 
      calculate_lines_counter++; 
      ack_counter++;
      pointer_A = pointer_B; 
      pointer_B = 0;
      dumbo_counter = 0;             
    }
    else if(strcmp(receivedCommand,"QWERT") == 0)
    {
      packet_not_received =  true; 
      transmit_flag = false; 
      dumbo_counter++;
      //if( calculate_lines_counter >= 0)calculate_lines_counter--;     
      memory_write_flag = true;
      if(dumbo_counter == 5)
      {
        //TODO:send this to STM32
        Serial3.println("ABORT"); 
        delay(100); 
        resetFunc(); 
        startFlag = false; 
      }
    }
    else
    {
        transmit_flag = false;  
        packet_not_received = false; 
        //Serial.print("marty222");
    }
    while (Serial3.available() > 0) {
      //Serial.print("marty3333");
      Serial3.read();
    }
  }
  delay(10);
}

/*
 * struct fetched_data fetch_hex(File &myFile)
 * This function will fetch bytes of data depends on the file 
 * it will return X no. bytes to send packet function 
 */
int* fetch_hex(File &myFile)
{
  int* data = NULL; 
  if(transmit_flag == true)
  {       
    unsigned long file_pointer_current_position = 0; // TODO: Erase this  
    currentChar = myFile.read();
    //debug:
    //Serial.print("current char:"); 
    //Serial.println(currentChar); 
    if(currentChar == '@')
    {
      char hexAddress[5];
      address_counter_fetch_hex++;  //Means we fetched the address here. 
      dummy_address_counter++;
      calculate_lines_counter = 1;  //lines_counter is reset for fresh count. 
      if(address_counter_fetch_hex == 1)
      {
        dummy_address_counter = 0;
        //calculate_lines_counter = 0;
  //debug:      
        //Serial.print("Fetching address:");
        //Serial.println(address_counter_fetch_hex);
      }   
      myFile.readBytesUntil('\n', hexAddress, sizeof(hexAddress)); //we read full address line here. 
      pointer_A = myFile.position();    //Save Current Pointer location in file
  //debug: 
      //Serial.print("Pointer A value:"); 
      //Serial.println(pointer_A); 
      //Serial.print("Dummy Address Counter in address loop:"); 
      //Serial.println(dummy_address_counter);                      
    }
    
    if(dummy_address_counter <= addressCount) 
    {                 
      struct LineCalculationResult calc_result = calculate_lines(addressCount, byteCounts);
      //debug:
      //Serial.print("Dummy Address Counter in Line calculation block:"); 
      //Serial.println(dummy_address_counter);
      //Serial.print("calculate_lines_counter in Line calculation block:"); 
      //Serial.println(calculate_lines_counter);      
    }
   
  if( calculate_lines_counter < calc_result.number_lines_per_address[dummy_address_counter]) 
      { 
      values = 16; 
      //debug:
      //Serial.print("Printing lines counter Array:"); 
      //Serial.println(calculate_lines_counter);
      //Serial.print("Printing lines counter:"); 
      //Serial.println(calc_result.number_lines_per_address[dummy_address_counter]);
      //Serial.print("Inside Fetch_hex-1, values:"); 
      //Serial.println(values);     
      }
    else 
      {      
      values = calc_result.number_values_last_line[dummy_address_counter];
      //debug:
      //Serial.print("Inside Fetch_hex-2, values:"); 
      //Serial.println(values); 
      //Serial.print("Printing lines counter:"); 
      //Serial.println(calculate_lines_counter);         
      }  
       free(calc_result.number_lines_per_address);
      free(calc_result.number_values_last_line); 
  
    if(memory_write_flag == true)
    {
      memory_write_flag = false; 
      data = (int*)malloc(values*sizeof(int)); 
      if (data == NULL)
      {
        Serial.println("Memory allocation failed!");
        // Handle the error or return from the function
        return data; // Assuming you want to return the partially allocated data
      }
      //debug:
      //Serial.print("Start Address of allocated memory: ");
      //Serial.println((unsigned long)data, HEX);
      //Serial.print("End Address of allocated memory: ");
      //Serial.println((unsigned long)(data + values - 1), HEX);
      // TODO: clear the memory in send_packet function
      // we have determined the values i.e how many times we have to run the below while loop.
      packet_hex_counter  = 0;
    }
  
  if(currentChar != '@' )
  {
    myFile.seek(pointer_A-1); 
  }
      
    while(packet_hex_counter < values)
    {
    currentChar = myFile.read(); 
    //debug:
    //Serial.print("in loop: packet_hex_counter:");
    //Serial.println(packet_hex_counter);
    //Serial.print("in loop: values:");
    //Serial.println(values);
    
    if(isHexDigit(currentChar))
        {
          hex[j] = currentChar;
          j++;
          if(j==2)
          {
            //data_counter++;           
            looper = true;
            j = 0;              
          }
          if(looper == true)
          {
            looper = false; 

           int combinedHex = (hex[0] >= 'A' ? hex[0] - 'A' + 10 : hex[0] - '0') * 16 +
            (hex[1] >= 'A' ? hex[1] - 'A' + 10 : hex[1] - '0');
         
            data[packet_hex_counter]= combinedHex;   // Store the most significant byte                      
            packet_hex_counter++;              
            fetch_hex_while_loop_end_flag = true; 
                  
          }
        } 
  }
      
      if(fetch_hex_while_loop_end_flag == true)
      {
        //debug:
        //Serial.print("Printing lines counter AFTER WHILE:"); 
        //Serial.println(calculate_lines_counter);
        //Serial.print("Printing packet hex counter AFTER WHILE:"); 
        //Serial.println(packet_hex_counter);
        
        //Serial.print("Printing dummy address counter AFTER WHILE:"); 
        //Serial.println(dummy_address_counter);
        if(packet_hex_counter == values)
        {
          transmit_flag = false; 
          packet_not_received = false; 
          fetch_hex_while_loop_end_flag = false; 
          fetch_hex_complete = true;
          //calculate_lines_counter++; 
          //free_flag = true; 
          //CAUTION:
          char hexAddress[5];
          myFile.readBytesUntil('\n', hexAddress, sizeof(hexAddress));
          pointer_B = myFile.position();          
          packet_hex_counter = 0;           
          file_pointer_current_position = myFile.position();
          //debug:
          //Serial.print("Current File position after loop end:"); 
          //Serial.println(int(file_pointer_current_position));         
        }
        if(memory_write_flag == true)
        {
          memory_write_flag = false;
          //calculate_lines_counter--;       
        }    
        return data; 
        }      
    }
    else if(packet_not_received == true)
    {
      transmit_flag = true; 
      myFile.seek(pointer_A); 
      fetch_hex_complete = false; 
      memory_write_flag == true;
      packet_hex_counter = 0;
      //this will resend the last packet        
      //Serial.println("Bingo its working"); 
    }   
}
  

void print_memory(int* data) {  
    fetch_hex_complete = false; 
    //Serial.print("Memory Address: ");
    //Serial.print((uint32_t)&data[i], HEX);
    Serial3.print("A"); 
    Serial3.print("A"); 
    Serial3.print(addressArray[dummy_address_counter],HEX);
    Serial3.print(values); 
    for (int i = 0; i < values; i++) 
    {
        Serial3.print(data[i], HEX);
    }
     Serial3.print("F"); 
     Serial3.print("F"); 
     Serial3.println();     
}
