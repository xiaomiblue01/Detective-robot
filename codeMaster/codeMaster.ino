// Enable debug console
// Set CORE_DEBUG_LEVEL = 3 first
//#define ERA_DEBUG

#define DEFAULT_MQTT_HOST "mqtt1.eoh.io"
// You should get Auth Token in the ERa App or ERa Dashboard
#define ERA_AUTH_TOKEN "c5a89487-5494-4286-ae78-58ba5ce664d6"

#include <Arduino.h>
#include <ERa.hpp>
#include <ERa/ERaTimer.hpp>

#define M0 5       
#define M1 18
#define leftRightPin_DC 34  
#define upDownPin_DC 35 
#define upDownPin_SV 33
#define leftRightPin_SV 32
#define btnOn 22
#define btnOff 23
#define ledConnectPin 2
//-----------------------------------------
int valueData;
bool dirUpDown = false;
int speedUpDown = 0;
int positionLeftRight_DC = 0;
int positionUpDown_SV = 0;
int positionLeftRight_SV = 0;
//-----------------------------------------
int statusButtonOn = 0;
int lastStatusBtnOn = 0;
int statusButtonOff = 0;
int lastStatusBtnOff = 0;
//-----------------------------------------
double temp = 10; 
double humi = 11;
String latitudeGPS = "Waitting Data";
String longitudeGPS = "Waitting Data";
String dateGPS = "Waitting Data";
String timeGPS = "Waitting Data";
//-----------------------------------------
String dataControl ="";
String dataFeedBack = "";
//-----------------------------------------
unsigned long nowTime = 0;
unsigned long lastTimeSendData = 0;
//-----------------------------------------
unsigned long lastTimeBlinkLed = 0;
unsigned long lastTimeCheckConnect = 0;
//-----------------------------------------
String dataCheckConnect = "Fail";
bool statusConnect = false;
bool statusLedConnect = false;
//-----------------------------------------
const char ssid[] = "Automation Control 2.4Ghz";
const char pass[] = "04051978";

/* This function print uptime every second */
ERaTimer timer;

void timerEvent() {
    ERA_LOG("Timer", "Uptime: %d", ERaMillis() / 1000L);
}
//-------------------------------------------------------------------------------------------
void setup() {
  Serial2.begin(9600);   // lora E32 gắn với cổng TX2 RX2 trên board ESP32
  Serial.begin(9600);
//-----------------------------------------
  ERa.begin(ssid, pass);
  timer.setInterval(1000L, timerEvent);
//-----------------------------------------
  pinMode(M0, OUTPUT);        
  pinMode(M1, OUTPUT);
  pinMode(btnOn, INPUT_PULLUP);   
  pinMode(btnOff, INPUT_PULLUP);        
  pinMode(ledConnectPin, OUTPUT);
  digitalWrite(M0, LOW);       // Set 2 chân M0 và M1 xuống LOW 
  digitalWrite(M1, LOW);       // để hoạt động ở chế độ Normal
  digitalWrite(ledConnectPin, statusLedConnect); 
}
//-------------------------------------------------------------------------------------------
void loop() {
  nowTime = millis();
  ERa.run();
  timer.run();

  ControlMotor(upDownPin_DC, speedUpDown);
  ControlMotor(leftRightPin_DC, positionLeftRight_DC);
  ControlMotor(upDownPin_SV, positionUpDown_SV);
  ControlMotor(leftRightPin_SV, positionLeftRight_SV);
  readButton(btnOn, statusButtonOn, lastStatusBtnOn);
  readButton(btnOff, statusButtonOff, lastStatusBtnOff);

  SendData();
  ReceiveData();
  checkConnect();
  
}
//-------------------------------------------------------------------------------------------
void checkConnect()
{
if (nowTime - lastTimeCheckConnect >= 5000) {
    if (dataCheckConnect == "Arduino") {
      dataCheckConnect = "";
      statusConnect = true;
      digitalWrite(ledConnectPin, HIGH);
    } else {
      statusConnect = false;
    }
    lastTimeCheckConnect = nowTime; 
    if(!statusConnect){
      blink(500);
    }
  }
}
//-------------------------------------------------------------------------------------------
void blink(int time)  //ms
{
  if(nowTime - lastTimeBlinkLed >= time)
  {
    statusLedConnect = !statusLedConnect;
    digitalWrite(ledConnectPin, statusLedConnect); 
    lastTimeBlinkLed = nowTime;
  }
}
//-------------------------------------------------------------------------------------------
void readData()
{
  if(Serial2.available() > 1){
    String input = Serial2.readString();
    Serial.println(input);    
  }
  delay(20);
}
//-------------------------------------------------------------------------------------------
void SendData(){
// ~dirUpDown!speedUpDown@positionLeftRight_DC#positionUpDown_SV$positionLeftRight_SV%statusButtonLeft^statusButtonRight&ESP
  dataControl = "";
  dataControl += "~";
  dataControl += String(dirUpDown);
  dataControl += "!";
  dataControl += String(speedUpDown);
  dataControl += "@";
  dataControl += String(positionLeftRight_DC);
  dataControl += "#";
  dataControl += String(positionUpDown_SV);
  dataControl += "$";
  dataControl += String(positionLeftRight_SV);
  dataControl += "%";
  dataControl += String(statusButtonOn);
  dataControl += "^";
  dataControl += String(statusButtonOff);
  dataControl += "&";
  dataControl += String("ESP");
  dataControl += "*\n";
  if(nowTime - lastTimeSendData>= 1000)
  {
    Serial2.print(dataControl);
    sendDataToServer();
    lastTimeSendData = nowTime;
  }    
}
//-------------------------------------------------------------------------------------------
void ReceiveData() {
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    if (c == '\n') {
      parseData();
      Serial.println(dataCheckConnect);
      dataFeedBack = "";
    } else {
      dataFeedBack += c; // Thêm ký tự vào buffer
    }
  }
}
//-------------------------------------------------------------------------------------------
void parseData() {
  temp =  getDoubleValue('~', '!');
  humi = getDoubleValue('!', '@');
  latitudeGPS = getStringValue('@', '#');
  longitudeGPS = getStringValue('#', '$');
  dateGPS = getStringValue('$', '%');
  timeGPS = getStringValue('%', '^');
  if(getStringValue('^', '&') != "0")
    dataCheckConnect = getStringValue('^', '&');
}
//-------------------------------------------------------------------------------------------
double getDoubleValue(char start, char end) {
  double value = 0.0;
  int indexStart = dataFeedBack.indexOf(start);
  int indexEnd = dataFeedBack.indexOf(end);
  if (indexStart != -1 && indexEnd != -1) {
    String valueStr = dataFeedBack.substring(indexStart + 1, indexEnd);
    value = valueStr.toDouble();
  }
  return value;
}
//-------------------------------------------------------------------------------------------
String getStringValue(char start, char end) {
  String valueStr = "";
  int indexStart = dataFeedBack.indexOf(start);
  int indexEnd = dataFeedBack.indexOf(end);
  if (indexStart != -1 && indexEnd != -1) {
    valueStr = dataFeedBack.substring(indexStart + 1, indexEnd);
  }
  return valueStr;
}
//-------------------------------------------------------------------------------------------
void ControlMotor(int pin, int &value)
{
  int data_canche = 0;
  data_canche = analogRead(pin);
  valueData = data_canche;
  if(pin == upDownPin_DC ){
    if(data_canche <= 1700)
    {
      dirUpDown = false;
      value = map(data_canche, 0, 1800, 255, 0);
    }
    if(data_canche > 1700 && data_canche < 1900)
    {
      dirUpDown = false;
      value = 0;
    }
    if(data_canche >= 1900)
    {
      dirUpDown = true;
      value = map(data_canche, 1900, 4095, 0, 255);
    }
  }
  else{
    if(data_canche <= 1800)
    {
      value = map(data_canche, 0, 1800, 180, 91);
    }
    if(data_canche > 1800 && data_canche < 1900)
    {
      value = 90;
    }
    if(data_canche >= 1900)
    {
      value = map(data_canche, 1900, 4095, 89, 0);
    }
  }
}
//-------------------------------------------------------------------------------------------
void readButton(int pinButton, int &value, int &lastStatusButton)
{
  int statusButton = digitalRead(pinButton);
  if (statusButton == LOW && lastStatusButton == HIGH) {
    value = 1;
  }
  else {
    value = 0;
  }
  lastStatusButton = statusButton;
}
//-------------------------------------------------------------------------------------------
void sendDataToServer()
{
  ERa.virtualWrite(V0, positionLeftRight_DC); 
  ERa.virtualWrite(V1, speedUpDown); 
  ERa.virtualWrite(V2, positionUpDown_SV); 
  ERa.virtualWrite(V3, positionLeftRight_SV);   
  ERa.virtualWrite(V4, latitudeGPS.c_str()); //latitudeGPS
  ERa.virtualWrite(V5, longitudeGPS.c_str()); 
  ERa.virtualWrite(V6, dateGPS.c_str()); 
  ERa.virtualWrite(V7, timeGPS.c_str()); 
  ERa.virtualWrite(V8, temp); 
  ERa.virtualWrite(V9, humi); 
}