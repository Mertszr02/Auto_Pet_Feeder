/*********

  Create a wifi access point (ESPap) (pass:12345678) [You can change both]
  Start a server at (192.168.4.1)
  Put 4 text boxes, a checkBox and a submit button on the screen 
  when submit is pressed save user inputs
  
  ---------------------------------------------------
  
  When the set time is reached:
    Open the door to feed the pet
    Close door
    İf the door does not close(stuck food):
      open slightly then try closing again (do this until fully closed)
    succesfully fed the pet wait for the next feeding time
  ---------------------------------------------------
  Feeding system:
  Servo signal pin is connected to pin 2 
  servo turnes until open door pin detected
  delay set amount 
  servo turnes back until closed door pin detected
  ---------------------------------------------------
  
by Mert Sezer 2020
  
*********/

#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
//----------------------------
#include <Servo.h>

Servo myservo;
//----------------------------
#include <Wire.h>
#include <RTC.h>

static DS3231 RTC;
//----------------------------

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK  "12345678"
#endif

String inputMessage = "0";
String inputMessage2 = "0";
String inputMessage3 = "0";
String inputMessage4 = "0";
String enableArmChecked = "checked";
String inputMessage5 = "true";


const char *ssid = APSSID;
const char *password = APPSK;

//const int buttonPin = 6;
const int spd = 2; // aç kapa hızı
const int pin1 = 0;
const int pin2 = 2;

int pos = 90;   // servo kapalı pozisyonu
int bir;    // kapak kapalı buton
int iki;    // kapak açık buton
int tekrar = 0; // sıkışma engelleme tekrar sayısı max: 8
//int buttonState = 0;

int saat;
int dakika;
int saniye;

int besle1;
int besle2;
int besle3;
int besle4;

AsyncWebServer server(80);


//-----------------------------------------------------------------



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Temperature Threshold Output Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h2>Selam</h2> 
  <h3>Beslenme saatlerini yaz</h3>
  <h4>24 saat bicimi kullan</h4>
  
  <form action="/get">
    1. Besleme saat; <input type="number" step="0.1" name="saat_bir" placeholder="saat" min="0" max="23" required><br>
    Dakika; <input type="number" step="0.1" name="dakika_bir" placeholder="dakika" min="0" max="60" required><br><br>
    ----------------------------<br><br>
    2. Besleme saat <input type="number" step="0.1" name="saat_iki" placeholder="saat" min="0" max="23" required><br>
    Dakika; <input type="number" step="0.1" name="dakika_iki" placeholder="dakika" min="0" max="60" required><br>

    light On/Off <input type="checkbox" name="enable_arm_input" value= %ENABLE_ARM_INPUT%><br><br>
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";



//---------------------------------------------------------------


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


const char* PARAM_INPUT_1 = "saat_bir";
const char* PARAM_INPUT_2 = "dakika_bir";
const char* PARAM_INPUT_3 = "saat_iki";
const char* PARAM_INPUT_4 = "dakika_iki";
const char* PARAM_INPUT_5 = "enable_arm_input";






//-------------------------------------------------------------
void setup() {
  delay(500);
  
  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(buttonPin, INPUT);
  pinMode(13, OUTPUT); 
  pinMode(pin1, INPUT);
  pinMode(pin2, INPUT);

  delay(500);
  
  Serial.begin(115200);

//---------------------------------------------------------------

  RTC.begin();

  Serial.print("Is Clock Running: ");
  if (RTC.isRunning())
  {
    Serial.println("Yes");
    Serial.print(RTC.getDay());
    Serial.print("-");
    Serial.print(RTC.getMonth());
    Serial.print("-");
    Serial.print(RTC.getYear());
    Serial.print(" ");
    Serial.print(RTC.getHours());
    Serial.print(":");
    Serial.print(RTC.getMinutes());
    Serial.print(":");
    Serial.print(RTC.getSeconds());
    Serial.print("");
    Serial.println("");
    delay(1000);

    int day = RTC.getDay();
  }
  else
  {
    delay(1500);

    Serial.println("No");
    Serial.println("Setting Time");
    //RTC.setHourMode(CLOCK_H12);
    RTC.setHourMode(CLOCK_H24);
    RTC.setDateTime(__DATE__, __TIME__);
    Serial.println("New Time Set");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    RTC.startClock();
  }

 
//--------------------------------------------------------------------

  myservo.attach(12);
//---------------------------------------------------------------------
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("HTTP server started");




  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {

    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
    }
    
    if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
    }
     
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
    }
    
    if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage4 = request->getParam(PARAM_INPUT_4)->value();
    }

    if (request->hasParam(PARAM_INPUT_5)) {
        inputMessage5 = request->getParam(PARAM_INPUT_5)->value();
        enableArmChecked = "checked";
    }
      
    else {
        inputMessage5 = "false";
        enableArmChecked = "";
    }
    Serial.println(inputMessage);  // saat1
    Serial.println(inputMessage2); // dakika1
    Serial.println(inputMessage3); // saat2
    Serial.println(inputMessage4); // dakika2
    Serial.println(inputMessage5); // buton

    besle1 = inputMessage.toInt();
    besle2 = inputMessage2.toInt();
    besle3 = inputMessage3.toInt();
    besle4 = inputMessage4.toInt();
    
    
    
    request->send(200, "text/html", "<h1>Besleme Saatleri kaydedildi</h1> <br><a href=\"/\"> <h2>Ana Sayfa<h2></a>");
  });
  
  server.onNotFound(notFound);
  server.begin();
  
  myservo.write(90);
  delay(400);
  myservo.write(80);
  delay(400);
  myservo.write(90);

  delay(1000);

  feed();

}


void loop() {
  if (inputMessage5 == "false"){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else{
    digitalWrite(LED_BUILTIN, LOW);
  }
  //buttonState = digitalRead(buttonPin);
  delay(50);

  bir = digitalRead(pin1); //kapalı
  iki = digitalRead(pin2); //açık

  saat = RTC.getHours();
  dakika = RTC.getMinutes();
  saniye = RTC.getSeconds();

  if(saat == besle1 && dakika == besle2 && saniye <= 5)
  {
    feed();
    delay(10000);
  }
  
  if(saat == besle3 && dakika == besle4 && saniye <= 5)
  {
    feed();
    delay(10000);
  }
  Serial.println("  ");
  Serial.print(saat);
  Serial.print(" : ");
  Serial.print(dakika);
  Serial.print(" : ");
  Serial.print(saniye);
  Serial.print("   -  ");
  Serial.print(besle1);
  Serial.print(" : ");
  Serial.print(besle2);
  Serial.print("   -  ");
  Serial.print(besle3);
  Serial.print(" : ");
  Serial.print(besle4);
  Serial.println("  ");
  delay(50);
}



void feed(){
  Serial.println("Start");

  while (true){
    bir = digitalRead(pin1);//kapalı
    iki = digitalRead(pin2);//açık
    
    pos = pos - spd;
    myservo.write(pos);
    delay(20);
    if(iki == LOW){
      break;
    }
    else if(pos < 30 ){
      Serial.println("Door Open Error.");
      break;
    }
  }

  while (true){
    
    pos = pos + spd;
    myservo.write(pos);
    Serial.println(pos);
    delay(4);
    
    bir = digitalRead(pin1);//kapalı
    iki = digitalRead(pin2);//açık
    
    if(bir == LOW){
      break;
    }
    else if(pos >= 136 && tekrar < 8){
      pos = 55;
      myservo.write(pos);
      Serial.println(pos);
      tekrar++;  
      delay(5);
    }
    else if (tekrar >= 8){
      myservo.write(75);
      break;
    }
  }

  if(tekrar < 8){
    myservo.write(89);
    digitalWrite(LED_BUILTIN, LOW); 
  }

  Serial.println("Finish");
}
