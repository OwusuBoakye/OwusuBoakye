#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include "time.h"
#include <DHT.h>

#ifndef APSSID
#define APSSID "NodeMCU"
#define APPSK  "thereisnospoon"
#endif

#define sensorPower D0
#define sensorPin A0

#define DHTTYPE DHT22

/*
 * Light Terminal D7
 * Heat terminal D8
 * DHT22 terminal D6
 * RainPower terminal D0
 * RainSensor Reading A0
 * Door terminals D1 & D2
 * Windows terminals D3 & D4
 */

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

const long utcOffsetInSeconds = 0;  //Time Offset
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

//Time and Date Assignments
int H,M,S,Y,D;
String Mo,Da;

//Pins for DHT22
//DHT22 connect to Rx(D9)
int DHTPin = D6;
DHT dht(DHTPin, DHTTYPE);

//Light and Heat
//Connect light to D7
int light = D7;
bool lightstatus = LOW;

//Connect heat to D8
int heat = D8;
bool heatstatus = LOW;

//Temperature and Humidity
float t; 
float h;

//Motor control terminals
int in1 = D1;
int in2 = D2;
int in3 = D3;
int in4 = D4;

//Door and Window variables
 bool DoorOpen ;
 bool DoorClose;
 bool WindowOpen;
 bool WindowClose;

//Memories for motor movements
 int i,j,k,l;

//Memory for Automatic and Manual Switch
 int Sw;
 
String ptr;

void setup()
{
 // NodeMCU Connection and OTA Setup
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  delay(500);

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT  0 = 0
  timeClient.setTimeOffset(0);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {Serial.println("Auth Failed");} 
    else if (error == OTA_BEGIN_ERROR) {Serial.println("Begin Failed");} 
    else if (error == OTA_CONNECT_ERROR) {Serial.println("Connect Failed");}
    else if (error == OTA_RECEIVE_ERROR) {Serial.println("Receive Failed");}
    else if (error == OTA_END_ERROR) {Serial.println("End Failed");}
    });
    
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

//RainSensor Initializaiton
  pinMode(sensorPower, OUTPUT);
  // Initially keep the sensor OFF
  digitalWrite(sensorPower, LOW);

  //DHT Initialization
  dht.begin();

  //Define pins
  pinMode(DHTPin, INPUT);
  pinMode(light, OUTPUT);
  pinMode(heat, OUTPUT);

  //Motor pins
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);


  //Function codes
  server.on("/", handle_OnConnect);
  server.on("/lightoff", handle_lightoff);
  server.on("/lighton", handle_lighton);
  server.on("/heatoff", handle_heatoff);
  server.on("/heaton", handle_heaton);
  server.on("/closeC", handle_DoorClose);
  server.on("/closeW", handle_WindowClose);
  server.on("/openC", handle_DoorOpen);
  server.on("/openW", handle_WindowOpen);
  server.on("/Manual", handle_ManualMode);
  server.on("/Automatic", handle_AutoMode);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  ArduinoOTA.handle();

  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  

  int currentHour = timeClient.getHours();
  Serial.print("Hour: ");
  Serial.println(currentHour);
  H = currentHour;  
  
  int currentMinute = timeClient.getMinutes();
  Serial.print("Minutes: ");
  Serial.println(currentMinute); 
  M = currentMinute; 
  
  int currentSecond = timeClient.getSeconds();
  Serial.print("Seconds: ");
  Serial.println(currentSecond);  
  S = currentSecond;

  String weekDay = weekDays[timeClient.getDay()];
  Serial.print("Week Day: ");
  Serial.println(weekDay);
  Da = weekDay;

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);
  D = monthDay;

  int currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);


  String currentMonthName = months[currentMonth-1];
  Serial.print("Month name: ");
  Serial.println(currentMonthName);
  Mo = currentMonthName;

  int currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);
  Y = currentYear;

  //Print complete date:
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  Serial.print("Current date: ");
  Serial.println(currentDate);

  Serial.println("");
  delay(1000);
  
  server.handleClient();

  //get the reading from the function below and print it
  int rainStatus = readSensor();
  Serial.print("Analog Output: ");
  Serial.println(rainStatus);

  // Determine status of rain
  if (rainStatus > 600) {
    Serial.println("Status: Clear");
    WindowOpen  = HIGH;
    WindowClose = LOW;
    WindowOpen  = HIGH;
    WindowClose = LOW;
    
  } else {
    Serial.println("Status: It's raining");
    WindowOpen  = LOW;
    WindowClose = HIGH;
    WindowOpen  = LOW;
    WindowClose = HIGH;
  }

//Temperature Declaration
  t = dht.readTemperature();
  h = dht.readHumidity();

 //Automatically turning on Light
  if (Sw==0 && S>30 && S<=60)
  {
    lightstatus = 1;
  }
  else if(Sw==0 && S<=30)
  {
    lightstatus =0;
  }
  
//Light and heating
 if(lightstatus)
 {
  digitalWrite(light, HIGH);
 }
 else
 {
  digitalWrite(light, LOW);
 }

 if(heatstatus)
 {
  digitalWrite(heat, HIGH);
 }
 else
 {
  digitalWrite(heat, LOW);
 }

 //Door Control
 if (i==0 && DoorClose == 1)
 {
  CloseDoor();
  delay(5000);
  stopDoor();
 }
 else if(j == 0 && DoorOpen == 1)
 {
  OpenDoor();
  delay(5000);
  stopDoor();
 }
 
 //Window Control
  if (k==0 && WindowClose == 1)
 {
  CloseWindow();
  delay(5000);
  stopWindow();
 }
 else if(l == 0 && WindowOpen == 1)
 {
  OpenWindow();
  delay(5000);
  stopWindow();
 }
}

 //Defining Functions
 int readSensor()
 {
  digitalWrite(sensorPower, HIGH);         // Turn the sensor ON
  delay(10);                               // Allow power to settle
  int rainStatus = analogRead(sensorPin); // Read the sensor output
  digitalWrite(sensorPower, LOW);         // Turn the sensor OFF
  return rainStatus;                      // Return the value
 }

  void CloseDoor()
 {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
 }

   void CloseWindow()
 {
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
 }

 void OpenDoor()
 {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
 }

  void OpenWindow()
 {
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
 }

 void stopDoor()
 {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  i = DoorClose;
  j = DoorOpen;
 }

 void stopWindow()
 {
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  k = WindowClose;
  l = WindowOpen;
 }
 
 void handle_OnConnect()
 {
   t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
   lightstatus; heatstatus; DoorOpen ; DoorClose; WindowOpen; WindowClose; 
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
 }

 void handle_heaton()
 {
  t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
   heatstatus = HIGH;
   DoorOpen ; DoorClose; WindowOpen; WindowClose;
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
 }

 void handle_heatoff()
 {
  t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
  heatstatus = LOW;
  DoorOpen; DoorClose; WindowOpen; WindowClose;
  server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
 }

 void handle_lighton()
 {
  t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
  lightstatus = HIGH;
   DoorOpen; DoorClose; WindowOpen; WindowClose;
  server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus,  DoorOpen, DoorClose, WindowClose, WindowOpen));
 }
 
  void handle_lightoff()
  {
    t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
    lightstatus = LOW;
    DoorOpen ; DoorClose; WindowOpen; WindowClose;;
    server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }

  void handle_ManualMode()
  {
   t ;  h ; H ; M ; S; Y; Mo; D; Da;
   Sw = HIGH;
   lightstatus; heatstatus; DoorOpen ; DoorClose; WindowOpen; WindowClose; 
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }
  
  void handle_AutoMode()
  {
   t ;  h ; H ; M ; S; Y; Mo; D; Da;
   Sw = LOW;
   lightstatus; heatstatus; DoorOpen ; DoorClose; WindowOpen; WindowClose; 
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }

  void handle_DoorClose()
  {
   t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
   lightstatus ; heatstatus; WindowOpen; WindowClose;
   DoorOpen  = LOW;
   DoorClose = HIGH;
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }

    void handle_WindowClose()
  {
   t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
   lightstatus ; heatstatus; DoorOpen; DoorClose; 
   WindowOpen  = LOW;
   WindowClose = HIGH;
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }
  
  
  
  void handle_DoorOpen()
  {
  t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
   lightstatus ; heatstatus; WindowOpen; WindowClose;
   DoorOpen  = HIGH;
   DoorClose = LOW;
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }

  void handle_WindowOpen()
  {
   t ;  h ; H ; M ; S; Y; Mo; D; Da; Sw;
   lightstatus ; heatstatus; DoorOpen ; DoorClose;
   WindowOpen  = HIGH;
   WindowClose = LOW;
   server.send(200, "text/html", SendHTML(t, h, H, M, S, Y, Mo, D, Da, Sw, lightstatus, heatstatus, DoorOpen, DoorClose, WindowClose, WindowOpen));
  }
  
 
  void handle_NotFound()
  {
    server.send(404, "text/plain", "Not found");
  }

  // Web design
   String SendHTML(float t, float h,int H, int M,int S,int Y,String Mo,int D, String Da, int Switch, int lightstat, int heatstat, bool Dopen, bool Dclose, bool WindowO, bool WindowC)
 {
  ptr = "<!DOCTYPE html><html>\n";
  ptr += "<head><meta http-equiv=\"refresh\" name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no, 2\">\n";
  ptr +="<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
  ptr +="<title>NodeMCU</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  //ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";

  ptr += "body{ background-image: url(images/IMG-20210814-WA0005.jpg); background-repeat: no-repeat; background-attachment: fixed;background-size: cover;}\n";
  
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";

  
//Temperature and humidity Styling
  ptr +=".side-by-side{display: inline-block;vertical-align: middle;position: relative;}\n";
  ptr +=".humidity-icon{background-color: #3498db;width: 30px;height: 30px;border-radius: 50%;line-height: 36px;}\n";
  ptr +=".humidity-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr +=".Date-Year{font-weight: 150;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr +=".humidity{font-weight: 300;font-size: 60px;color: #3498db;}\n";
  ptr +=".temperature-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
  ptr +=".temperature-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr +=".temperature{font-weight: 300;font-size: 60px;color: #f39c12;}\n";
  ptr +=".superscript{font-size: 17px;font-weight: 600;position: absolute;right: -20px;top: 15px;}\n";
  ptr +=".data{padding: 10px;}\n";
  ptr +="</style>\n";

 ptr +="<script>\n";
 ptr +="setInterval(loadDoc,2000);\n";
 ptr +="function loadDoc() {\n";
 ptr +="var xhttp = new XMLHttpRequest();\n";
 ptr +="xhttp.onreadystatechange = function() {\n";
 ptr +="if (this.readyState == 4 && this.status == 200) {\n";
 ptr +="document.getElementById(\"webpage\").innerHTML =this.responseText}\n";
 ptr +="};\n";
 ptr +="xhttp.open(\"GET\", \"/\", true);\n";
 ptr +="xhttp.send();\n";
 ptr +="}\n";
 ptr +="</script>\n";
 ptr +="</head>\n";
 
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Dr. Ellis Poultry Farm</h1>\n";
  ptr +="<h2>Automated Coop System</h2>\n";

  //Day, Time and Year Display
   ptr +="<div class=\"side-by-side Date-Year\">";
   ptr +=(int)D; ptr +="  ";ptr +=(String)Mo;ptr +=",";ptr +=(int)Y;  
   ptr +="</div>\n";;
   ptr +=(String)Da; ptr +="           ";
   ptr +=(int)H;ptr +=":";ptr +=(int)M;ptr +=":";ptr +=(int)S;

//Switch from Manual to Automatic
   if(Sw)
  {
    ptr += "<p>Control Mode: Manual</p><a class=\"button button-off\" href=\"/Automatic\">Switch Mode</a>\n";
  }
 else
  {
   ptr += "<p>Control Mode: Automatic</p><a class=\"button button-on\" href=\"/Manual\">Switch Mode</a>\n";
  }

   ptr +="<div class=\"data\">\n";
   ptr +="<div class=\"side-by-side temperature-icon\">\n";
   ptr +="<svg version=\"1.1\" id=\"Layer_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n";
   ptr +="width=\"9.915px\" height=\"22px\" viewBox=\"0 0 9.915 22\" enable-background=\"new 0 0 9.915 22\" xml:space=\"preserve\">\n";
   ptr +="<path fill=\"#FFFFFF\" d=\"M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-0.04,6.522,0.421,6.924,1.142\n";
   ptr +="c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424,2.301,2.491\n";
   ptr +="c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-2.331,0.743-3.501,0.463\n";
   ptr +="c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-2.717,2.58-3.394\n";
   ptr +="c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z\"/>\n";
   ptr +="</svg>\n";
   ptr +="</div>\n";
   ptr +="<div class=\"side-by-side temperature-text\">Temperature</div>\n";
   ptr +="<div class=\"side-by-side temperature\">";
   ptr +=(int)t;
   ptr +="<span class=\"superscript\">°C</span></div>\n";
   ptr +="</div>\n";
   ptr +="<div class=\"data\">\n";
   ptr +="<div class=\"side-by-side humidity-icon\">\n";
   ptr +="<svg version=\"1.1\" id=\"Layer_2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n\"; width=\"12px\" height=\"17.955px\" viewBox=\"0 0 13 17.955\" enable-background=\"new 0 0 13 17.955\" xml:space=\"preserve\">\n";
   ptr +="<path fill=\"#FFFFFF\" d=\"M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057\n";
   ptr +="c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C-0.313,11.267,0.026,9.143,1.819,6.217\"></path>\n";
   ptr +="</svg>\n";
   ptr +="</div>\n";
   ptr +="<div class=\"side-by-side humidity-text\">Humidity</div>\n";
   ptr +="<div class=\"side-by-side humidity\">";
   ptr +=(int)h;
   ptr +="<span class=\"superscript\">%</span></div>\n";
   ptr +="</div>\n";

  if(lightstatus)
  {
    ptr += "<p>Light Status: ON</p><a class=\"button button-off\" href=\"/lightoff\">OFF</a>\n";
  }
   else
  {
    ptr +="<p>Light Status: OFF</p><a class=\"button button-on\" href=\"/lighton\">ON</a>\n";
  }

  if(heatstatus)
  {
    ptr += "<p>Heat Status: ON</p><a class=\"button button-off\" href=\"/heatoff\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>Heat Status: OFF</p><a class=\"button button-on\" href=\"/heaton\">ON</a>\n";
  }

  if(DoorClose)
  {
    ptr += "<p>Door Closed</p><a class=\"button button-off\" href=\"/openC\">Open Coop</a>\n";
  }
 else
  {
   ptr += "<p>Door Opened</p><a class=\"button button-on\" href=\"/closeC\">Close Coop</a>\n";
  }

  if(WindowClose)
  {
    ptr += "<p>Windows Closed</p><a class=\"button button-off\" href=\"/openW\">Open Coop</a>\n";
  }
 else
  {
   ptr += "<p>Windows Opened</p><a class=\"button button-on\" href=\"/closeW\">Close Coop</a>\n";
  }
  
  ptr +="</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
 }
