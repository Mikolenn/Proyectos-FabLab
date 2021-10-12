// Librerías del proyecto
#include <RTClib.h>

// Tiempo de riego. Ingrese el valor en minutos
float tRiego = 1;

// Tiempo de reposo. Ingrese el valor en minutos
int tReposo = 15;

// Hora de inicio. Ingrese el valor en horas
int inicio = 6;

// Hora de finalización. Ingrese el valor en horas
int fin = 18;

// Variable para la válvula
int valv = 2;

// Variable para la operación del reloj en tiempo real
RTC_DS3231 rtc;


// Función set up
void setup() {
  
  // Inicialización del reloj en tiempo real
  rtc.begin();

  /* Sincronización del reloj en tiempo real
   * Para sincronizar el reloj en tiempo real, descomente la siguiente línea 
   * y cargue el programa. Luego de sincronizar, comente nuevamente la línea 
   * y cargue nuevamente el programa
   */
  //rtc.adjust( DateTime( F(__DATE__), F(__TIME__) ) );

  // Declaración del comportamiento de la terminal del Arduino
  pinMode(valv, OUTPUT);
}


// Ciclo loop
void loop() {

  // Lectura de la fecha y hora actual y, cálculos necesarios
  DateTime actual = rtc.now();
  int mInt = actual.minute() % tReposo;

  // Tiempo de delay para el tiempo de riego
  unsigned long temp = tRiego*60000;

  // Se cumple la condición según el intervalo elegido y el horario
  if ( (actual.minute() == 0 || mInt == 0) && (inicio < actual.hour() ) && (actual.hour() < fin ) ) {
    
    // Activación y desactivación de la válvula con base en el tiempo leído
    digitalWrite(valv, HIGH);
    delay(temp);
    digitalWrite(valv, LOW);
  }
}
