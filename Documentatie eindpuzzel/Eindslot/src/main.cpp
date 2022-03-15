#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"
#include "Keypad.h"
#include "HX711.h"

//MQTT
#define SSID          "NETGEAR68"
#define PWD           "excitedtuba713"
#define MQTT_SERVER   "192.168.1.2"
#define MQTT_PORT     1883
#define LED_PIN       2

WiFiClient espClient;
PubSubClient client(espClient);

//Staten
boolean reset;
boolean opgelost;


//code 
int cinput[4];
int code[] = {6,4,2,9};


long lastMsg = 0;
char msg[50];
int value = 0;


void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
   
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  if (messageTemp == "Reset escaperoom"){
    ESP.restart();
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Eindslot"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("controlpanel/reset");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



//Keypad
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9,7,8}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char key;


//Varia
#define Button_pin1 1
#define Button_pin2 2
#define Button_pin3 3
char* straf;


//lcd
LiquidCrystal_I2C lcd(0x27,20,4);
int c;
boolean bl;
boolean codeTekst;



void setup() {
  // lcd init
  lcd.init();
  lcd.backlight();
  codeTekst = false;
  c= 8;

   //MQTT
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  //Overige

  //Ready
  client.publish("controlpanel/status","Eindpuzzel Ready");
  Serial.println("Ready gestuurd");
}
  
void loop() {

  key = keypad.getKey(); //Vraag de input van de key op

  //MQTT
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }
  
    if(opgelost){
 }
   
}


