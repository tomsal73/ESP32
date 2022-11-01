/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/telegram-request-esp32-esp8266-nodemcu-sensor-readings/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  Project created using Brian Lough's Universal Telegram Bot Library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
*/
#define ESP8266
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Adafruit_Sensor.h>
//#include <DHT.h>


// Replace with your network credentials
//const char* ssid = "Peteliskiu24";
//const char* password = "Salkai8875";
const char* ssid = "HUAWEI-B311-EB71";
const char* password = "7G899TADM50";

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "1174904160"

// Initialize Telegram BOT
#define BOTtoken "2026965506:AAG0btvUpMvEKqqVPyeIV1ZxkvKaNLeNugs"  // your Bot Token (Get from Botfather)

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

//Define cinbfig file name
#define CONFIGFILE "/config.json"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

#define sensorPin A0
#define ONE_WIRE_BUS 2
#define relayPin 5
#define LED D4
//#define DHTPIN 14     // Digital pin connected to the DHT sensor
//#define DHTTYPE    DHT11     // DHT 22 (AM2302)

//Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
int botStatus = 0 ;
int botStatus2 = 0 ;
int botForcedStatus = 0 ;

int botMinTemp = 3; // temperature lower range
int botMaxTemp = 25; // temperature higher range
unsigned long botMessageInt = 10000 ; //10 sec
float botTemperature = botMinTemp+1 ;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

//DHT dht(DHTPIN, DHTTYPE);

void SaveParameters (int _min, int _max) {
  char min_temp[3] ;
  char max_temp[3] ;

  sprintf (min_temp, "%d", _min) ;
  sprintf (max_temp, "%d", _max) ;
  //save the custom parameters to FS
  Serial.println("saving config");

    // ArduinoJson 5
    //DynamicJsonBuffer jsonBuffer;
    //JsonObject& json = jsonBuffer.createObject();
    //json["min_temp"] = min_temp;
    //json["max_temp"] = max_temp;
    //json.printTo(Serial);
    //json.printTo(configFile);

    // ArduinoJson 6
    DynamicJsonDocument doc(1024);
    doc["min_temp"] = min_temp;
    doc["max_temp"] = max_temp;
    serializeJson(doc, Serial);          
    Serial.println("");
  
   if (SPIFFS.begin()) {          
      File configFile = SPIFFS.open(CONFIGFILE, "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      } else {
        serializeJson(doc, configFile);
        configFile.close();
      }
   } else {
      Serial.println("failed to mount FS");
   }
   //end save
}

void ReadParameters (int* _pmin, int* _pmax) {
  char min_temp[3] ;
  char max_temp[3] ;

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(CONFIGFILE)) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(CONFIGFILE, "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        
        // ArduinoJson 5
        //DynamicJsonBuffer jsonBuffer;
        //JsonObject& json = jsonBuffer.parseObject(buf.get());
        //json.printTo(Serial);
        //if (json.success()) {
        //  strcpy(min_temp, json["min_temp"]);
        //  strcpy(max_temp, json["max_temp"]);
        //  _min = String(min_temp).toInt() ;
        //  _max = String(max_temp).toInt() ;
        //} else {
        //  Serial.println("failed to load json config");
        // }

        // ArduinoJson 6
        DynamicJsonDocument doc(size);
        auto error = deserializeJson(doc, buf.get());
        if (error) {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(error.c_str());
          return;
        }
        Serial.println("\nparsed json");
        strcpy(min_temp, doc["min_temp"]);
        strcpy(max_temp, doc["max_temp"]);
        *_pmin = String(min_temp).toInt() ;
        *_pmax = String(max_temp).toInt() ;        
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}
// Get Temp sensor readings and return them as a String variable
String getReadings(){
  // Get a reading from the temperature sensor:
  //int reading = analogRead(sensorPin);
  // For ESP8266 At 3.3 volts A0 pin gives 1023 and at 1.5 volts its (1.5 / 3.3)*1023 = 465
  // 465 / 150 = 3.1. Now if nodemcu analog pin counts 3.1 its equal to 1 degree change in centigrade/Celsius temperature of LM35..
  // Convert the reading into voltage:
  //float voltage = (reading /  1024.0) * 3300;
  // Convert the voltage into the temperature in degree Celsius:
  //float temperature = voltage / 10;
  
  //Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  Serial.print("Temperature is: "); 
  Serial.println(sensors.getTempCByIndex(0));
  botTemperature = sensors.getTempCByIndex(0);

/*
  // current temperature & humidity
  float temperature = 0.0;
  float hum = 0.0;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
  if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      temperature = newT;
      Serial.println(temperature);
    }
  // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      hum = newH;
      Serial.println(hum);
    }
*/
    
  String message = "Temp.: " + String(botTemperature) + " ºC";
  message+=" ["+ String(botMinTemp) + " ºC -";  
  message+=" "+ String(botMaxTemp) + " ºC]\n"; 
  //message+="   Drėgmė.: "+ String(hum) + " % \n"; 
  return message;
}

String getRelayStatus(){
  String message = "";
  if (botStatus == 1) {
    message+="ĮJUNGTA" ;
  }
  else
  {
    message+="IŠJUNGTA" ;
  }
  return message;
}

//Turn on relay
void setRelayOn () {
  digitalWrite(relayPin, HIGH);
  botStatus = 1 ;
}

//Turn off relay
void setRelayOff () {
  digitalWrite(relayPin, LOW);
  botStatus = 0 ;
}

//Turn on or off based on temperature
void  processTemp () {
  if (!botForcedStatus) {
    if ((botTemperature < botMinTemp) && !botStatus){
        setRelayOn () ;
    } else if ((botTemperature >= botMaxTemp) && botStatus) {
        setRelayOff () ;
    }
  }
}

//Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      Serial.println(welcome);
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/status") {
      String statusMsg = getReadings();
      statusMsg +="Būsena: ";
      statusMsg += getRelayStatus() ;
      Serial.println(statusMsg);
      bot.sendMessage(chat_id, statusMsg, "");
    }  

    if (text == "/on") {
      botForcedStatus = 1 ;
      setRelayOn () ;
      String stat = getRelayStatus();
      Serial.println(stat);
      bot.sendMessage(chat_id, stat, "");
    } 

    if (text == "/off") {
      botForcedStatus = 1 ;
      setRelayOff () ;
      String stat = getRelayStatus();
      Serial.println(stat);
      bot.sendMessage(chat_id, stat, "");
    } 
    
    if (text == "/reset") {
      botForcedStatus = 0 ;
      processTemp () ;
      String stat = getRelayStatus();
      Serial.println(stat);
      bot.sendMessage(chat_id, stat, "");
    } 
    
    if (text.startsWith("/setmin")) {
      Serial.println(text.substring(8));
      botMinTemp = text.substring(8).toInt() ;
      SaveParameters (botMinTemp, botMaxTemp) ;
      String stat = "Minimali temperatūra nustatyta:" + String(botMinTemp);
      Serial.println(stat);
      bot.sendMessage(chat_id, stat, "");
    } 

    if (text.startsWith("/setmax")) {
      Serial.println(text.substring(8));
      botMaxTemp = text.substring(8).toInt() ;
      SaveParameters (botMinTemp, botMaxTemp) ;
      String stat = "Maksimali temperatūra nustatyta:" + String(botMaxTemp);
      Serial.println(stat);
      bot.sendMessage(chat_id, stat, "");
    }     
  }
}


void bot_setup()
{
  const String commands = F("["
                            "{\"command\":\"on\",  \"description\":\"Įjungti\"},"
                            "{\"command\":\"off\", \"description\":\"Išjungti\"},"
                            "{\"command\":\"reset\", \"description\":\"Grįžti prie automatinio valdymo\"},"
                            "{\"command\":\"setmin\", \"description\":\"Nustatyti minimalią temperatūrą\"},"
                            "{\"command\":\"setmax\", \"description\":\"Nustatyti maksimalią temperatūrą\"},"
                            "{\"command\":\"status\",\"description\":\"Parodyti būseną\"}" // no comma on last command
                            "]");
  bot.setMyCommands(commands);
  setRelayOff () ;
  //Read parameters from config file
  ReadParameters (&botMinTemp, &botMaxTemp) ;
  
  
  delay(500);
  bot.sendMessage(CHAT_ID, "Pasiruošęs!", "");
  Serial.println("Pasiruošęs!");
}


void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  //pinMode(LED, OUTPUT); //LED pin as output
  
  // Start up the library
   sensors.begin();
  //dht.begin();
  
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif
  
  // Init BME280 sensor
 //if (!bme.begin(0x76)) {
 //   Serial.println("Could not find a valid BME280 sensor, check wiring!");
 //   while (1);
 // }
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    // turn LED On
    //digitalWrite(LED, LOW);
    delay(1000);
    // turn LED Off
    //digitalWrite(LED, HIGH);
    
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  bot_setup();
}

unsigned long lastmessage = millis() ;
void loop() {
  
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  if (millis() >  lastmessage + botMessageInt)  {
      String statusMsg = getReadings();
      int currentStatus = botStatus ;
      int currentStatus2 = botStatus2 ;
      processTemp () ;
      statusMsg += getRelayStatus() ;
      if (currentStatus != botStatus) {
        bot.sendMessage(CHAT_ID, statusMsg, "");
        Serial.println(statusMsg);
      }// else  if (currentStatus2 != botStatus2) {
       // bot.sendMessage(CHAT_ID, statusMsg, "");
       // Serial.println(statusMsg);
      //}
     lastmessage = millis() ;
  }
}
