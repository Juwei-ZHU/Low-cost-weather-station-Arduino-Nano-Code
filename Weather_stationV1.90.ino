#include <Wire.h>
#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <DHT.h>;
#include <SoftwareSerial.h>

#define DHTPIN 6     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

String Version = "V1.90";
unsigned int INTERVAL = 60;
const byte rainPin = 3;
const byte windSpeedPin = 2;
const byte windDirPin = A0;
//int Riantiptime[10]={0};
String Send;
String Header = "Time; Temperature; Humidity; Solar radiation; Rainfall tips; Wind tips; Wind direction; Visable light; IR light; UV light; Raw pyranometer data; software version";

/////////////////////////////////////////////////////////////////////
long debouncing_time = 10; //Debouncing Time in Milliseconds for anemometer
long debouncing_time_rain_gauge = 1000; //Debouncing Time in Milliseconds for rain gauge
volatile unsigned long last_micros_wind;
volatile unsigned long last_micros_rain;
unsigned int windcnt = 0;
unsigned int raincnt = 0;
unsigned long lastSend;


// Create BME280 instance
//Adafruit_BME280 bme; 

//Pyranometer command 
unsigned char item[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};  //Hex query command
String data = ""; // Hex string received
SoftwareSerial RadiaSerial(7, 8);  // RX, TX
float CalRadiation(String Radiaerature); 


#include <Wire.h>
#include "Adafruit_SI1145.h"

Adafruit_SI1145 uv = Adafruit_SI1145();

//////////////// SETUP //////////////////////////////////////////////
void setup() {
  
    RadiaSerial.begin(9600);
    Serial.begin(9600);
    while(!Serial);
    Serial.println(Header);
    dht.begin();
    uv.begin();

    pinMode(windSpeedPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(windSpeedPin),InterruptWind,RISING);


    pinMode(rainPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(rainPin),InterruptRain,RISING);


    pinMode(windDirPin, INPUT);  

    GetAndSendRadiation();
    
    lastSend = millis();
   
}

//////////////// LOOP //////////////////////////////////////////////
void loop() 
{
     
  if ( millis() - lastSend > INTERVAL*1000 ) { 
    
    getAndSendData(); 

    lastSend = millis();
}

}
//////////////// Functions //////////////////////////////////////////

void getAndSendData()
{  


  float Temperature = dht.readTemperature();
  float Humidity = dht.readHumidity();
  float Radiation = GetAndSendRadiation();
  float Rainfall = GetAndSendRainfall();
  float WindSpeed = GetAndSendWindSpeed();
  String WindDirection = GetAndSendWindDirection();
  float Vis = uv.readVisible();
  float IR = uv.readIR();
  float UV = uv.readUV()/100.0;

  Send = " ;" + String(Temperature) + "°C; ";
  Send = Send + String(Humidity, 2) + "%; ";
  Send = Send + String(Radiation, 0) + "W/m2; ";
  Send = Send + String(Rainfall, 0) + "tR/min; ";
  Send = Send + String(WindSpeed, 0) + "tW/min; ";
  Send = Send + WindDirection + "; ";
  Send = Send + "Vis " + String(Vis,2) + "; ";
  Send = Send + "IR " + String(IR,2) + "; ";
  Send = Send + "UV " + String(UV,2) + "; ";
  Send = Send + data + "; "; 
  Send = Send + Version;

  Serial.println(Send);
 // memset(Riantiptime,0,sizeof(Riantiptime));
}

float GetAndSendRadiation() 
{
  
  for (int i = 0 ; i < 8; i++) {  // Send inquiry command
    RadiaSerial.write(item[i]);   // write output
  }
  //delay(100);  // Wait for data to return
  data = "";
  while (RadiaSerial.available()) {//Read data from the serial port
    unsigned char in = (unsigned char)RadiaSerial.read();  // read 
    //Serial.print(in, HEX);
    //Serial.print(',');
    data += in;
    data += ',';
  }

  if (data.length() > 0) { //output the received data
    return CalRadiation(data);
  }   
  }

float CalRadiation(String Radia) {
  int commaPosition = -1;
  String info[9];  // Use string array to storage
  for (int i = 0; i < 9; i++) {
    commaPosition = Radia.indexOf(',');
    if (commaPosition != -1)
    {
      info[i] = Radia.substring(0, commaPosition);
      Radia = Radia.substring(commaPosition + 1, Radia.length());
    }
    else {
      if (Radia.length() > 0) {   
        info[i] = Radia.substring(0, commaPosition);
        
      }
    }
  }
  return (info[3].toInt() * 256 + info[4].toInt());
}

float GetAndSendRainfall() 
{
  float Rainfall = raincnt;
  raincnt = 0;
  return Rainfall; 
}

float GetAndSendWindSpeed() 
{
  float WindSpeed = windcnt;
  windcnt = 0;
  return WindSpeed; 
}

String GetAndSendWindDirection() 
{

  float dirpin = analogRead(windDirPin)*(4.65 / 1023.0);
    String WindDirection;
  
  if(dirpin > 3.10 &&  dirpin < 3.70 ){
    WindDirection = "N";
  }
  if(dirpin > 3.70 &&  dirpin < 4.10 ){
    WindDirection = "NW";
  }
  if(dirpin > 4.10 &&  dirpin < 4.40 ){
    WindDirection = "W";
  }
  if(dirpin > 2.50 &&  dirpin < 3.10 ){
    WindDirection = "SW";
  }
  if(dirpin > 0.90 &&  dirpin < 1.50 ){
    WindDirection = "S";
  }
  if(dirpin > 0.40 &&  dirpin < 1.00 ){
    WindDirection = "SE";
  }
  if(dirpin > 0.10 &&  dirpin < 0.70 ){
    WindDirection = "E";
  }
  if(dirpin > 1.60 &&  dirpin < 2.20 ){
    WindDirection = "NE";
  }    
  return WindDirection;
  
}
// debounceFunction
void InterruptWind() {
  if((long)(micros() - last_micros_wind) >= debouncing_time * 1000) {
     if (digitalRead(windSpeedPin)==HIGH)
  {
  windcnt++;
//  Serial.println(windcnt);
  last_micros_wind = micros();
  } 
  }
}
void InterruptRain() {
  if((long)(micros() - last_micros_rain) >= debouncing_time_rain_gauge * 1000) {
   if (digitalRead(rainPin)==HIGH)
  {
 // Riantiptime[raincnt]= millis()-lastSend;
  raincnt++;
  Serial.print(raincnt);
  Serial.println(";");
  last_micros_rain = micros();
  }
  }  
}
