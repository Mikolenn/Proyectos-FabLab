// Librerías del proyecto
#include <LiquidCrystal_I2C.h>

//Inicialización de la pantalla LCD 16x2
LiquidCrystal_I2C pant(0x27, 16, 2);

// Variables para las lecturas
bool readButton = 7;
int sensor = A0;

// Calibración del sensor específica del proyecto
float m = -0.21567;
float b = 206.86;


// Función de set up
void setup() {

  // Inicialización de la pantalla
  initPant();

  // Declaración del comportamiento de las terminales de Arduino
  pinMode(readButton, INPUT);
  pinMode(sensor, INPUT);
  analogReference(EXTERNAL);
}


// Ciclo loop
void loop() {
  
  // Lectura de las variables de interés
  bool button = digitalRead(readButton);
  unsigned int value = analogRead(sensor);

  // Cálculo del valor real en función de la calibración
  float y = m * value + b;

  // Actualización de los valores en pantalla
  pant.print("Humedad ");
  pant.setCursor(0, 1);
  pant.print(value);
  pant.print("  ");
  pant.print(y);
  pant.print(" %");
  delay(200);
  
  // Tiempo de espera en caso de presionar el botón
  if(button)
  {
    delay(5000);
  }
  
  // Limpieza de la pantalla para la siguiente iteración
  pant.clear();
}

// Función encargada de inicializar la pantalla LCD 16x2
void initPant() {

  // Inicialización de la pantalla y activación de su iluminación
  pant.init();
  pant.backlight();
  pant.begin(16, 2);
  pant.home();
  
  // Mensaje inicial
  pant.print("SENSOR DE HUMEDAD");
  pant.setCursor(0, 1);  
  pant.print("COLOQUE EL SENSOR Y PRESIONE EL BOTON");  

  delay(500);
  pant.home();
  delay(750);
  
  // Desplazamiento del mensaje hacia la izquierda
  for(int i=0; i < 21; i++)
  {
  	pant.scrollDisplayLeft();
    delay(300);
  }
  
  // Regreso del cursor de la pantalla a su posición original
  delay(750);
  pant.home();
}
