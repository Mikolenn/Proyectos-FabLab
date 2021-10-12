/*
 * Universidad de Costa Rica
 *
 * Escuela de Ingeniería Eléctrica
 * 
 * Proyecto eléctrico
 * 
 * "Automatización del sistema hidropónico para producción
 * de cultivos en interiores, propiedad del FabLab del IICA"
 * 
 * Ricardo Arias Castro
 * B60633
 * 
 * Fecha: 15 de julio, 2021
 */


/*
 * Descriptions and comments in English for international compatibility
 * Developer: Ricardo Arias Castro
 * Code version: V1.0
 */


/*
 * Libraries used in the project
 */
#include <Wire.h>
#include <EEPROM.h>
#include <SPI.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <DFRobot_EC.h>
#include <DFRobot_PH.h>
#include <DS18B20.h>
#include <OneWire.h>


/*
 * Variables definition for input/output pins
 */

// Input pins definition

#define pinTemp 2       // Temperature sensor
#define pinPH A0        // PH sensor
#define pinEC A1        // Electrical conductivity sensor
#define pinLevel A2     // Water level sensor
#define pinSD 10        // CS pin of the SD card module

// Output pins definition

#define pinLight 3     // System lights
#define pinHeater 4    // Water heater
#define pinCpump 5     // Water circulation pump
#define pinUpump 6     // Urea dosing pump
#define pinLpump 7     // Level compensation water pump
#define pinBpump 8     // B substance dosing pump
#define pinApump 9     // A substance dosing pump


/*
 * System parameters
 * Change the values, depending on the needs of the crops
 * produced and the environmental characteristics
 */
int dawn = 6;           // Hour at the lights turn on
int nightfall = 20;     // Hour at the lights turn off

int period = 15;        // Period when the water circulation pump is on
                        // It can be 5, 10, 15 or 30 mins

float Tmin = 17;        // Desired temperature value
float PHmax = 6.5;      // Maximun pH value
float ECmin = 1.3;      // Minimun EC value

int displayData = 2;    // Period in minutes, between screen updates
int writeData = 10;     // Period in minutes, between data writing 


/*
 * Control variables
 * Change the values, depending on the performance of the control
 * algorithm
 */
float tA = 8;       // Dosing time constant for substance A
float tB = tA/4;    // Dosing time constant for substance B

// Auxiliary variables of the control algorithm

bool lightsON = false;       // Flag to indicate that the lights are on
bool cPumpON = false;        // Flag to indicate that the water circulation pump is on
int Lvalue = 0;              // Read water level value
float PHvalue = 7.0;         // Read pH value
float ECvalue = 1.5;         // Read EC value
float Tvalue = 25;           // Read temperature value
DateTime rightNow;           // Current day and hour
long start = 0;              // Auxiliar variable for pump A and B activation
long displayAux = 130000;    // Auxiliar variable for screen displaying
long writeAux = 610000;      // Auxiliar variable for writing to SD card


/*
 * Variables for the libraries functionalities
 */
LiquidCrystal_I2C screen(0x27, 16, 2);    // LCD screen 
File data;                                // SD card file
RTC_DS3231 rtc;                           // Real time clock
DS18B20 DS(pinTemp);                      // Temperature sensor
DFRobot_PH pH;                            // PH sensor
DFRobot_EC EC;                            // EC sensor


/*
 * "Set up" function
 * It runs at the beginning and only once, when the Arduino is
 * powered
 */
void setup() {

  // Declaration of the SD module location
  SD.begin(pinSD);

  // Initialization of the pH and EC sensors
  pH.begin();
  EC.begin();

  // Initialization of the screen and the SD card
  initScreen();
  initSDcard();

  // Real time clock initialization and verification
  while ( !rtc.begin() ){

    screen.home();
    screen.print(F(" COLOQUE EL RTC"));
    screen.setCursor(0, 1);
    screen.print(F("DS3231 Y REANUDE"));
    
  }
  
  /* Real time clock synchronization
   * To synchronize the real time clock, uncomment the next line 
   * and upload the code. After sinchronized, comment back the
   * line and upload the code again
   */
  //rtc.adjust( DateTime( F(__DATE__), F(__TIME__) ) );
  
  // Definition of the digital output pins
  pinMode(pinLight, OUTPUT);
  pinMode(pinHeater, OUTPUT);
  pinMode(pinCpump, OUTPUT);
  pinMode(pinUpump, OUTPUT);
  pinMode(pinLpump, OUTPUT);
  pinMode(pinBpump, OUTPUT);
  pinMode(pinApump, OUTPUT);

}

/*
 * "Loop" function
 * It runs repeatedly while the Arduino is powered
 */
void loop() {
  
  rightNow = rtc.now();    // Current date and hour

  lights();

  circulationPump();

  Lvalue = analogRead(pinLevel);    // Level sensor value

  Tvalue = DS.getTempC();         // Tempertature sensor value
  
  // EC voltage value
  float ECv = float(analogRead(pinEC))/1024.0 * 5000;
  // Converted EC value
  ECvalue = EC.readEC(ECv, Tvalue);
  // PH voltage value
  float pHv = float(analogRead(pinPH))/1024.0 * 5000;
  // Converted pH value
  PHvalue = pH.readPH(pHv, Tvalue);

  parametersControl();

  // Condition for writing to SD card
  unsigned long wAux = abs(millis() - writeAux);

  if( wAux > writeData*60000 ) {

    writeSD();
    writeAux = millis();

  }

  // Condition for screen displaying
  unsigned long dAux = abs(millis() - displayAux);

  if( dAux > displayData*60000 ) {

    displayOnScreen();
    displayAux = millis();
  }
  
}

/*
 * Funtion for the initialization of the screen.
 * It turn on the backlight of the screen and display the initial
 * message
 */
void initScreen() {
  
  screen.init();
  screen.backlight();
  screen.begin(16, 2);
  screen.home();
  
  screen.print(F("    SISTEMA"));
  screen.setCursor(0, 1);
  screen.print(F("  HIDROPONICO")); 

  delay(2000);
}

/* 
 * Initialization of the SD card
 * It open the data file of the SD and writes on the data header
 * If the file does not exist, it is created
 */
void initSDcard() {

  data = SD.open("datos.txt", FILE_WRITE);

  screen.clear();
 
  while ( !data ) {

    screen.home();
    screen.print(F("INSERTE TARJETA"));
    screen.setCursor(0, 1);
    screen.print(F(" SD Y REANUDE"));
 
  }
  
  data.print(F("Fecha,"));
  data.print(F("Hora,"));
  data.print(F("pH,"));
  data.print(F("Conductividad eléctrica (mS/cm),"));
  data.print(F("Temperatura (°C),"));
  data.println(F("Nivel del agua"));

  data.close();
}

/* 
 * System lights control
 * Turns the system lights on, if they are off and it is about the
 * activation hour. It also turns the system lights off, if they
 * are on and it is about the deactivation hour
 */
void lights() {

  if( (rightNow.hour() >= dawn) && !lightsON ) {

    digitalWrite(pinLight, HIGH);
    lightsON = true;
  }

  if( (rightNow.hour() >= nightfall) && lightsON ) {

    digitalWrite(pinLight, LOW);
    lightsON = false;
  }
  
}

/* 
 * Circulation pump control
 * The circulation pump turns on at the even multiples of the time
 * constant and turns off at its odd multiples. It is designed for
 * 10, 15 or 30 minute intervals
 */
void circulationPump() {

  if( (rightNow.minute() == 0) || (rightNow.minute() == 2*period) || (rightNow.minute() == 4*period) ) {

    digitalWrite(pinCpump, HIGH);
    
    cPumpON = true;
  }

  if( (rightNow.minute() == period) || (rightNow.minute() == 3*period) || (rightNow.minute() == 5*period) ) {

    digitalWrite(pinCpump, LOW);
    
    cPumpON = false;
  }
  
}

/* 
 * Water parameters control
 * Control the tank level, temperature, electrical conductivity
 * and pH. The algorithm follows the described order in and does
 * not modify the next variable until the previous one is at the
 * correct value
 */
void parametersControl() {

  // Level control
  if( (409 < Lvalue) && (Lvalue < 613) ) digitalWrite(pinLpump, HIGH);
    
  if( Lvalue > 818 ) digitalWrite(pinLpump, LOW);
   

  // Temperature control
  if( (Tvalue < Tmin) && (Lvalue < 50) && cPumpON ) digitalWrite(pinHeater, HIGH);

  if( (Tvalue >= Tmin) || !cPumpON ) digitalWrite(pinHeater, LOW);


  // EC control
  if( (ECvalue < ECmin) && (Tvalue >= Tmin) && (Lvalue < 50) && cPumpON && (start == 0) ) {

    digitalWrite(pinApump, HIGH);
    digitalWrite(pinBpump, HIGH);
    
    start = millis();
        
  }

  unsigned long diff = abs(millis() - start);

  if( diff > tB*1000 ) digitalWrite(pinBpump, LOW);
  
  if( diff > tA*1000 ) {
    
    digitalWrite(pinApump, LOW);
    start = 0;
    
  }
  
  // PH control
  if( (PHvalue > PHmax ) && (ECvalue >= ECmin) && (Tvalue >= Tmin) && (Lvalue < 50) && cPumpON )
      digitalWrite(pinUpump, HIGH);

  if( (PHvalue <= PHmax) || !cPumpON ) digitalWrite(pinUpump, LOW);
  
}

/* 
 * SD card writing
 * Writes the current date and time, with the measured values of
 * pH, electrical conductivity, temperature and tank level on the
 * SD card, using the .csv format
 */
void writeSD() {

  data = SD.open("datos.txt", FILE_WRITE);

  if( data ){

    // Prints the current date and hour
    char buffer [26] = "DDD DD MMM YYYY, hh:mm:ss";
    data.print(rightNow.toString(buffer));
    data.print(",");
    
    // Prints the pH value with 2 decimal places
    data.print(PHvalue, 2);
    data.print(",");
    
    // Prints the EC value with 2 decimal places
    data.print(ECvalue, 2);
    data.print(",");
    
    // Prints the temperature value with 1 decimal places
    data.print(Tvalue, 1);
    data.print(",");

    // Interpretation of the level value for better understanding
    if( Lvalue > 818 ) data.print("Máximo");

    if( (409 < Lvalue) && (Lvalue < 613) ) data.print("Mínimo");
    
    if( Lvalue < 50 ) data.print("Normal");
    
    data.println("");

  }
  data.close();
}

/* 
 * Screen display
 * Writes the measured values of pH, electrical 
 * conductivity and temperature on the LCD screen
 */
void displayOnScreen(){

  screen.clear();
  screen.home();
  screen.print(F("  PH   EC  TEMP"));
  screen.setCursor(0, 1);
  screen.print(" ");
  screen.print(PHvalue, 2);
  screen.print(" ");
  screen.print(ECvalue, 2);
  screen.print(" ");
  screen.print(Tvalue, 1);
  
}
