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
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

const char *ssid = "MiFibra-1980-plus";
const char *password = "45Rtfgcv@_739";

#define BOTtoken "6239834702:AAFiD5nxEEqcG3ZBAaC2474E8AIoPkBCQCg"
#define CHAT_ID "1349728576"

const String authorizedUsers[] = {"1349728576", "987654321"};

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 250;
unsigned long lastTimeBotRan;

unsigned long tiempoAnterior = 0;
unsigned long intervalo = 300000;

unsigned long reinicioProgramado;
unsigned long intervaloReinicio = 12 * 60 * 60 * 1000;

int valortemp = 0;

int lastProcessedMessageId = 0;

#include "DHT.h"

#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA 64

#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

#define DHTPIN 33
#define DHTTYPE DHT22

const uint16_t kIrLed = 4;
IRGreeAC ac(kIrLed);

DHT dht(DHTPIN, DHTTYPE);

float hum;
float temp;


void printState()
{
  Serial.println("GREE A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());

  unsigned char *ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kGreeStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
#endif

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());
}

void bot_setup()
{
  const String commands = F("["
                            "{\"comando\":\"start\",  \"description\":\"Get bot usage help\"},"
                            "{\"comando\":\"start\", \"description\":\"Message sent when you open a chat with a bot\"},"
                            "{\"comando\":\"status\",\"description\":\"Answer device current status\"}"
                            "]");
  bot.setMyCommands(commands);
}

void setup()
{
#ifdef __DEBUG__
  ac.begin();
  Serial.begin(115200);
  dht.begin();
  hum = 0;
  delay(200);
  Serial.println("Iniciando pantalla OLED");
  printState();
  Serial.println("Setting desired state for A/C.");
  ac.on();
  ac.setFan(1);
  ac.setMode(kGreeCool);
  ac.setTemp(22);
  ac.setSwingVertical(false, kGreeSwingAuto);
  ac.setXFan(false);
  ac.setLight(true);
  ac.setSleep(false);
  ac.setTurbo(false);
  ac.send();
  Serial.println("Ac encendido en setup");
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();
  delay(2000);
  display.clearDisplay();
#else
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();
  delay(2000);
  display.clearDisplay();
#endif

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.println("WiFi connected");
  display.print("IP address:");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
  display.clearDisplay();
  display.println("Conectado");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.println("Enviando mensaje");
  display.display();
  delay(2000);
  display.clearDisplay();

  bot_setup();

  Serial.println("Bot ready");
  display.println("Bot ready");
  display.display();
  delay(2000);
  display.clearDisplay();

  lastTimeBotRan = 0;

  
}



void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String command = text.substring(0, text.indexOf(' '));

    // Verificar si el chat_id está en la lista de usuarios autorizados
    bool isAuthorized = false;
    for (const auto& user : authorizedUsers) {
      if (chat_id.equals(user)) {
        isAuthorized = true;
        break;
      }
    }

    if (!isAuthorized) {
      // Si el chat_id no está autorizado, ignorar el mensaje
      continue;
    }

    if (command == "/start")
    {
      // Crear los botones del teclado de respuesta
      String keyboard = "[[\"/status\", \"/ac_on\"], [\"/ac_off\"]]";

      // Enviar mensaje con los botones del teclado de respuesta
      bot.sendMessageWithReplyKeyboard(chat_id, "Hola " + bot.messages[i].from_name + "!\n"
                        "Soy un bot de control de aire acondicionado. "
                        "Elige una opción:", "Markdown", keyboard);
    }
    else if (command == "/status")
    {
      float h = dht.readHumidity();
      float t = dht.readTemperature();

      if (isnan(h) || isnan(t))
      {
        String errorMessage = "Error al leer los datos de humedad y temperatura. Asegúrate de que el sensor esté conectado correctamente.";
        bot.sendMessage(chat_id, errorMessage, "Markdown");
      }
      else
      {
        String statusMessage = "Estado actual del dispositivo:\n"
                               "Temperatura: " + String(t) + " °C\n" +
                               "Humedad: " + String(h) + " %";
        bot.sendMessage(chat_id, statusMessage, "Markdown");
      }
    }
    else if (command == "/ac_on")
    {      
      String statusMessage = "Dispositivo encendido:";
      ac.on();
      ac.setFan(1);
      ac.setMode(kGreeCool);
      ac.setTemp(22);
      ac.setSwingVertical(false, kGreeSwingAuto);
      ac.setXFan(false);
      ac.setLight(true);
      ac.setSleep(false);
      ac.setTurbo(false);
      ac.send();
      bot.sendMessage(chat_id, statusMessage, "Markdown");  
    }
    else if (command == "/ac_off")
    {      
      ac.on();
      ac.off();
      ac.send();
      String statusMessage = "Dispositivo apagado:";
      bot.sendMessage(chat_id, statusMessage, "Markdown");  
    }
    else
    {
      String errorMessage = "Comando no reconocido. Envía '/start' para obtener ayuda.";
      bot.sendMessage(chat_id, errorMessage, "Markdown");
    }
    lastProcessedMessageId = bot.messages[i].message_id;
  }
}



void loop()
{
  if (millis() > lastTimeBotRan + botRequestDelay)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      int latestMessageId = bot.messages[numNewMessages - 1].message_id;

      if (latestMessageId > lastProcessedMessageId)
      {
        handleNewMessages(numNewMessages);
        lastProcessedMessageId = latestMessageId;
      }

      numNewMessages--;
    }

    lastTimeBotRan = millis();
  }

  unsigned long tiempoActual = millis();
    hum = dht.readHumidity();
    temp = dht.readTemperature();

      display.clearDisplay();
  
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Temp:");
  display.setCursor(0, 20);
  display.println(temp);
  display.setCursor(65, 20);
  display.println("C");

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("Humedad:");
  display.setCursor(50, 40);
  display.println(hum);
   display.setCursor(80, 40);
  display.println("%");

  display.display();
  delay(2000);
}
