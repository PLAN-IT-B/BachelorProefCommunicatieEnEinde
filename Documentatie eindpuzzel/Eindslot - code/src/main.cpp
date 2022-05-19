#include <Arduino.h>            //Arduino library
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  //lcd library
#include <TM1637Display.h>      //clock display library
#include "Keypad.h"             //keypad libraray
#include "WiFi.h"               //wifi library
#include "PubSubClient.h"       //mqtt communication library

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

//functies definiëren
void callback(char *topic, byte *message, unsigned int length);
void IRAM_ATTR ISR_restart();
void IRAM_ATTR ISR_start();
void IRAM_ATTR onTimer();
void setup_wifi();
void reconnect();
void setup_lcd();
void open_Vault();
void puzzelReady(String message);
void openSlot();
void tip();

//glabale booleans definiëren
bool codeJuist = false;
bool AllReady = false;
bool ReadyArray[5];
int Wristbands = 0;
bool reset= false;
bool keypadBlocked = false;
bool energie =false;
bool notificatie_START = false;
bool openVault = false;

//Start reset knoppen
const int Spin = 14;
const int Rpin = 13;
const int Sled = 27;
const int Rled = 23;
int startbutton_status = 0;
int resetbutton_status = 0;

//code slot
int cinput[4];
int code[] = {2,6,0,3}; //de datum van overshoot day

//keypad configuratie
const byte ROWS = 4; 
const byte COLS = 3; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {18, 5, 17, 16}; // <= De IO pinnen van de esp
                    //2, 7, 6, 4  <= de pinnen van de keypad
byte colPins[COLS] = {4, 15, 2}; // <= De IO pinnen van de esp
                    //3, 1, 5 <= de pinnen van de keypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);
int c =8; //pointer x-as

//Relais 
const int Relais_Sol = 32;
const int SoundSys = 19;

// Clock display pins (digital pins)
#define CLK 26
#define DIO 25
TM1637Display display = TM1637Display(CLK, DIO);

//timer interrupt
int t1 = 0;
int t2 = 0;
int adc_read_counter = 0;
hw_timer_t * timer = NULL; 

//timer variabelen
long int count_time = 3600000;      // 1000ms in 1sec, 60secs in 1min, 60mins in 1hr. So, 1000x60x60 = 3600000ms = 1hr
unsigned long NewTime = 0;
unsigned long current_time = 0;
unsigned long previous_time = 0;
volatile short int start_pause = 0;
int seconds, minutes;             // For switching between left & right part of the display(Even = left, Odd = right)

void setup() {
  pinMode(Relais_Sol,OUTPUT);
  pinMode(SoundSys,OUTPUT);
  pinMode(Sled,OUTPUT); 
  pinMode(Rled,OUTPUT);
  digitalWrite(Rled, HIGH);
  pinMode(Spin, INPUT);
  pinMode(Rpin, INPUT);
  Serial.begin (115200);
  digitalWrite(Relais_Sol, LOW);
  Serial.print("begin search");
  Wire.begin();
  byte count = 0;
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0){
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

  //timer interrupt setup
  timer = timerBegin(0, 80, true);               //Begin timer with 1 MHz frequency (80MHz/80)
  timerAttachInterrupt(timer, &onTimer, true);   //Attach the interrupt to Timer1
  timerAlarmWrite(timer, 1000000, true);         //Initialize the timer. The 1000000 in this line means the alarm should go off every 1000000 cycles of the clock. 1000000/1000000 = 1s
  
  //hardware interrupt setup
  attachInterrupt(Rpin, ISR_restart, FALLING);
  attachInterrupt(Spin, ISR_start, FALLING);

  //default timer display setup
  display.setBrightness(7);
  display.showNumberDecEx(6000,0b01000000);
  count_time = 3600000;

  //default lcd display setup
  lcd.init();
  lcd.clear();         
  lcd.backlight(); 
  lcd.setCursor(2,0);
  lcd.print("Voer de code in:");
  lcd.setCursor(8,2);
  lcd.print("____");
}

void loop(){
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

  //Op een gegeven moment is dit true, en zullen de acties correct worden uitgevoerd.
  if (openVault){
    open_Vault();
    openVault = false;
  }  

  //De reset interrupt zet dit op true zodat in de volgende loop, dit wordt uitgevoerd.
  if(reset){
    client.publish("controlpanel/reset","Reset escaperoom");
    //Serial.println("Het reset signaal is gestuurd naar alle puzzeles.");
    for (int i = 0; i < 30; i++){
      delay(100);
      digitalWrite(Rled, LOW);
      delay(100);
      digitalWrite(Rled, HIGH);
    }
    reset = false;
  }

  //Lezen wat de status is van de knoppen
  startbutton_status = digitalRead(Spin);
  resetbutton_status = digitalRead(Rpin);
  char key = customKeypad.getKey();
  
  if(codeJuist){ //Als de code klopt 
    openSlot();    
  }
  //Als de knop wordt ingedrukt en de keypad niet geblokt moet worden
  else if(key && !keypadBlocked ){
      /*
      Serial.print("De waarde uit het keypad is: ");
      Serial.println(key);
      */
      //Als # (enter wordt ingedrukt) en er is energie
      if(key =='#'){    
        if(c ==12){ //De positie is het laatste cijfer
          codeJuist = true; //Controleer of de code klopt
          /*for (int i = 0; i < 4; i++)
          {
            Serial.print(cinput[i]);
          }*/

          //Als de ingegeven code fout is, of als de escape room nogniet klaar is, zal het fout zijn en word je lcd voor een nieuwe poging ingesteld.
          for(int i = 0;i<4;i++){
           if(code[i]!=cinput[i] || !AllReady){
             codeJuist = false;
             lcd.setCursor(8,2);
             lcd.print("____");
             c = 8;
           }
          }
          //Als de code fout word ingegeven zal het zeggen dat je fout bent, en zal de buffer HEEL VEEL zakken!
          if(!codeJuist && energie){
            lcd.clear();
            lcd.setCursor(7,1);
            lcd.print("Fout!");
            lcd.setCursor(2,2);
            lcd.print("De buffer zakt!");
            delay(2000);
            for (int i = 0; i < 6; i++){client.publish("TrappenMaar/buffer","grote fout");}
            delay(4000);
            setup_lcd();
          }
          //Als de buffer niet genoeg energie heeft, zal het cijferslot zeggen dat je meer energie nodig hebt.
          if (!energie){
            tip();
            codeJuist = false;
            lcd.setCursor(8,2);
            lcd.print("____");
            c = 8;
          }
        }
      }
      else if(key == '*'){ //Als * (terug) wordt ingevuld
          //Ga 1 terug, vervang het getal door _ en vervang de code door 0 (standaard getal in rij)
        if(c>8){
          c--;
          lcd.setCursor(c,2);
          lcd.print("_");
          cinput[c-8] = 0;
        }
          
      }
      else{ //Als er iets anders (dus cijfer) wordt ingedrukt.
        if(c<12){ //Vul het getal in en schuif 1 plaats op.
        lcd.setCursor(c,2);
        lcd.print(key);
        cinput[c-8]= key-'0';
        lcd.setCursor(c,2);
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
  String messageTemp = "";

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  //Als het toekomend bericht een statusupdate is van een puzzel, zullen we gaan registreren welke puzzel dit is en onthouden.
  if (strcmp(topic,"controlpanel/status") == 0) 
  {
   puzzelReady(messageTemp);
  }

 //De status van de buffer beinvloed of je de code kan ingeven of niet.
  if (strcmp(topic,"TrappenMaar/zone") == 0) 
  {
    Serial.println("zone bericht");
    if(messageTemp == "vol"){
      energie = true;
    }
    else if(messageTemp == "niet vol"){
      energie = false;
    }
  }
}  


//De interrupt service handler voor de timer
void IRAM_ATTR onTimer() {
  NewTime = count_time - 1;                       // Calculate the time remaining 
  seconds = (NewTime % 60);                       // To display the countdown in mm:ss format
  minutes = ((NewTime / 60) % 60 );               // separate CountTime in two parts
  
  display.showNumberDecEx(seconds, 0, true, 2, 2); // Display the seconds in the last two places
  display.showNumberDecEx(minutes, 0b11100000, true, 2, 0); // Display the minutes in the first two places, with colon

  count_time = NewTime;                  // Update the time remaining
}

//De interrupt service handler voor de restartknop
void IRAM_ATTR ISR_restart(){
  Wristbands = 0;
  for (int i = 0; i < sizeof(ReadyArray); i++)
  {
    ReadyArray[i] = false;
  }
  reset= true;  
  digitalWrite(Sled, LOW);
}

//De interrupt service handler voor de startknop
void IRAM_ATTR ISR_start() {
  if(AllReady== true){
  openVault=true;
  }
}

//Default code gekreven van onderzoeksgroep dramco om wifi connectie te realiseren.
void setup_wifi(){
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
  //default lcd display setup
  lcd.init();
  lcd.clear();         
  lcd.backlight(); 
  lcd.setCursor(2,0);
  lcd.print("Voer de code in:");
  lcd.setCursor(8,2);
  lcd.print("____");
}


void open_Vault(){
  //30 seoconden flikkeren en de tijd om de gsms in het kluisje te steken
  digitalWrite(Relais_Sol, HIGH);
  for(int i = 0; i < 30; i++){
    digitalWrite(Sled, LOW);
    delay(700);
    digitalWrite(Sled, HIGH);
    delay(300);
  }
  //Zodat in de reconnect een berichten worden gestuurd naar de andere puzzels.
  //Er wordt een reconnect geforceerd door het elektromagnetisch veld van de solenoïde.
  notificatie_START=true;
  //De timer starten en slot zal sluiten.
  timerAlarmEnable(timer); 
  digitalWrite(Relais_Sol, LOW);
}

void reconnect(){
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Eindpuzzel_ESP"))
    {
    Serial.println("connected");
    //Na de geforceerde reconnect worden er hier berichten gestuurd.
    if(notificatie_START){
      client.publish("Wristbands","Herstart Wristbands"); 
      client.publish("Garbage","Buzz lawaai om te starten"); 
      notificatie_START = false;
    }
    //Subscribions    
    client.subscribe("controlpanel/status");
    client.subscribe("TrappenMaar/zone");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

//Deze functie gaat registreren en bijhouden welke puzzels klaar zijn en welke niet.
void puzzelReady(String message){
  if(message== "Trappenmaar Ready"){
    ReadyArray[0] = true;
  }
  if(message== "Traingame Ready"){
    ReadyArray[1] = true;
  }
  if(message== "Wristbands Ready"){
    Wristbands=Wristbands+1;
    if(Wristbands == 4){
    ReadyArray[2] = true;
    }
  }
  if(message== "Garbage Ready"){
    ReadyArray[3] = true;
  }
  if(message== "UV-slot Ready"){
    ReadyArray[4] = true;
  }

  //Kijken of alles ready is
  AllReady = true;
  for (int i = 0; i < sizeof(ReadyArray) ; i++)
  {
    if(!ReadyArray[i]){ 
    AllReady = false;
    }
  }
  //Zoja dan zetten we het ledje op hoog om aan te duiden dat nu de knop zal werken.
  if (AllReady)
  {
    digitalWrite(Sled, HIGH); 
  }
}


//Opent de kluis van de puzzlebox
void openSlot(){
  digitalWrite(Relais_Sol, HIGH);
    //Stop de timer
    timerAlarmDisable(timer);
    //Laten flikkeren om te vieren dat het gelukt is!
    //En voor hun tijd te geven voor hun gsms eruit te halen.
    for (size_t i = 0; i < 50; i++)
    {
      lcd.clear();
      delay(200);
      lcd.setCursor(4,1);
      lcd.print("Code correct!");
      lcd.setCursor(2,2);
      lcd.print("De deur is open!");
      delay(300);
    }
    //voor bugs te fixen
    codeJuist = false;
    keypadBlocked = true;
    for (int i = 0; i < 4; i++)
    {
      cinput[i]=0;
    }
  //sluit slot
  digitalWrite(Relais_Sol, LOW);
}

//Zet een tip op het scherm dat ze meer energie nodig hebben voor deze eindcode in te vullen.
void tip(){
  lcd.clear();        
  lcd.backlight(); 
  lcd.setCursor(1,1);
  lcd.print("Dit heeft meer");
  lcd.setCursor(5,2);
  lcd.print("energie nodig!");
  delay(6000);
  setup_lcd();
}