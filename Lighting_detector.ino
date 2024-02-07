#include <WiFiClientSecure.h>                
#include <PubSubClient.h>
#include <Wire.h>
#include <SparkFun_AS3935.h>
#include <DHT.h>                  

String WiFi_SSID = "WiFi SSID";						//WiFi SSID
String WiFi_PW =  "WiFi Password";					//WiFI Password
const char* mqttServer = "MQTT Server";				//MQTT Server
const int mqttPort = 1883;							//MQTT Port
const char* mqttUser = "mqtt Username"; 			//MQTT Username
const char* mqttPassword = "mqtt Password";     	//MQTT Passowrt

unsigned long waitCount = 0;                   
uint8_t conn_stat = 0;                      
unsigned long lastStatus = 0;
unsigned long lastTask = 0;           

const char* Status = "{\"Message\":\"ich laufe\"}";	//MQTT Status message
 
long lastMsg = 0;
char msg[50];
int value_1 = 0;
long time_1 = 0;
#define AS3935_ADDR 0x03 
#define INDOOR 0x12 
#define OUTDOOR 0xE
const int lightningInt = 4; 
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01




int noiseFloor = 2;
int intVal = 0;
int periode = 1000;
float tempair = 0;                                                       
char chartempair[] = "0.00";
float humid = 0;
char charhumid[] = "0.00";

WiFiClientSecure TCP;                        
WiFiClient espClient;
PubSubClient client(espClient);
SparkFun_AS3935 lightning(AS3935_ADDR);
DHT dht(32,DHT22); 


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);                                            
  pinMode(lightningInt, INPUT); 
  delay(500);
  Wire.begin(); 
  lightning.begin();
  lightning.setIndoorOutdoor(OUTDOOR);
  dht.begin(); 
 }



void loop() {                                                     
// start of non-blocking connection setup section
  if ((WiFi.status() != WL_CONNECTED) && (conn_stat != 1)) { conn_stat = 0; }
  if ((WiFi.status() == WL_CONNECTED) && !client.connected() && (conn_stat != 3))  { conn_stat = 2; }
  if ((WiFi.status() == WL_CONNECTED) && client.connected() && (conn_stat != 5)) { conn_stat = 4;}
  switch (conn_stat) {
    case 0:                                                       // MQTT and WiFi down: start WiFi
      Serial.println("MQTT and WiFi down: start WiFi");
      WiFi.begin(WiFi_SSID.c_str(), WiFi_PW.c_str());
      conn_stat = 1;
      break;
    case 1:                                                       // WiFi starting, do nothing here
      Serial.println("WiFi starting, wait : "+ String(waitCount));
      waitCount++;
      break;
    case 2:                                                       // WiFi up, MQTT down: start MQTT
      Serial.println("WiFi up, MQTT down: start MQTT");
      client.setServer(mqttServer, mqttPort);         
      client.connect("ESP32Client", mqttUser, mqttPassword );
      conn_stat = 3;
      waitCount = 0;
      break;
    case 3:                                                       // WiFi up, MQTT starting, do nothing here
      Serial.println("WiFi up, MQTT starting, wait since: "+ String(waitCount));
      waitCount++;
      break;
    case 4:                                                       // WiFi up, MQTT up: finish MQTT configuration
      Serial.println("WiFi up, MQTT up: finish MQTT configuration");
      client.publish("Blitzsensor/Status", "MQTT Verbunden");
      conn_stat = 5;                    
      break;
  }
// end of non-blocking connection setup section


// start section with tasks where WiFi/MQTT is required
  if (conn_stat == 5) {
    if (millis() - lastStatus > 300000) {                            
      Serial.println(Status);
      client.publish("Blitzsensor/Status", "MQTT Alive");
      client.publish("Blitzsensor/Disturber", "-");
      client.publish("Blitzsensor/Noise", "-");
      client.publish("Blitzsensor/Blitzerkennung", "-");
      client.publish("Blitzsensor/Blitzentfernung", "-");
      tempair = dht.readTemperature();                                  
      dtostrf(tempair, 2, 2, chartempair);
      humid = dht.readHumidity();                                       
      dtostrf(humid, 4, 1, charhumid);
      Serial.print("Lufttemperatur: ");                                             
      Serial.println(tempair);
      client.publish("Blitzsensor/Lufttemperatur", chartempair);
      Serial.print("Luftfeuchtigkeit: ");
      Serial.println(humid);
      client.publish("Blitzsensor/Luftfeuchtigkeit", charhumid);
      lastStatus = millis();
                        
    }
       
  if(digitalRead(lightningInt) == HIGH){
    intVal = lightning.readInterruptReg();
    if(intVal == NOISE_INT){
      client.publish("Blitzsensor/Noise", "Rauschen empfangen");
      Serial.println("Rauschen!");
      lastStatus = millis();    
    }
    else if(intVal == DISTURBER_INT){
      client.publish("Blitzsensor/Disturber", "Störung empfangen");
      Serial.println("Störung!");
      lastStatus = millis();     
    }
    else if(intVal == LIGHTNING_INT){
      client.publish("Blitzsensor/Blitzerkennung", "Blitz erkannt");
      Serial.println("Blitz erkannt");
      byte distance = lightning.distanceToStorm(); 
      char s [20];
      sprintf (s, "%d", lightning.distanceToStorm());
      client.publish("Blitzsensor/Blitzentfernung", (const char*) s);
      lastStatus = millis();     
      }
  }



  //Ende Programm
  } 
   
  client.loop();
}
