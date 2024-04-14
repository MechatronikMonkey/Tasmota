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
  "DEC|";

void (* const X9CxxxCommand[])(void) PROGMEM = {
  &CmdINC, &CmdDEC};

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
    delayMicroseconds(5);

    // Count upwards set UD = HIGH
    digitalWrite(X9cxxx.pin_ud, direction);
    delayMicroseconds(5);

    // Count numbers
    for (int i=0; i<count; i++)
    {
      digitalWrite(X9cxxx.pin_inc, LOW);
      delayMicroseconds(5); 
      digitalWrite(X9cxxx.pin_inc, HIGH);
      delayMicroseconds(5); 
    }

    // End with command by bringing up CS
    digitalWrite(X9cxxx.pin_cs, HIGH);

    // Tell Tasmota that this command was successfull.
    ResponseCmndDone();
  }
  
  // End of Command INC
}

void CmdINC(void) {

  moveWiper(1);
}


void CmdDEC(void) {

  moveWiper(0);
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