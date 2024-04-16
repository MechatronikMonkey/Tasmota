/*
  xdrv_101_x9cxxx.ino - Driver for X9Cxxx digital potentiometer.
*/


#ifdef USE_X9Cxxx
/*********************************************************************************************\
 * Driver for X9Cxxx digital potentiometer.
 *
 *
\*********************************************************************************************/

#define XDRV_101 101

#define D_LOG_X9Cxxx       "X9Cxxx: "

struct X9Cxxx {
  int8_t pin_ud = 0;
  int8_t pin_inc = 0;
  int8_t pin_cs = 0;
  bool detected = false;
} X9cxxx;

/*********************************************************************************************\
 * X9Cxxx Functions
\*********************************************************************************************/

// This variable will be set to true after initialization
bool initSuccess = false;


const char X9CxxxCommands[] PROGMEM = "|"  // No Prefix
  "INC|" 
  "DEC|"
  "STR";

void (* const X9CxxxCommand[])(void) PROGMEM = {
  &CmdINC, &CmdDEC, &CmdSTR};

void moveWiper(int direction) {

  int count = 0;

  if (XdrvMailbox.data_len > 0) {
  
    count = (int)(CharToFloat(XdrvMailbox.data));
  }
  else
  {
    ResponseCmndChar(PSTR("Error. Enter increment as integer after command! INC 3 for 3 increments."));
  }
  
  if (count > 99 || count < 1)
  {
    // Count out of range
    ResponseCmndChar(PSTR("Error. Out of range, only 1-99!"));
  }
  else
  {
    // Begin with command by bringing down CS
    digitalWrite(X9cxxx.pin_cs, LOW);
    delayMicroseconds(6);

    // Count upwards set UD = HIGH
    digitalWrite(X9cxxx.pin_ud, direction);
    delayMicroseconds(6);

    // Count numbers
    for (int i=0; i<count; i++)
    {
      digitalWrite(X9cxxx.pin_inc, LOW);
      delayMicroseconds(6); 
      digitalWrite(X9cxxx.pin_inc, HIGH);
      delayMicroseconds(6); 
    }
    
    // Bring INC LOW again before deselect CS, because we want not save to volatile mem yet!
    digitalWrite(X9cxxx.pin_inc, LOW);
    delayMicroseconds(6); 

    // End with command by bringing up CS
    digitalWrite(X9cxxx.pin_cs, HIGH);

    // Bring INC HIGH again in neutral pos. after deselect (no change of whiper anymore)
    digitalWrite(X9cxxx.pin_inc, HIGH);

    // Tell Tasmota that this command was successfull.
    Response_P(PSTR("{\"%s\":\"%i\"}"), XdrvMailbox.command, count);
  }
  // End of Command INC
}

void CmdINC(void) {

  moveWiper(HIGH);
}


void CmdDEC(void) {

  moveWiper(LOW);
}

void CmdSTR(void) {

  // Begin with command by bringing down CS
  digitalWrite(X9cxxx.pin_cs, LOW);

  delayMicroseconds(15);

  // Now bring CS up to begin writing to mem.
  digitalWrite(X9cxxx.pin_cs, HIGH);

  // Wait 20 ms to finish writing to mem.
  delay(20);

  Response_P(PSTR("{\"%s\":\"ok\"}"), XdrvMailbox.command);
}


/*********************************************************************************************\
 * Tasmota Functions
\*********************************************************************************************/



void X9CxxxInit()
{

    X9cxxx.pin_ud = Pin(GPIO_X9Cxxx_UD);
    X9cxxx.pin_inc = Pin(GPIO_X9Cxxx_INC);
    X9cxxx.pin_cs = Pin(GPIO_X9Cxxx_CS);

    // init pins
    pinMode(X9cxxx.pin_ud, OUTPUT);
    digitalWrite(X9cxxx.pin_ud, LOW);

    pinMode(X9cxxx.pin_inc, OUTPUT);
    digitalWrite(X9cxxx.pin_inc, HIGH);

    pinMode(X9cxxx.pin_cs, OUTPUT);
    digitalWrite(X9cxxx.pin_cs, HIGH);

  // Init is successful
  initSuccess = true;

}



void X9CxxxProcessing(void)
{

  /*
    No processing on timer, only processing on command call.
  */

}






/*********************************************************************************************\
 * Interface
\*********************************************************************************************/
bool Xdrv101(uint32_t function)
{


  bool result = false;

  if (FUNC_INIT == function) {
    X9CxxxInit();
    AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("X9Cxxx init is done..."));
  }
  else if (initSuccess) {

    switch (function) {
      // Select suitable interval for polling your function
    case FUNC_EVERY_SECOND:
//      case FUNC_EVERY_250_MSECOND:
//    case FUNC_EVERY_200_MSECOND:
//    case FUNC_EVERY_100_MSECOND:
        X9CxxxProcessing();
        break;

      // Add command support to send command by MQTT or Console.
      case FUNC_COMMAND:
        AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("Calling X9Cxxx command..."));
        result = DecodeCommand(X9CxxxCommands, X9CxxxCommand);
        break;

    }

  }

  return result;
}

#endif  // USE_MY_PROJECT_CMD