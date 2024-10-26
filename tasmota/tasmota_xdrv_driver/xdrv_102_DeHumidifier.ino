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
  int8_t pin_water = 0;
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
int water_full = 0;
int restartProtection = 0;
bool COMP_ON_LAST = false;

bool COMP_STATE = false;
bool COMP_REQUS = false;

bool FAN_STATE = false;
bool FAN_REQUS = false;

/*********************************************************************************************\
 * Dehumidifier Functions
\*********************************************************************************************/

// This variable will be set to true after initialization
bool DeHumInitSuccess = false;


const char DeHumCommands[] PROGMEM = "|"  // No Prefix
  "FAN|" 
  "COMP|"
  "RES";

void (* const DeHumCommand[])(void) PROGMEM = {
  &CmdFAN, &CmdCOMP, &CmdRES};

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
      // For switching off fan, the compressor needs to shut down.
      // Triger Compressor Shutdown
      FAN_REQUS = false;
      Response_P(PSTR("Request FAN OFF."));

    break;
    case 1:
      FAN_SPEED = 1;
      FAN_REQUS = true;
      Response_P(PSTR("Request FAN set to low."));
      break;
    case 2:
      // Set Fan to high level
      FAN_SPEED = 2;
      FAN_REQUS = true;
      Response_P(PSTR("Request FAN set to high."));
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
      // Set Fan to high level

      COMP_REQUS = true;
      Response_P(PSTR("Request Compressor ON."));
    }
    else
    {
      // Trigger Compressor shutdown with 90 seconds trailing time
      COMP_REQUS = false;
      Response_P(PSTR("Request Compressor OFF..."));
    }
  }
}

void CmdRES(void) {

int res = 0;

  if (XdrvMailbox.data_len > 0) {
  
    res = (int)(CharToFloat(XdrvMailbox.data));
  }
  else
  {
    ResponseCmndChar(PSTR("Error. Enter 1 to reset Water and restart!"));
  }
  
  if (res > 1 || res < 0)
  {
    // Count out of range
    ResponseCmndChar(PSTR("Error. Out of range, only 0-1!"));
  }
  else
  {
    if (res == 1)
    {
      water_full = 0;
      Response_P(PSTR("Water reset. You can start compressor now."));
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
  DeHum.pin_water = Pin(GPIO_HUM_SENS);

  // init pins
  pinMode(DeHum.pin_comp, OUTPUT);
  digitalWrite(DeHum.pin_comp, LOW);

  pinMode(DeHum.pin_fanh, OUTPUT);
  digitalWrite(DeHum.pin_fanh, LOW);

  pinMode(DeHum.pin_fanl, OUTPUT);
  digitalWrite(DeHum.pin_fanl, LOW);

  pinMode(DeHum.pin_water, INPUT);

  // Init is successful
  DeHumInitSuccess = true;

}



void DeHumProcessing(void)
{
  // Transitions 
  if (COMP_REQUS != COMP_ON)
  {

    // COMP OFF to ON only when restart protection 0
    // Enable FAN with compressor
    if (COMP_REQUS == true && restartProtection == 0)
    {
      COMP_ON = true;
      FAN_ON = true;
    }
    
    if (COMP_REQUS == false)
    {
      COMP_ON = false;
      FAN_REQUS = false; // Not off immediatelly but after trailing timer
      restartProtection = 90;
      FanTrailingTimer = 90;
    }
  }

  if (FAN_REQUS != FAN_ON)
  {
    // FAN ON to OFF only when  COMP is off AND trailing timer 0
    if (FAN_REQUS == false && COMP_ON == false && FanTrailingTimer == 0)
    {
      FAN_ON = false;
    }
    else
    {
      FAN_ON = true;
    }
  }

  if (FAN_ON)
  {
    digitalWrite(DeHum.pin_fanh, HIGH);
    delay(5);
    digitalWrite(DeHum.pin_fanl, LOW);
  }
  else
  {
    digitalWrite(DeHum.pin_fanh, LOW);
    delay(5);
    digitalWrite(DeHum.pin_fanl, LOW);
  }
      
  if (COMP_ON)
  {
      // Set Compressor to ON
      digitalWrite(DeHum.pin_comp, HIGH);
  }
  else
  {
          // Set Compressor to OFF
      digitalWrite(DeHum.pin_comp, LOW);
  }


}

void DeHumShow(void)
{
  ResponseAppend_P(PSTR("{\"FAN_SPEED\": %i}"), FAN_SPEED);
  ResponseAppend_P(PSTR("{\"COMPRESSOR\": %i}"), COMP_ON);
  ResponseAppend_P(PSTR("{\"WATER\": %i"), water_full);
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
        {
          FanTrailingTimer --;
        }
        else
        {
          FanTrailingTimer = 0;
        }
          

        if (restartProtection > 0)
        {
          restartProtection --;
        }
        else
        {
          restartProtection = 0;
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