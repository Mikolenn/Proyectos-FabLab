// Librerías del proyecto
#include <Wire.h>

// Declaración de las variables para el control del tiempo
float Ttotal = 1.0;
long long horas = 0.0;

// Declaración de las variables para los potenciómetros
int potRiego = A0;
int potReposo = A1;
int potTotal = A2;

// Declaración de la variable para activar la válvula
int valv = 7;


// Función set up
void setup() {
  
  // Declaración del comportamiento de las terminales del Arduino
  pinMode(potRiego, INPUT);
  pinMode(potReposo, INPUT);
  pinMode(potTotal, INPUT);
  pinMode(valv, OUTPUT);
}


// Ciclo loop
void loop() {
  
  // Valor temporal del inicio del ciclo
  unsigned long inicio = millis();

  // Verificación para determinar si aun es tiempo de riego
  if( horas < Ttotal ){
    
    // Lectura del tiempo de riego y conversión a segundos
    unsigned int riego = analogRead(potRiego);
    float Triego = (float(riego) * 10/1023) * 1000*60;

    // Lectura del tiempo de reposo y conversión a segundos
    unsigned int reposo = analogRead(potReposo);
    float Treposo = (float(reposo) * 60/1023) * 1000*60;

    // Lectura del tiempo total de riego y conversión a segundos
    unsigned int total = analogRead(potTotal);
    Ttotal = (float(total) * 12/1023) * 1000*3600;
    
    // Activación y desactivación de la válvula con base en los tiempos leídos
    digitalWrite(valv, HIGH);
    delay(Triego);
    digitalWrite(valv, LOW);
    delay(Treposo);
  }
  // Cálculo de las 24 horas del día (1000*60*60*24)
  long long diaCompleto = 86400000UL;

  // Verificación del tiempo total transcurrido
  if( horas > diaCompleto ){
    horas = 0;
  }

  delay(1000);

  // Valor temporal del final del ciclo
  unsigned long final = millis();
  
  // Cálculo del total de horas transcurridas
  horas = horas + abs(final-inicio);
}
