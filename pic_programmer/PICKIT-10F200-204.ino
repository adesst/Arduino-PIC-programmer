#define MCLR 8     /* since it toggles opamp then it is inverting mode */
#define PGM 9      /* We dont implemet PGM in PIC10F200/204 */
#define VDD 9      /* Here we toggle VDD so we could have HVP-first or VDD-first */
#define CLK 10
#define DATA 11

#define MCLR_HIGH HIGH
#define MCLR_LOW  LOW

/*
This sketch is the arduino part of the pic programmer project.
It works on the low voltage programming mode and the configuration word
is fixed to ensure it remains in this mode
All the user interaction should be from the python part and this sketch
should only be edited for changing the pin numbers, or if you really know 
what you are doing.
Connection between the And the arduino should be as follows:
  Aduino    |    PIC10F
--------------------------------  
  MCLR(8)  ->    MCLR(GP3, SOT-23 Pin 6, PDIP Pin 8)
  VDD(9)   ->    SOT-23 Pin 2, PDIP Pin 2
  CLK(10)  ->    GP1 (SOT-23 Pin 3, PDIP Pin 4)
  DATA(11) ->    GP0 (SOT-23 Pin 1, PDIP Pin 5)

PIC12F200/204 requires 13V (12V will do), MCLR to the op-amp non-inverting input.

Docs is here: https://ww1.microchip.com/downloads/en/DeviceDoc/41228F.pdf
*/

/*
 * Bit 0-1 Unimplemented
 * Bit 2 - WDTE ON/OFF (0 is Off)
 * Bit 3 - CodeProtection (1 is Off)
 * Bit 4 - MCLRE ( 1 is GP3 as MCLRE and not GPIO) 
 * Bit 5-11 Unimplemented
 */
uint16_t CONFIG_INIT = 0b111111111011;
int DATA_SIZE = 0;
int debugging = 0; //flag for debugging mode

void initial_state()
{
    pinMode(DATA, OUTPUT);
    digitalWrite(DATA, LOW);
    digitalWrite(CLK,  LOW);
    digitalWrite(VDD,  LOW);
    digitalWrite(MCLR, MCLR_LOW);
    
    
    Serial.println("Enter command:");
    Serial.println("P for programming");
    Serial.println("V for verification");
    Serial.println("D for debugging");
    Serial.println("E for erase");
    Serial.println("K for Read Current OSCAL");
    Serial.println("C for Read Backup OSCAL");
    Serial.println("L for Load OSCAL");
    Serial.println("F for Reset Factory");
    Serial.println("U for Read UserID 4 Bytes");
    Serial.println("S for SetUID");
    Serial.println("B for Read ORG_0");
    Serial.println("O for Read InitConfig");
    Serial.println("M for LoadInitConfig()");
    Serial.println("R for reset");
    delay(50);
}

void setup() 
{
    pinMode(MCLR, OUTPUT);
    pinMode(VDD, OUTPUT);
    pinMode(DATA, OUTPUT);
    pinMode(CLK, OUTPUT);

    Serial.begin(9600);
    DATA_SIZE = nextWord(); //getting the data size by making nextWord function send 0x04
    initial_state();
}

void loop() {

    Serial.write(2); //request input
    while(Serial.available() == 0); //waiting for input
    int input = Serial.read();
    switch(input)
    {
        case 'p':
        case 'P':
          //Serial.println("Not implemented yet");
          HVP_init();
          Program(DATA_SIZE);
          break;
        case 'v':
        case 'V':
          debugging = 0;
          HVP_init();
          verify(DATA_SIZE);
          HVP_exit();
          break;
        case 'd':
        case 'D':
          debugging = 1;
          HVP_init();
          verify(DATA_SIZE);
          HVP_exit();
          break;
        case 'e':
        case 'E':
          debugging = 1;
          HVP_init();
          chipErase();
          HVP_exit();
          break;
        case 'k':
        case 'K':
          debugging = 1;
          HVP_init();
          ReadCurrentOSCAL();
          HVP_exit();
          break;
        case 'f':
        case 'F':
          debugging = 1;
          ResetFactory();
          break;
        case 'b':
        case 'B':
          debugging = 1;
          HVP_init();
          ReadORG_0();
          HVP_exit();
          break;
        case 'c':
        case 'C':
          debugging = 1;
          HVP_init();
          ReadCAL();
          HVP_exit();
          break;
        case 'l':
        case 'L':
          debugging = 1;
          HVP_init();
          LoadInitOSCAL();
          HVP_exit();
          break;
        case 'u':
        case 'U':
          debugging = 1;
          HVP_init();
          PrintUID();
          HVP_exit();
          break;
        case 'o':
        case 'O':
          debugging = 1;
          HVP_init();
          ReadInitConfig();
          HVP_exit();
          break;
        case 'm':
        case 'M':
          debugging = 1;
          HVP_init();
          LoadInitConfig();
          HVP_exit();
          break;
        case 's':
        case 'S':
          debugging = 1;
          HVP_init();
          SetUID();
          HVP_exit();
          break;
        case 'r':
        case 'R':
          initial_state();
          break;
    }
}

void HVP_exit()
{
    digitalWrite(DATA, LOW);
    digitalWrite(CLK, LOW);
    digitalWrite(VDD, LOW);
    delayMicroseconds(2);
    digitalWrite(MCLR, MCLR_LOW);
}

void HVP_init()
{
    /*
     * HVP custom mode to toggle op-amp comparator
     * MCRL pin HIGH would be seen as LOW at target IC
     * MCRL pin LOW would be seen as HIGH at target IC
     */
    digitalWrite(DATA, LOW);
    digitalWrite(CLK, LOW);
    digitalWrite(VDD, LOW);
    digitalWrite(MCLR, MCLR_LOW);
    delayMicroseconds(10);

    digitalWrite(MCLR, MCLR_HIGH);
    delayMicroseconds(5);
    digitalWrite(VDD, HIGH);
    delayMicroseconds(5);
}

void ReadORG_0()
{
    ReadInitConfig();
    increment();
    uint16_t dataRead = readData();
    Serial.print("Reading Data ORG_0: ");
    Serial.println(dataRead, BIN);
}

uint16_t ReadCAL()
{
    ReadInitConfig();
    increment(); // it is now at 0x00
    uint16_t calPos = 0x104;
    uint16_t dataRead = 0;
    for(int i =0; i <= calPos; i++)
    {
        if( i == calPos)
        {
            dataRead = readData(); 
        }
        else
        {
            increment();
        }
    }
    Serial.print("Reading Data CAL: ");
    Serial.println(dataRead, BIN);
    return dataRead;
}

void ResetFactory()
{
    uint16_t factoryResetPos = 0x100;
    uint16_t factoryOSCALPos = 0x104;
    
    HVP_init();
    uint16_t bakCal = ReadCAL();
    HVP_exit();

    HVP_init();
    increment();
    
    for(uint16_t i=0; i <= factoryResetPos; i++)
    {
        if( i == factoryResetPos )
        {
            // do not increment();
        }
        else
        {
            increment();
        }
    }

    chipErase();
    HVP_exit();
    
    HVP_init();
    increment();
    for(uint16_t i=0; i <= factoryOSCALPos; i++)
    {
        if( i == factoryOSCALPos )
        {
            // do not increment();
        }
        else
        {
            increment();
        }
    }

    LoadData(bakCal); 
     
    StartProgram();
    delayMicroseconds(2);
    uint16_t dataRead = readData();
    HVP_exit();
    
    Serial.print("Factory RESET, Reading Data CAL: ");
    Serial.println(dataRead, BIN);
}

void SetUID()
{
    HVP_init();
    
    uint16_t initConfig = CONFIG_INIT; // read from factory
    uint16_t uidPos = 0x100;
    uint16_t dataRead = 0;
    uint16_t dataReadUID0 = 0;
    uint16_t dataReadUID1 = 0;
    uint16_t dataReadUID2 = 0;
    uint16_t dataReadUID3 = 0;
    HVP_exit();
    
    HVP_init();
    LoadData(initConfig);
    StartProgram();
    delayMicroseconds(2);
    dataRead = readData();
    increment();
    
    for(uint16_t i=0; i<=uidPos; i++)
    {
        if( i == uidPos)
        {
            // Do nothing
        }
        else
        {
            increment();
        }
    }
    
    LoadData(0b0101);
    StartProgram();
    delayMicroseconds(2);
    dataReadUID0 = readData();
    increment();

    LoadData(0b1001);
    StartProgram();
    delayMicroseconds(2);
    dataReadUID1 = readData();
    increment();

    LoadData(0b1101);
    StartProgram();
    delayMicroseconds(2);
    dataReadUID2 = readData();
    increment();

    LoadData(0b1011);
    StartProgram();
    delayMicroseconds(2);
    dataReadUID3 = readData();
    increment();

    HVP_exit();

    Serial.print("Reading Config: ");
    Serial.println(dataRead, BIN);   
    Serial.print("Reading UID_0: ");
    Serial.println(dataReadUID0, BIN);   
    Serial.print("Reading UID_1: ");
    Serial.println(dataReadUID1, BIN);   
    Serial.print("Reading UID_2: ");
    Serial.println(dataReadUID2, BIN);   
    Serial.print("Reading UID_3: ");
    Serial.println(dataReadUID3, BIN);   
}

void PrintUID()
{
    ReadInitConfig();
    increment(); // it is now at 0x00
    uint16_t calPos = 0x100;
    uint16_t dataRead = 0;
    for(int i =0; i <= calPos; i++)
    {
        if( i == calPos )
        {
            Serial.println("Hit 0x100");
        }
        else 
        {
            increment();
        }
    }

    for(int i =0; i < 4; i++)
    {
        if( i != 0 )
        {
            increment();
        }
        else
        {

        }

        dataRead = readData();
        Serial.print("Reading Data UID: ");
        Serial.println(dataRead, BIN);   
    }
}


uint16_t ReadInitConfig() 
{
    uint16_t dataRead = readData();
    Serial.print("Reading Data Config: ");
    Serial.println(dataRead, BIN);
    return dataRead;
}

uint16_t ReadCurrentOSCAL() 
{
    increment(); // initial PC from 0xff to 0x00
    
    int OSCALpos = 0xff;
    for(int i=0; i < 0xff; i++) 
    {
        increment();
    }
    uint16_t dataRead = readData();
    Serial.print("Reading Data OSCAL: ");
    Serial.println(dataRead, BIN);
    return dataRead;
}

void LoadInitOSCAL()
{
    uint16_t backupOSCAL = ReadCAL();
    uint16_t dataRead = 0;
    
    HVP_exit();
    HVP_init();
    uint16_t currentOSCAL = ReadCurrentOSCAL();
    LoadData(backupOSCAL);
    StartProgram();
    delayMicroseconds(10);
       
    dataRead = readData();
    Serial.print("On Writing.. OSCAL ");
    Serial.println(backupOSCAL, BIN);
    Serial.print("On verifying.. OSCAL ");
    Serial.println(dataRead, BIN);
}

void LoadInitConfig()
{
    uint16_t initConfig = CONFIG_INIT; // read from factory
    
    uint16_t dataRead = 0;
    HVP_exit();
    
    HVP_init();
    LoadData(initConfig);
    StartProgram();
    delayMicroseconds(2);
    dataRead = readData();
    Serial.print("Verifying Config: ");
    Serial.println(initConfig, BIN);
    Serial.print(" with ");
    Serial.println(dataRead, BIN);
    HVP_exit();
}

void Program(int _data_size)
{
    uint16_t dataRead = ReadInitConfig();
    increment();
    
    uint16_t sentData= 0;
    for (int i = 0; i < _data_size-1; i++)
    {
        sentData = nextWord();
        LoadData(sentData);
        
        StartProgram();
        delayMicroseconds(10);
       
        dataRead = readData();
        Serial.print("On Writing.. Data program ");
        Serial.println(sentData, BIN);
        Serial.print("On verifying.. Data program ");
        Serial.println(dataRead, BIN);
       
        increment(); 
        dataRead = readData();
        dataRead = 0;
        sentData = 0;
        delayMicroseconds(8);
    }

    HVP_exit();
    
    Serial.println("Progamming Done");
    Serial.write(3);
}

void verify(int _data_size)
{
    int flag = 0;
    uint16_t dataRead = 0;
    int _word = 0;
    Serial.print("Data Size is "); 
    Serial.println(_data_size, DEC); 

    // first data is the Config
    if(debugging == 1)
    {
        Serial.print("Config word "); 
        Serial.println(readData(),BIN);
    }

    increment(); // skip OSCAL
    
    for(int i = 0; i < _data_size-1; i++)
    {
        _word = nextWord();
        //_word = 0b001111111001;
        dataRead = 0;
        dataRead = readData();
        if(dataRead != _word)
        {
            if(debugging == 1)
            {
                Serial.print("Error at word "); 
                Serial.print(i,DEC); 
                Serial.print(": Expected ");  
                Serial.print(_word,BIN); 
                Serial.print(" but found "); 
                Serial.println(dataRead,BIN);
            }
            flag = 1;
        }
        increment();
        delayMicroseconds(2);
    }
    
    if(flag == 0)
        Serial.println("Verification Done");
    else if(debugging == 0)
        Serial.println("Verification failed, try reprogramming or debugging.");
    Serial.write(3);

    HVP_exit();
}

int nextWord()
{
    int value = 0;
    int askDataSize = 0;
    if(DATA_SIZE == 0)
    {
        askDataSize = 1;
        Serial.write(4);
    }
    else
    {
        Serial.write(1);
    }
      
    for(int i = 0; i < 2; i++)
    {
        while(Serial.available() == 0);
        value += Serial.read()<<(8*i);
    }

    if( askDataSize == 1 )
    {
        askDataSize = 0;
        Serial.print("Data size is ");
        Serial.println(value, DEC);
    }
    
    return value;
}
    
void writeBit(int a)
{
    digitalWrite(DATA, a);
    digitalWrite(CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(CLK, LOW);
    delayMicroseconds(1);
    digitalWrite(DATA, LOW);
}

byte readBit()
{
    byte _bit;
    digitalWrite(CLK, HIGH);
    delayMicroseconds(1);
    _bit = digitalRead(DATA);
    digitalWrite(CLK, LOW);
    delayMicroseconds(1);
    return _bit;
}

void sendCommand(char command)
{
  // LSB first, all 6 cycles
  int max_cycle = 6;

  // https://stackoverflow.com/questions/2249731/how-do-i-get-bit-by-bit-data-from-an-integer-value-in-c
  writeBit((command >> 0) & 1);
  writeBit((command >> 1) & 1);
  writeBit((command >> 2) & 1);
  writeBit((command >> 3) & 1);
  writeBit((command >> 4) & 1);
  writeBit((command >> 5) & 1); 
}

void sendData(uint16_t dataword)
{
   writeBit(1);
   for (int i = 0; i < 12; i++)
   {
     writeBit((dataword >> i) & 1);
   }
   writeBit(1);
   writeBit(1);
   writeBit(1);
}

void LoadData(uint16_t dataword)
{
   sendCommand(0b000010);
   delayMicroseconds(2); 
   sendData(dataword);
   delayMicroseconds(2);
}

uint16_t readData()
{
    sendCommand(0b000100);
    delayMicroseconds(2);
    uint16_t value = 0;
    pinMode(DATA, INPUT);
    readBit(); // start bit
    for(int i = 0; i < 12; i++)
    {
        value = value | (readBit()<<i);
    }
    readBit(); // dont care bit
    readBit(); // dont care bit
    readBit(); // stop bit
    pinMode(DATA, OUTPUT);
    return value;
}

void StartProgram()
{
   sendCommand(0b001000);
   delayMicroseconds(2);
   sendCommand(0b001110);
   delayMicroseconds(100); // 100uS needed to discharge
}

void StartEraseProgram()
{
   sendCommand(0b001000);
   delay(4);
}

void loadConfig()
{
   sendCommand(0b000000);
   delayMicroseconds(2); 
   sendData(0x3FFF);
   delayMicroseconds(2);
}

void increment()
{
   sendCommand(0b000110);
   delayMicroseconds(2);
}

void chipErase()
{
   sendCommand(0b001001);
   delay(10);
}
