/*
 Application:
 – Interface water flow sensor with ESP32 board.
 
 Board:
 – ESP32 Dev Module
 https://my.cytron.io/p-node32-lite-wifi-and-bluetooth-development-kit
 Sensor:
 – G 1/2 Water Flow Sensor
 https://my.cytron.io/p-g-1-2-water-flow-sensor
 */
#include <WiFi.h>  // Biblioteca para el control de WiFi
#include <PubSubClient.h> //Biblioteca para conexion MQTT
#include "string.h"

#define LED_BUILTIN 2
#define SENSOR 14

//Datos de WiFi
const char* ssid = "**********";  // Aquí debes poner el nombre de tu red
const char* password = "**********";  // Aquí debes poner la contraseña de tu red

//Datos del broker MQTT
const char* mqtt_server = "18.158.198.79"; // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica 18.158.198.79
IPAddress server(18,158,198,79);

// Objetos
WiFiClient espClient; // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient); // Este objeto maneja los datos de conexion al broker

int flashLedPin = 4;  // Para indicar el estatus de conexión
int statusLedPin = 33; // Para ser controlado por MQTT
 
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
std::string json = "{ \"id\": \"sensorVick\" \n \"location\": \"19.232424,-91.4939\" \n \"value\": ";
std::string msj = "";
 
void IRAM_ATTR pulseCounter()
{
 pulseCount++;
}
 
void setup()
{
 Serial.begin(115200);
 
 pinMode(LED_BUILTIN, OUTPUT);
 pinMode(SENSOR, INPUT_PULLUP);
 
 pulseCount = 0;
 flowRate = 0.0;
 flowMilliLitres = 0;
 totalMilliLitres = 0;
 previousMillis = 0;
 
 attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

 Serial.println();
 Serial.println();
 Serial.print("Conectar a ");
 Serial.println(ssid);
 
 WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi
 
 while (WiFi.status() != WL_CONNECTED) { // Este bucle espera a que se realice la conexión
   digitalWrite(flashLedPin, HIGH);
   delay(500); //dado que es de suma importancia esperar a la conexión, debe usarse espera bloqueante
   digitalWrite(flashLedPin, LOW);
   Serial.print(".");  // Indicador de progreso
   delay (5);
 }
  
 // Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
 Serial.println();
 Serial.println("WiFi conectado");
 Serial.println("Direccion IP: ");
 Serial.println(WiFi.localIP());

 // Si se logro la conexión, encender led
 if(WiFi.status () > 0){
   digitalWrite (flashLedPin, HIGH);
 }
  
 delay (1000); // Esta espera es solo una formalidad antes de iniciar la comunicación con el broker

 // Conexión con el broker MQTT
 client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
 client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos
 delay(1500);  // Esta espera es preventiva, espera a la conexión para no perder información
  
}
 
void loop()
{

 //Verificar siempre que haya conexión al broker
 if (!client.connected()) {
   reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
 }// fin del if (!client.connected())
 client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker
 
 currentMillis = millis();
 if(currentMillis - previousMillis > interval) {
 
  pulse1Sec = pulseCount;
  pulseCount = 0;
 
  // Because this loop may not complete in exactly 1 second intervals we calculate
  // the number of milliseconds that have passed since the last execution and use
  // that to scale the output. We also apply the calibrationFactor to scale the output
  // based on the number of pulses per second per units of measure (litres/minute in
  // this case) coming from the sensor.
  flowRate = ( (1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
  previousMillis = millis();
 
   // Divide the flow rate in litres/minute by 60 to determine how many litres have
  // passed through the sensor in this 1 second interval, then multiply by 1000 to
  // convert to millilitres.
  flowMilliLitres = (flowRate / 60) * 1000;
 
   // Add the millilitres passed in this second to the cumulative total
  totalMilliLitres += flowMilliLitres;
 
  // Print the flow rate for this second in litres / minute
  Serial.print("Flow rate: ");
  Serial.print(flowRate); // Print the integer part of the variable
  Serial.print("L/min");
  Serial.print("\t"); // Print tab space
 
  // Print the cumulative total of litres flowed since starting
  Serial.print("Output Liquid Quantity: ");
  Serial.print(totalMilliLitres);
  Serial.print("mL / ");
  Serial.print(totalMilliLitres / 1000);
  Serial.println("L");
  msj = json + " \"" + std::to_string(flowRate) + "\" }";
  client.publish("codigoIoT/leakless/esp32cam", msj.c_str() );
 }
}


// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* message, unsigned int length) {

  // Indicar por serial que llegó un mensaje
  Serial.print("Llegó un mensaje en el tema: ");
  Serial.print(topic);

  // Concatenar los mensajes recibidos para conformarlos como una varialbe String
  String messageTemp; // Se declara la variable en la cual se generará el mensaje completo  
  for (int i = 0; i < length; i++) {  // Se imprime y concatena el mensaje
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  // Se comprueba que el mensaje se haya concatenado correctamente
  Serial.println();
  Serial.print ("Mensaje concatenado en una sola variable: ");
  Serial.println (messageTemp);

  // En esta parte puedes agregar las funciones que requieras para actuar segun lo necesites al recibir un mensaje MQTT

  // Ejemplo, en caso de recibir el mensaje true - false, se cambiará el estado del led soldado en la placa.
  // El ESP323CAM está suscrito al tema esp/output
  if (String(topic) == "esp32/output") {  // En caso de recibirse mensaje en el tema esp32/output
    if(messageTemp == "true"){
      Serial.println("Led encendido");
      digitalWrite(statusLedPin, LOW);
    }// fin del if (String(topic) == "esp32/output")
    else if(messageTemp == "false"){
      Serial.println("Led apagado");
      digitalWrite(statusLedPin, HIGH);
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "esp32/output")
}// fin del void callback

// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      client.subscribe("esp32/output"); // Esta función realiza la suscripción al tema
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      // Espera de 5 segundos bloqueante
      delay(5000);
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect(
