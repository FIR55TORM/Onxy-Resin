#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <arduino-timer.h>
#include <Wire.h>
#include <IR_Thermometer_Sensor_MLX90614.h>
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <math.h>

#define DEBUG 1

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugLn(x) Serial.println(x)
#else
#define debug(x)
#define debugLn(x)
#endif

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
IR_Thermometer_Sensor_MLX90614 MLX90614 = IR_Thermometer_Sensor_MLX90614();
auto timer = timer_create_default();
byte mac[] = {0xDE, 0xAD, 0xBF, 0xEF, 0xFE, 0xEA}; // physical mac address
byte ip[] = {192, 168, 1, 112};
EthernetServer server(80);
EthernetClient client;
File webPage;
String readString;

bool hasPrinterStopped = false;
bool hasShutDownStarted = false;
bool isPrinterOn = false;
bool isHeaterOn = false;
//bool isPhotoInterrupterClosed = false;
bool isLCDMessagesPaused = false;
bool isResettingSystem = false;

int nextLcdMessage = 0;

//const int photoInterrupterPin = 3; // define the port of light blocking module //Unused but retained for reference
const int heaterPowerRelayPin = 6;
const int printerPowerRelayPin = 5;

void setup()
{
  lcd.begin(16, 2);
  lcd.print("Initializing....");

  Ethernet.begin(mac, ip);
  server.begin();

  Serial.begin(9600);
  if (!MLX90614.begin())
  {
    debugLn("Temp Sensor Error!!!");
  };

  debugLn(Ethernet.localIP());
  debugLn("Initializing SD card...");

  lcd.print(Ethernet.localIP());

  if (!SD.begin(4))
  {
    debugLn("ERROR - SD card initialization failed!");
    return;
  }
  else
  {
    debugLn("SUCCESS - SD card initialized.");
  }

  if (!SD.exists("index.htm"))
  {
    debugLn("ERROR - Can't find index.htm file!");
  }
  else
  {
    debugLn("SUCCESS - Found index.htm file.");
  }

  //pinMode(photoInterrupterPin, INPUT); // define light blocking module as a output port //Unused but retained for reference
  pinMode(heaterPowerRelayPin, OUTPUT);
  pinMode(printerPowerRelayPin, OUTPUT);

  //digitalWrite(photoInterrupterPin, LOW); //Unused but retained for reference
  digitalWrite(heaterPowerRelayPin, HIGH);
  digitalWrite(printerPowerRelayPin, HIGH);
}

void (*resetFunc)(void) = 0; // declare reset function at address 0

void resetSystem()
{
  resetFunc();
}

void loop()
{
  timer.tick();
  client = server.available();

  if (client)
  {
    bool currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();

        // read char by char HTTP request
        if (readString.length() < 100)
        {
          // store characters to string
          readString += c;
        }

        if (c == '\n')
        {
          bool servePage = true;

          client.println("HTTP/1.1 200 OK");
          client.println("Access-Control-Allow-Origin: *");

          if (readString.indexOf("/GetStatus") > 0)
          {
            getSystemStatus();

            // clearing string for next read
            readString = "";
            break;
          }
          else
          {
            if (readString.indexOf("?HeaterPower=on") > 0)
            {
              powerOnHeater();
              servePage = false;
            }

            if (readString.indexOf("?HeaterPower=off") > 0)
            {
              powerOffHeater();
              servePage = false;
            }

            if (readString.indexOf("?PrinterPower=on") > 0)
            {
              powerOnPrinter();
              servePage = false;
            }

            if (readString.indexOf("?PrinterPower=off") > 0)
            {
              powerOffPrinter();
              servePage = false;
            }

            if (readString.indexOf("?ResetSystem") > 0)
            {
              servePage = false;
              client.write("");
              isResettingSystem = true;
              break;
            }

            if (servePage)
            {
              client.println("Content-Type: text/html");
              client.println("Connection: close");
              client.println();

              webPage = SD.open("index.htm");
              if (webPage)
              {
                while (webPage.available())
                {
                  client.write(webPage.read());
                }
                webPage.close();
              }
            }

            // clearing string for next read
            readString = "";
            break;
          }
        }
        else if (c != '\r')
        {
          currentLineIsBlank = false;
        }
      }
    }

    client.stop();
  }

  if (isResettingSystem)
  {
    delay(2000);
    resetSystem();
  }

  if (!isLCDMessagesPaused)
  {
    switch (nextLcdMessage)
    {
    case 0:
      timer.in(2000, displayTemperatures);
      break;

    case 1:
      timer.in(2000, displayPrinterAndHeaterStatus);
      break;
    }
  }
}

void getSystemStatus()
{
  const int capacity = 5 * JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> doc;

  doc["temperature"]["ambient"] = round(MLX90614.GetAmbientTemp_Celsius());
  doc["temperature"]["resin"] = round(MLX90614.GetObjectTemp_Celsius());
  doc["isHeaterOn"] = isHeaterOn;
  doc["isPrinterOn"] = isPrinterOn;  

  int length = measureJsonPretty(doc);

  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(length + 2);
  client.println();
  client.println();

  // Write JSON document
  serializeJsonPretty(doc, client);
}

bool displayTemperatures(void *argument)
{
  lcd.setCursor(0, 0);
  lcd.print("Ambient:    ");
  lcd.print(round(MLX90614.GetAmbientTemp_Celsius()));
  lcd.print(char(223)); // ascii degree symbol
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Resin:      ");
  lcd.print(round(MLX90614.GetObjectTemp_Celsius()));
  lcd.print(char(223)); // ascii degree symbol
  lcd.print("C");

  nextLcdMessage = 1;

  return false;
}

bool displayPrinterAndHeaterStatus(void *argument)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Printer is ");
  if (isPrinterOn)
  {
    lcd.print("On");
  }
  else
  {
    lcd.print("Off");
  }
  lcd.setCursor(0, 1);
  lcd.print("Heater is ");
  if (isHeaterOn)
  {
    lcd.print("On");
  }
  else
  {
    lcd.print("Off");
  }

  nextLcdMessage = 0;
  return false;
}

void powerOffHeater()
{
  if (isHeaterOn)
  {
    digitalWrite(heaterPowerRelayPin, HIGH);
    isHeaterOn = false;
    debugLn("HEATER: Off");
  }
}

void powerOnHeater()
{
  if (isPrinterOn && !isHeaterOn)
  {
    digitalWrite(heaterPowerRelayPin, LOW);
    isHeaterOn = true;
    debugLn("HEATER: On");
  }
}

void powerOffPrinter()
{
  if (isPrinterOn)
  {
    digitalWrite(printerPowerRelayPin, HIGH);
    isPrinterOn = false;
    debugLn("PRINTER: Off");
    softResetSystem();
  }
}

void powerOnPrinter()
{
  digitalWrite(printerPowerRelayPin, LOW);
  isPrinterOn = true;
  hasPrinterStopped = false;
  debugLn("PRINTER: On");
}

bool powerOffPrinterByPhotoInterrupter(void *argument)
{
  if (!hasPrinterStopped)
  {
    powerOffPrinter();
    powerOffHeater();
    debugLn("PRINTER: Has Stopped");
    hasPrinterStopped = true;
    return false; // stop timer;
  }
}

void softResetSystem()
{
  if (!isPrinterOn)
  {
    // reset flags
    hasShutDownStarted = false;
  }
}
