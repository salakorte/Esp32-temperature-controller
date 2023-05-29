#define __DEBUG__
 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <IRsend.h>
#include <ir_Gree.h>




#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

const char* ssid = "MiFibra-1980-plus";
const char* password = "45Rtfgcv@_739";

#define BOTtoken "6239834702:AAFiD5nxEEqcG3ZBAaC2474E8AIoPkBCQCg"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "1349728576"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

unsigned long tiempoAnterior = 0;
unsigned long intervalo = 300000;

int valortemp = 0;

#include "DHT.h"
 
// Definir constantes
#define ANCHO_PANTALLA 128 // ancho pantalla OLED
#define ALTO_PANTALLA 64 // alto pantalla OLED
 
// Objeto de la clase Adafruit_SSD1306
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

#define DHTPIN 33
#define DHTTYPE DHT22

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRGreeAC ac(kIrLed);  // Set the GPIO to be used for sending messages.

DHT dht(DHTPIN, DHTTYPE);

 float hum;
 float temp;

 void printState() {
  // Display the settings.
  Serial.println("GREE A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kGreeStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();

 WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void bot_setup()
{
  const String commands = F("["
                            "{\"comando\":\"start\",  \"description\":\"Get bot usage help\"},"
                            "{\"comando\":\"start\", \"description\":\"Message sent when you open a chat with a bot\"},"
                            "{\"comando\":\"status\",\"description\":\"Answer device current status\"}" // no comma on last command
                            "]");
  bot.setMyCommands(commands);
  //bot.sendMessage("25235518", "Hola amigo!", "Markdown");
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP
void setup() {
#ifdef __DEBUG__
  ac.begin();
  Serial.begin(115200);
  dht.begin();
  hum = 0;
  delay(200);
  Serial.println("Iniciando pantalla OLED");
    // Set up what we want to send. See ir_Gree.cpp for all the options.
  // Most things default to off.
  Serial.println("Default state of the remote.");
  printState();
  Serial.println("Setting desired state for A/C.");
  ac.on();
  ac.setFan(1);
  // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
  ac.setMode(kGreeCool);
  ac.setTemp(22);  // 16-30C
  ac.setSwingVertical(false, kGreeSwingAuto);
  ac.setXFan(false);
  ac.setLight(true);
  ac.setSleep(false);
  ac.setTurbo(false);
  ac.send();
  Serial.println("Ac encendido en setup");
#endif
 
  // Iniciar pantalla OLED en la dirección 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
#ifdef __DEBUG__
    Serial.println("No se encuentra la pantalla OLED");
#endif
    while (true);
  }
   String keyboardJson2 = "[[{ \"text\" : \"Encender\", \"callback_data\" : \"/ac_on\" }],[{ \"text\" : \"Apagar\", \"callback_data\" : \"/ac_off\" }],[{ \"text\" : \"Estado\", \"callback_data\" : \"/estado\" }]]";
        bot.sendMessageWithInlineKeyboard(CHAT_ID, "Elegir una de las siguientes opciones", "", keyboardJson2);
}
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {

  unsigned long tiempoActual = millis();
  hum = dht.readHumidity();
  temp = dht.readTemperature();
    // Limpiar buffer
  display.clearDisplay();
 
  // Tamaño del texto
  display.setTextSize(2);
  // Color del texto
  display.setTextColor(SSD1306_WHITE);
  // Posición del texto
  
  // Escribir texto
  display.setCursor(0, 0);
  display.println("Temp");
  display.setCursor(10,17);
  display.println(temp);
  display.println("Humedad");
  display.setCursor(10,49);
  display.println(hum);
 
  // Enviar a pantalla
  display.display();

    // Now send the IR signal.
/*#if SEND_GREE
  Serial.println("Sending IR command to A/C ...");
  ac.send();
#endif  // SEND_GREE/*/
  //printState();
  delay(5000);

  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
if (tiempoActual - tiempoAnterior >= intervalo) {
  if (temp > 23) {
  ac.on();
  ac.setFan(1);
  // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
  ac.setMode(kGreeCool);
  ac.setTemp(22);  // 16-30C
  ac.setSwingVertical(false, kGreeSwingAuto);
  ac.setXFan(true);
  ac.setLight(true);
  ac.setSleep(false);
  ac.setTurbo(false);
    Serial.println(F("AC Switched On"));
    ac.send();
  
  }
  else if (temp < 21) {
  ac.off();
    Serial.println(F("AC Switched Off"));
    ac.send();
    
  }

   tiempoAnterior = tiempoActual;
  }

  
}

// Handle what happens when you receive new messages
/*void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

 / for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
  //convertir de float a string
    String strTemp = String(temp);
    String strHum = String(hum);
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/menu") {
      /*String welcome = "Bienvenido, " + from_name + ".\n";
      welcome += "Usa los siguientes comandos para navegar.\n\n";
      welcome += "/ac_on Para encender el aire \n";
      welcome += "/ac_off Para apagar el aire \n";
      welcome += "/estado para ver la temperatura \n";
      welcome += "/temp x [Donde la x pones la temperatura\n";
      welcome += "deseada]Para poner la temperatura\n";
      bot.sendMessage(chat_id, welcome, "");*/
     // String keyboardJson = "[[{ \"text\" : \"Encender\", \"callback_data\" : \"/ac_on\" }],[{ \"text\" : \"Apagar\", \"callback_data\" : \"/ac_off\" }],[{ \"text\" : \"Estado\", \"callback_data\" : \"/estado\" }]]";
      //  bot.sendMessageWithInlineKeyboard(CHAT_ID, "Elegir una de las siguientes opciones", "", keyboardJson);
      //  if (text == "/options")
    {
      String keyboardJson = "[[\"/ac_on\", \"/ac_off\"],[\"/estado\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson, true);
    }
    }

 if (text == "/start")
    {
     String welcome = "Bienvenido, " + from_name + ".\n";
      welcome += "Usa los siguientes comandos para navegar.\n\n";
      welcome += "/ac_on Para encender el aire \n";
      welcome += "/ac_off Para apagar el aire \n";
      welcome += "/estado para ver la temperatura \n";
      welcome += "/temp x [Donde la x pones la temperatura\n";
      welcome += "deseada]Para poner la temperatura\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    
    }
    if (text == "/ac_on") {
     ac.on();
     ac.send();
    }
    
    if (text == "/ac_off") {
     ac.off();
     ac.send();
    }

    if (text.startsWith("/temp")) {
        String valorString = text.substring(7);
        valortemp = valorString.toInt();
        ac.on();
        ac.setTemp(valortemp);
        bot.sendMessage(chat_id, "Establecido: " + String(valortemp)+ "º");
}

    
    if (text == "/estado") {
      String State = "Estado de los sensores.\n";
      State += "Temperatura de la sala:\n";
      State += strTemp + "º.\n";
      State += "Humedad en la sala:\n";
      State += strHum + "g/m³.\n";
      bot.sendMessage(chat_id, State, "");

    }
   

    }
   
  }
  
