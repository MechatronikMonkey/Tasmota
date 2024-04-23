/*
  xdrv_102_DeHumidifier.ino - Driver for Dehumidifier electronics by Chris.
*/


//#ifdef USE_DeHum
/*********************************************************************************************\
 * Driver for Dehumidifier electronics by Chris.
 *
 *
\*********************************************************************************************/

#define XDRV_102 102

#define D_LOG_DeHum       "DeHum: "

struct DeHum {
  int8_t pin_adc = 0;
  int8_t pin_comp = 0;
  int8_t pin_fanh = 0;
  int8_t pin_fanl = 0;
  bool detected = false;
} DeHum;

bool FAN_ON = false;
int FAN_SPEED = 0;
bool COMP_ON = false;
int minOffTimer = 0;
int FanTrailingTimer = 0;

/*********************************************************************************************\
 * Dehumidifier Functions
\*********************************************************************************************/

// This variable will be set to true after initialization
bool DeHumInitSuccess = false;


const char DeHumCommands[] PROGMEM = "|"  // No Prefix
  "FAN|" 
  "COMP";

void (* const DeHumCommand[])(void) PROGMEM = {
  &CmdFAN, &CmdCOMP};

void CmdFAN(void) {

  int fan_speed = 0;

  if (XdrvMailbox.data_len > 0) {
  
    fan_speed = (int)(CharToFloat(XdrvMailbox.data));
  }
  else
  {
    ResponseCmndChar(PSTR("Error. Enter Fan speed as integer after command! FAN 2 for high speed fan."));
  }
  
  if (fan_speed > 2 || fan_speed < 0)
  {
    // Count out of range
    ResponseCmndChar(PSTR("Error. Out of range, only 0-2!"));
  }
  else
  {
    switch (fan_speed)
    {
    case 0: 
      // Set Fan to low level and set trailing run.
      digitalWrite(DeHum.pin_fanh, LOW);
      delay(5);
      digitalWrite(DeHum.pin_fanl, HIGH);
      FanTrailingTimer = 60; // Set trailing timer for fan to 60 seconds.
      FAN_SPEED = 1;
      FAN_ON = true;

      digitalWrite(DeHum.pin_comp, LOW);
      COMP_ON = false;

      Response_P(PSTR("Compressor OFF. FAN set to low... trailing time set to 60 seconds."));

    break;
    case 1:
      // Set Fan to low level
      digitalWrite(DeHum.pin_fanh, LOW);
      delay(5);
      digitalWrite(DeHum.pin_fanl, HIGH);
      FAN_SPEED = 1;
      FAN_ON = true;
      Response_P(PSTR("FAN set to low."));
      break;
    case 2:
      // Set Fan to high level
      digitalWrite(DeHum.pin_fanl, LOW);
      delay(5);
      digitalWrite(DeHum.pin_fanh, HIGH);
      FAN_SPEED = 2;
      FAN_ON = true;
      Response_P(PSTR("FAN set to high."));
      /* code */
    break;
    
    default:
      break;
    }
  }
  
}


void CmdCOMP(void) {

int comp = 0;

  if (XdrvMailbox.data_len > 0) {
  
    comp = (int)(CharToFloat(XdrvMailbox.data));
  }
  else
  {
    ResponseCmndChar(PSTR("Error. Enter Compressor state! COMP 1 for on, COMP 0 for off."));
  }
  
  if (comp > 1 || comp < 0)
  {
    // Count out of range
    ResponseCmndChar(PSTR("Error. Out of range, only 0-1!"));
  }
  else
  {
    if (comp == 1)
    {
      // Set Fan to low level
      digitalWrite(DeHum.pin_fanh, LOW);
      delay(5);
      digitalWrite(DeHum.pin_fanl, HIGH);
      FAN_ON = true;
      FAN_SPEED = 1;

      // Set Compressor to ON
      digitalWrite(DeHum.pin_comp, HIGH);
      COMP_ON = true;
      ResponseCmndChar(PSTR("Compressor ON."));
    }
    else
    {
      // Set Compressor to OFF
      digitalWrite(DeHum.pin_comp, LOW);
      COMP_ON = false;
      ResponseCmndChar(PSTR("Compressor OFF."));
    }
  }
}



/*********************************************************************************************\
 * Tasmota Functions
\*********************************************************************************************/



void DeHumInit()
{

  DeHum.pin_comp = Pin(GPIO_HUM_COMP);
  DeHum.pin_fanh = Pin(GPIO_HUM_FANH);
  DeHum.pin_fanl = Pin(GPIO_HUM_FANL);

  // init pins
  pinMode(DeHum.pin_comp, OUTPUT);
  digitalWrite(DeHum.pin_comp, LOW);

  pinMode(DeHum.pin_fanh, OUTPUT);
  digitalWrite(DeHum.pin_fanh, LOW);

  pinMode(DeHum.pin_fanl, OUTPUT);
  digitalWrite(DeHum.pin_fanl, LOW);

  // Init is successful
  DeHumInitSuccess = true;

}



void DeHumProcessing(void)
{
  // is the compressor running and the fan off?
  if (COMP_ON == true && FAN_ON == false)
  { 
    //switch on fan on low
    digitalWrite(DeHum.pin_fanh, LOW);
    delay(5);    
    digitalWrite(DeHum.pin_fanl, HIGH); 
    FAN_ON = true;
    FAN_SPEED = 1;
    Response_P(PSTR("FAN set to low because compressor cannot run without fan."));
  }

}


void DeHumShow(void)
{
  ResponseAppend_P(PSTR("{\"FAN_SPEED\": %i}"), FAN_SPEED);
  ResponseAppend_P(PSTR("{\"COMPRESSOR\": %i"), COMP_ON);
  ResponseJsonEnd();
}



/*********************************************************************************************\
 * Interface
\*********************************************************************************************/
bool Xdrv102(uint32_t function)
{


  bool result = false;

  if (FUNC_INIT == function) {
    DeHumInit();
    AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("DeHum init is done..."));
  }
  else if (DeHumInitSuccess) {

    switch (function) {
      // Select suitable interval for polling your function
    case FUNC_EVERY_SECOND:
        if (FanTrailingTimer > 0)
          FanTrailingTimer--;

        // FAN timer reached 1, so disable fan and set 0
        if (FanTrailingTimer == 1)
          {
            digitalWrite(DeHum.pin_fanh, LOW);
            digitalWrite(DeHum.pin_fanl, LOW);
            FanTrailingTimer = 0;
            FAN_ON = false;
            FAN_SPEED = 0;
          }
    break;
//      case FUNC_EVERY_250_MSECOND:
//    case FUNC_EVERY_200_MSECOND:
    case FUNC_EVERY_50_MSECOND:
        DeHumProcessing();
        break;

    case FUNC_JSON_APPEND:
        DeHumShow();
    break;
#ifdef USE_WEBSERVER
    case FUNC_WEB_SENSOR:
    
    break;
#endif  // USE_WEBSERVER
      // Add command support to send command by MQTT or Console.
      case FUNC_COMMAND:
        AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("Calling DeHum command..."));
        result = DecodeCommand(DeHumCommands, DeHumCommand);
        break;

    }

  }

  return result;
}

//#endif  // USE_MY_PROJECT_CMD