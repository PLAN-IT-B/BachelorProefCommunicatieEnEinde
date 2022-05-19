#include <Arduino.h>                //Arduino library
#include <Wire.h>                   
#include <LiquidCrystal_I2C.h>      //lcd library
#include <TM1637Display.h>          //clock display library
#include "Keypad.h"                 //keypad libraray 
#include "WiFi.h"                   //wifi library
#include "PubSubClient.h"           //mqtt communication library

#define SSID          "NETGEAR68"
#define PWD           "excitedtuba713"

#define MQTT_SERVER   "192.168.1.2"
#define MQTT_PORT     1883

#define LED_PIN       2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char *topic, byte *message, unsigned int length);

bool garbage_Ready = false;
bool opgelost = false;
bool blockKeypad = false;
bool Curr_energie = false;
bool Prev_energie = false;

//code slot
int cinput[4];
int code[] = {-1,-1,-1,-1};

//keypad configuratie
const byte ROWS = 4; 
const byte COLS = 3; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {19, 5, 18, 17}; // <= De IO pinnen van de esp
                    //2, 7, 6, 4  <= de pinnen van de keypad
byte colPins[COLS] = {4, 16, 2}; // <= De IO pinnen van de esp
                    //3, 1, 5 <= de pinnen van de keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);
int c =5; //pointer x-as

//Relaïs 
const int Relais_UV = 32;

//Globale variabelen
long int count_time = 3600000;       // 1000ms in 1sec, 60secs in 1min, 60mins in 1hr. So, 1000x60x60 = 3600000ms = 1hr
unsigned long NewTime = 0;
unsigned long current_time = 0;
unsigned long previous_time = 0;
volatile short int start_pause = 0;
int seconds, minutes;             // For switching between left & right part of the display(Even = left, Odd = right)

//functies definieren
void setup_wifi();
void setup_lcd();
void tip();
void reconnect();
void UV_Enable();
void callback(char *topic, byte *message, unsigned int length);



void setup() {
  pinMode(Relais_UV,OUTPUT); 
  Serial.begin (115200);
  digitalWrite(Relais_UV, LOW);
  Serial.print("begin search");
  Wire.begin();
  byte count = 0;
  for (byte i = 6; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
      delay (1);  // maybe unneeded?
      } // end of good response
  } // end of for loop
  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");

  //Wifi setup
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  //lcd setup
  setup_lcd();
  delay(2000);
  lcd.clear();
  lcd.noBacklight();
}

void loop() {
  //Connectie checken en tesnoods reconnecten
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

  //Bij het veranderen van het energie niveau
  if(Prev_energie != Curr_energie){
    if(Curr_energie == true){
      setup_lcd();
      Prev_energie = Curr_energie;
      //Serial.println("keypad aan");
    }else if(Curr_energie == false){
      lcd.clear();
      lcd.noBacklight();
      Prev_energie = Curr_energie;
      //Serial.println("keypad uit");
    }
  }
  
  //Als de knop wordt ingedrukt, het keypad niet geblokkeerd is en er genoeg energie is.
  char key = customKeypad.getKey();
  if(key && blockKeypad == false && Curr_energie){
    /*
     Serial.print("De waarde uit het keypad is: ");
     Serial.println(key);
    */
    //Als # (enter wordt ingedrukt)
    if(key =='#'){    
      if(c ==9 ){ //De positie is het laatste cijfer
        opgelost = true; 
        //Controleer of de code klopt
        for(int i = 0;i<4;i++){
          if(code[i]!=cinput[i]){
            opgelost = false;
            lcd.setCursor(5,1);
            lcd.print("____kg");
            c = 5;
          }
        }
        //Als de garbage puzzel nogniet klaar is, gaan we zeggen dat ze moeten focussen op een andere puzzel.
        if (garbage_Ready==false) {
          tip();
        }
        //Als het opgelost is schakel we het UV-licht en blokeren we het keypad voor bugs.
        if(opgelost){
          UV_Enable();   
          blockKeypad = true;
        }            
      }
    }     
    else if(key == '*'){ //Als * (terug) wordt ingevuld
      //Ga 1 terug, vervang het getal door _ en vervang de code door 0 (standaard getal in rij)
      if(c>5){
        c--;
        lcd.setCursor(c,1);
        lcd.print("_");
        cinput[c-5] = 0;
      }
    }
    else{ //Als er iets anders (cijfer) wordt ingedrukt
      if(c<9){ //Vul het getal in en schuif 1 plaats op.
        lcd.setCursor(c,1);
        lcd.print(key);
        cinput[c-5]= key-'0';
        lcd.setCursor(c,1);
        c++;
        lcd.display();
      }    
    }
  }
}

void callback(char *topic, byte *message, unsigned int length)
{
  //Print overzichtelijk uit wat het bericht is en op welk topic dit is toegekomen.
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp ="";

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println(" ");

  //Als het een bericht is van de garbadge puzzel, zal het zijn voor de code in te stellen/veranderen.
  if (strcmp(topic,"garbage/eindcode") == 0) 
  {
    for (int i = 0; i < 4; i++)
    {
      code[i] = message[i]-'0';
    }
    /*Serial.print("De code is veranderd naar ");
      for (int i = 0; i < 4; i++)
      {
        Serial.print(code[i]);
      }
    Serial.println(); 
    */
    garbage_Ready = true;
  }

  //De status van de buffer updates bij verandering
  if (strcmp(topic,"TrappenMaar/zone") == 0) {
    if(messageTemp == "groen"){
      Curr_energie = true;
    }
    else if(messageTemp == "oranje"){
      Curr_energie = true;
    }
    else if(messageTemp == "rood"){
      Curr_energie = false;
    }
  }

  //Als het een bericht is om de escape room klaar te maken voor gebruik, gaan we resetten.
   if (strcmp(topic,"controlpanel/reset") == 0) 
  {
    ESP.restart();
  }
}

void setup_wifi()
{
  //Default code gekreven van onderzoeksgroep dramco.
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

void setup_lcd(){
  //default lcd keypad display
  lcd.init();
  lcd.clear();         
  lcd.backlight(); 
  lcd.setCursor(0,0);
  lcd.print("Voer de code in:");
  lcd.setCursor(5,1);
  //Dit wordt in kg uitgedrukt zodat de speler de link gaat leggen tussen het totaal gewicht van de garbage puzzel en dit slot.
  lcd.print("____kg");
}

void tip(){

  //tip printen
  lcd.init();
  lcd.clear();         
  lcd.backlight(); 
  lcd.setCursor(0,0);
  lcd.print("Tip: Doe een");
  lcd.setCursor(3,1);
  lcd.print("andere puzzel");

  //tijd om te lezen
  delay(5000);
  
  //oorspronkelijke scherm terugzetten
  setup_lcd();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("UV-Slot_ESP"))
    {
      Serial.println("connected");
      //Tijdens het opstarten een ready signaal sturen naar de eindpuzzel esp.
      client.publish("controlpanel/status", "UV-slot Ready");
      //Subscribions
      client.subscribe("controlpanel/reset");
      client.subscribe("garbage/eindcode");
      client.subscribe("TrappenMaar/zone");
    }
    else{
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

void UV_Enable(){
  //schakelt de UV lamp via een relaïs
  digitalWrite(Relais_UV, HIGH);
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Correct!");
  lcd.setCursor(3,1);
  lcd.print("Fiets maar");

  //zorgen dat deze functie maar één keer wordt uitgevoerd
  opgelost = false;
  for (int i = 0; i < 4; i++)
  {
    cinput[i]=0;
  }
}
