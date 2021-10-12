// Librerías del proyecto
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <DHT.h>
#include <DHT_U.h>

// Declaración del tipo de sensor a utilizar
#define DHTTYPE DHT22


// Intervalo de tiempo entre lecturas. Debe ingresar el valor en minutos
int inter = 15;


// Declaración de variables generales para los componentes utilizados
LiquidCrystal_I2C pant(0x27, 16, 2);
File lectura;
RTC_DS3231 rtc;

int dhtInterno = 5;
int dhtExterno = 6;
int sensorSuelo = A0;
int lastMin = 0;

DHT DHTinterno(dhtInterno, DHTTYPE);
DHT DHTexterno(dhtExterno, DHTTYPE);


// Calibración propia para el sensor de humedad del suelo
float m = -0.20964;
float b = 201.78;

// Variables para las lecrturas de temperatura y humedad
float Hin = -99;
float Tin = -99;
float Hout = -99;
float Tout = -99;


// Función set up
void setup() {

  // Inicialización de la pantalla
  initPant();

  // Inicializacion de los sensores y componentes adicionales
  DHTinterno.begin();
  DHTexterno.begin();
  rtc.begin();
  SD.begin(10);

  // Archivo para la escritura en la tarjeta SD
  lectura = SD.open("lecturas.txt", FILE_WRITE);

  // Encabezado de los datos a escribir en la tarjeta
  if( lectura ){
  
    lectura.print("Fecha,");
    lectura.print("Hora,");
    lectura.print("Humedad interna (%),");
    lectura.print("Temperatura interna (°C),");
    lectura.print("Humedad externa (%),");
    lectura.print("Temperatura externa (°C),");
    lectura.print("Humedad en la bolsa,");
    lectura.println("Humedad en la bolsa (%)");
  }
  lectura.close();


  /* Sincronización del reloj en tiempo real
   * Para sincronizar el reloj en tiempo real, descomente la siguiente línea 
   * y cargue el programa. Luego de sincronizar, comente nuevamente la línea 
   * y cargue nuevamente el programa
   */
  //rtc.adjust( DateTime( F(__DATE__), F(__TIME__) ) );


  // Declaración del comportamiento de las terminales del Arduino
  pinMode(sensorSuelo, INPUT);
  pinMode(dhtInterno, INPUT);
  pinMode(dhtExterno, INPUT);
  analogReference(EXTERNAL);
}


// Ciclo loop
void loop() {

  // Lectura de la fecha y hora actual y, cálculos necesarios
  DateTime actual = rtc.now();
  int mins = actual.minute() % inter;
  
  // Lectura de los sensores ambientales, con verificación de errores
  if ( isnan(DHTinterno.readHumidity()) != 1 )
      Hin = DHTinterno.readHumidity();
  if ( isnan(DHTinterno.readTemperature()) != 1 )
      Tin = DHTinterno.readTemperature();
  if ( isnan(DHTexterno.readHumidity()) != 1 )
      Hout = DHTexterno.readHumidity();
  if ( isnan(DHTexterno.readTemperature()) != 1 )
      Tout = DHTexterno.readTemperature();

  // Lectura del sensor de humedad para suelo
  float Hbag = analogRead(sensorSuelo);
  float HbagC = m * Hbag + b;

  // Condición de lectura. Se cumple una vez por minuto
  if ( lastMin != actual.minute() ){

    lastMin = actual.minute();
    
    // Despliegue de los valores en pantalla
    pantalla(Hin, Tin, Hout, Tout, Hbag, HbagC);
    
    // Condición de escritura. Se cumple si los minutos son 0, 15, 30 o 45
    if ( lastMin == 0 || mins == 0 ) {
         
      // Escritura de los valores en la tarjeta SD
      escribir(actual, Hin, Tin, Hout, Tout, Hbag, HbagC);

      // Reinicio de los valores almacenados
      Hin = -99;
      Tin = -99;
      Hout = -99;
      Tout = -99;
      
    }
  }
  
  // Desplazamiento del mensaje en pantalla, hacia la izquierda
  for(int i=0; i < 15; i++){
    
    delay(400);  
    pant.scrollDisplayLeft();
  }

  // Regreso del cursor de la pantalla a su posición original
  delay(700);
  pant.home();
  delay(700);
}

// Función encargada de inicializar la pantalla LCD 16x2
void initPant(){
  
  // Inicialización de la pantalla y activación de su iluminación
  pant.init();
  pant.backlight();
  pant.begin(16, 2);
  pant.home();
  
  // Mensaje inicial
  pant.print("MEDICION DE HUMEDAD/TEMPERATURA");
  pant.setCursor(0, 1);
  pant.print("COLOQUE LOS SENSORES Y ESPERE");

  // Desplazamiento del mensaje en pantalla, hacia la izquierda
  for(int i=0; i < 15; i++){
    
    delay(400);  
    pant.scrollDisplayLeft();
  }

  // Regreso del cursor de la pantalla a su posición original
  delay(700);
  pant.home();
  delay(200);
}

/* Función para el despliegue de los valores en pantalla
 * Recibe como parámetros las lecturas realizadas y escribe
 * los valores en la pantalla
 */
void pantalla(float Hin, float Tin, float Hout, float Tout, float Hbag, float HbagC) {

  // Limpia la pantalla
  pant.clear();
  
  // Escritura del valor de humedad interna, con un valor decimal
  pant.print("Hin:");
  pant.print(Hin,1);

  // Escritura del valor de humedad externa, con un valor decimal
  pant.print(" Hout:");
  pant.print(Hout,1);

  // Escritura del valor de humedad en la bolsa, sin decimales
  pant.print(" Hbag:");
  pant.print(Hbag,0);

  pant.setCursor(0,1);
  
  // Escritura del valor de temperatura interna, con un valor decimal
  pant.print("Tin:");
  pant.print(Tin,1);

  // Escritura del valor de temperatura externa, con un valor decimal
  pant.print(" Tout:");
  pant.print(Tout,1);
  
  // Escritura del porcentage de humedad en la bolsa, con un valor decimal
  pant.print(" Hbag:");
  pant.print(HbagC,1);
  pant.print("%");
}

/* Función para la escritura de los valores en la tarjeta SD
 * Recibe como parámetros las lecturas realizadas y escribe
 * los valores en la tarjeta, en formato .CSV
 */
void escribir(DateTime actual, float Hin, float Tin, float Hout, float Tout, float Hbag, float HbagC) {

  // Se abre el archivo para la escritura en él
  lectura = SD.open("lecturas.txt", FILE_WRITE);

  // Se verifica la correcta apertura, para evitar errores
  if( lectura ){

    // Se procesa la fecha y hora y, se escribe
    char buffer [26] = "DDD DD MMM YYYY, hh:mm:ss";
    lectura.print(actual.toString(buffer));
    lectura.print(",");

    // Escritura del valor de humedad interna, con un valor decimal
    lectura.print(Hin,1);
    lectura.print(",");

    // Escritura del valor de temperatura interna, con un valor decimal
    lectura.print(Tin,1);
    lectura.print(",");

    // Escritura del valor de humedad externa, con un valor decimal
    lectura.print(Hout,1);
    lectura.print(",");

    // Escritura del valor de temperatura externa, con un valor decimal
    lectura.print(Tout,1);
    lectura.print(",");

    // Escritura del valor de humedad en la bolsa, sin decimales
    lectura.print(Hbag,1);
    lectura.print(",");

    // Escritura del porcentage de humedad en la bolsa, con un valor decimal
    lectura.print(HbagC,1);
    lectura.println("");

  }
  // Cierre del archivo de escritura
  lectura.close();
}
