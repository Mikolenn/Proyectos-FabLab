#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <DHT.h>
#include <DHT_U.h>

// Variables para los pines de entrada/salida
const byte WSPD = 3;
const byte WDIR = A0;
const byte HYTS = 9;
const byte SDIN = 10;

// Declaración de variables generales para los componentes utilizados
LiquidCrystal_I2C screen(0x27, 16, 2);
File lectura;
DHT DHTsensor(HYTS, DHT22);

// Periodos de lectura, actualización de pantalla y escritura 
unsigned int readPeriod = 2;
unsigned int dispPeriod = 2;
unsigned int writePeriod = 60;

// Variables ambientales
float hum = 0;
float temp = 0;
float WSpeed = 0;
float WGust = 0;
String WDir = "";
String WGustDir = "";

// Variables auxiliares para la medición del viento
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
long lastWindCheck = 0;

// Variables auxiliares para periodos de lectura, escritura y despliegue
unsigned int dispTime = 0;
unsigned int writeTime = 0;
unsigned int readTime = 0;


// Interrupción para la lectura de los clicks del anemómetro del pin D3 (2 clicks por rotación)
void wspeedIRQ() {

  // Ignoramos los errores del interruptor, por rebotes menores a 10 ms (velocidad máxima del viento de 228 km/h)
  if (millis() - lastWindIRQ > 10) {
    
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; // Cada click por segundo representa 1.492 MPH (2.401 km/h)
  }
}


void setup() {

  DHTsensor.begin();
  SD.begin(SDIN);

  pinMode(WSPD, INPUT_PULLUP);
  pinMode(HYTS, INPUT);
  pinMode(SDIN, OUTPUT);

  // attach external interrupt pins to IRQ functions
  attachInterrupt(digitalPinToInterrupt(WSPD), wspeedIRQ, FALLING);

  // turn on interrupts
  interrupts();

  initScreen();
  initSDcard();
    
}

void loop() {
  
  unsigned long inicio = millis();

  if ( readTime > readPeriod*1000 ) {
    
    readValues();
    readTime = 0;
  }
  
  if ( dispTime > dispPeriod*1000 ) {

    displayOnScreen();
    dispTime = 0;
  }

  if ( writeTime > writePeriod*1000 ) {
    
    writeSD();
    hum = 0;
    temp = 0;
    writeTime = 0;
  }
  
  unsigned long total = millis() - inicio;
  dispTime += total;
  writeTime += total;
  readTime += total;
}

// Lectura del viento y conversión a km/h
float get_wind_speed() {
  
  float deltaTime = millis() - lastWindCheck;
  
  deltaTime /= 1000.0;          // Conversión a segundos
  
  float windSpeed = (float)windClicks / deltaTime;
  
  windClicks = 0;               // Reinicio de la cuenta de clicks
  lastWindCheck = millis();

  windSpeed *= 2.40114;         // Conversión de MPH a km/h

  return (windSpeed);
}

// Lectura de la dirección del viento
String get_wind_direction() {

  unsigned int adc;

  adc = analogRead(WDIR); // get the current reading from the sensor

  // Los valores de la veleta se muestran comparan en orden ascendente
  // Cada límite se definió 5 unidades de resistencia por sobre el valor del sensor
  
  if (adc < (397+5) ) return ("E");
  if (adc < (483+5) ) return ("SE");
  if (adc < (570+5) ) return ("S");
  if (adc < (701+5) ) return ("NE");
  if (adc < (813+5) ) return ("SO");
  if (adc < (903+5) ) return ("N");
  if (adc < (957+5) ) return ("NO");
  if (adc < (986+5) ) return ("O");
  return ("ERROR"); // error, disconnected?
}

void readValues() {
  
  WSpeed = get_wind_speed();                      // Se lee la velocidad del viento
  WDir = get_wind_direction();                    // Se lee la dirección del viento

  if ( isnan(DHTsensor.readHumidity()) != 1 )
        hum = DHTsensor.readHumidity();           // Se lee la humedad
  if ( isnan(DHTsensor.readTemperature()) != 1 )
        temp = DHTsensor.readTemperature();       // Se lee la temperatura

  // Se guarda la velocidad y dirección de la ráfaga
  // Se actualiza con cada reinicio del sistema
  if ( WGust < WSpeed ) {

    WGust = WSpeed;
    WGustDir = WDir;
  }
}

/* Función encargada de inicializar la pantalla LCD 16x2
 * Enciende la luz de fondo de la pantalla y despliega 
 * un mensjae inicial en esta
 */
void initScreen() {
  
  screen.init();
  screen.backlight();
  screen.begin(16, 2);
  screen.home();
  
  screen.print(F("   ESTACION"));
  screen.setCursor(0, 1);
  screen.print(F(" METEOROLOGICA")); 

  delay(2000);
}

/* Función encargada de inicializar la tarjeta SD
 * Verifica que la tarjeta se encuentra conectada correctamente
 * y escribe el encabezado para los datos, en formato .CSV
 */
void initSDcard() {

  // Archivo para la escritura en la tarjeta SD
  lectura = SD.open("datos.txt", FILE_WRITE);

  // Encabezado de los datos a escribir en la tarjeta
  if( lectura ) {
  
    lectura.print(F("Humedad (%),"));
    lectura.print(F("Temperatura (°C),"));
    lectura.print(F("Velocidad del viento (Km/h),"));
    lectura.print(F("Dirección del viento,"));
    lectura.print(F("Velocidad de la ráfaga (Km/h),"));
    lectura.println(F("Dirección de la ráfaga"));
  }
  else {

    screen.clear();
    screen.home();
    screen.print(F(" TARJETA SD NO"));
    screen.setCursor(0, 1);
    screen.print(F("   DETECTADA"));
  }

lectura.close();
}

/* Función para la escritura de los valores en la tarjeta SD
 * Escribe los valores en la tarjeta, en formato .CSV
 */
void writeSD() {

  lectura = SD.open("datos.txt", FILE_WRITE);

  if( lectura ){
    
    // Escribe el valor de humedad sin espacios decimales
    lectura.print(hum, 0);
    lectura.print(",");

    // Escribe el valor de temperatura con un espacio decimal
    lectura.print(temp, 1);
    lectura.print(",");

    // Escribe el valor de velocidad del viento con un espacio decimal
    lectura.print(WSpeed, 1);
    lectura.print(",");
    
    // Escribe el valor de dirección del viento
    lectura.print(WDir);
    lectura.print(",");

    // Escribe el valor de velocidad de la ráfaga con un espacio decimal
    lectura.print(WGust, 1);
    lectura.print(",");
    
    // Escribe el valor de dirección de la ráfaga con un espacio decimal
    lectura.print(WGustDir);
    lectura.print(",");
    
    // Salto de línea
    lectura.println("");
  }

  lectura.close();
}

/* Función para el despliegue de los valores en pantalla
 * Escribe los valores leídos en la pantalla
 */
void displayOnScreen(){

  screen.clear();
  screen.home();
  
  screen.print(F("VEL "));  // Etiqueta para la velocidad
  screen.print(WSpeed, 1);  // Velocidad
  screen.setCursor(10, 0);  // Etiqueta para la dirección
  screen.print(F("DIR "));  // Dirección
  screen.print(WDir);
  screen.setCursor(0, 1);
  screen.print(F("TEM "));  // Etiqueta para la temperatura
  screen.print(temp, 1);    // Temperatura
  screen.setCursor(10, 1);
  screen.print(F("HUM "));  // Etiqueta para la humedad
  screen.print(hum, 0);     // Humedad
  
}
