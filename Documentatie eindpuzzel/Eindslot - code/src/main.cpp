#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>
#include "Keypad.h"
#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

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


bool opgelost = false;
bool AllReady = false;
bool ReadyArray[5];
bool reset= false;
bool keypadBlocked = false;

//Start reset knoppen
const int Spin = 14;
const int Rpin = 13;
const int Sled = 27;
const int Rled = 23;

int startbutton_status = 0;
int resetbutton_status = 0;

//code slot
int cinput[6];
int code[] = {1,2,3,4};

//
bool enterRoom = false;

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

// Clock display pins (digital pins)
#define CLK 26
#define DIO 25
TM1637Display display = TM1637Display(CLK, DIO);

//timer interrupt
int t1 = 0;
int t2 = 0;
int adc_read_counter = 0;
hw_timer_t * timer = NULL; 

//timer variables
long int count_time = 3600000;      // 1000ms in 1sec, 60secs in 1min, 60mins in 1hr. So, 1000x60x60 = 3600000ms = 1hr
unsigned long NewTime = 0;
unsigned long current_time = 0;
unsigned long previous_time = 0;
volatile short int start_pause = 0;
int seconds, minutes;             // For switching between left & right part of the display(Even = left, Odd = right)


//interupt handlers
  //for timer
void IRAM_ATTR onTimer() {
  NewTime = count_time - 1;                       // Calculate the time remaining 
  seconds = (NewTime % 60);                       // To display the countdown in mm:ss format
  minutes = ((NewTime / 60) % 60 );               //separate CountTime in two parts
  
  display.showNumberDecEx(seconds, 0, true, 2, 2); // Display the seconds in the last two places
  display.showNumberDecEx(minutes, 0b11100000, true, 2, 0); // Display the minutes in the first two places, with colon

  count_time = NewTime;                  // Update the time remaining
}
  //for restart button
void IRAM_ATTR ISR_restart() {
  reset= true;
}


  //for start button
void enter_room(){
  digitalWrite(Relais_Sol, HIGH);
  delay(5000);
  timerAlarmEnable(timer); 
  digitalWrite(Relais_Sol, LOW);
}
void IRAM_ATTR ISR_start() {
  if(AllReady== true){
  enterRoom=true;
  }
}

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

void reconnect(){
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // CREATE UNIQUE client ID!
    // in Mosquitto broker enable anom. access
    if (client.connect("Eindpuzzel_ESP"))
    {
      Serial.println("connected");
      // Subscribe
      // Vul hieronder in naar welke directories je gaat luisteren.
      //Voor de communicatie tussen de puzzels, check "Datacommunicatie.docx". (terug tevinden in dezelfde repository) 
      client.subscribe("controlpanel/status");
      client.subscribe("controlpanel/reset");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

void check_message(String message){
  if(message== "UV-slot Ready"){
    ReadyArray[4] = true;

      AllReady = true;
      digitalWrite(Sled, HIGH);
  }
  /*
  for (int i = 0; i < sizeof(ReadyArray) ; i++)
  {
    AllReady = true;
    digitalWrite(Sled, HIGH);    
  }
  */
  
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
  Serial.println();

  if (strcmp(topic,"controlpanel/status") == 0) 
  {
   check_message(messageTemp);
  }
}

void setup() {
  pinMode(Relais_Sol,OUTPUT);
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

  //timer interrupt setup
  timer = timerBegin(0, 80, true);                //Begin timer with 1 MHz frequency (80MHz/80)
  timerAttachInterrupt(timer, &onTimer, true);   //Attach the interrupt to Timer1
  timerAlarmWrite(timer, 1000000, true);      //Initialize the timer. The 1000000 in this line means the alarm should go off every 1000000 cycles of the clock. 1000000/1000000 = 1s
  
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

void openDeur(){
  //Opent de deur van de escape room
  digitalWrite(Relais_Sol, HIGH);
    timerAlarmDisable(timer);
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
    opgelost = false;
    keypadBlocked = true;
    for (int i = 0; i < 4; i++)
    {
      cinput[i]=0;
    }
  digitalWrite(Relais_Sol, LOW);
}

void loop(){
  //Connectie checken en tesnoods reconnecten
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();


  if (enterRoom)
  {
    digitalWrite(Relais_Sol, HIGH);
    delay(30000);
    timerAlarmEnable(timer); 
    digitalWrite(Relais_Sol, LOW);
    enterRoom = false;
  }

  

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }

  if(reset){
    client.publish("controlpanel/reset","Reset escaperoom");
    Serial.println("Het reset signaal is gestuurd naar alle puzzeles.");
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
  
  if(opgelost){ //Als de code klopt 
    //gaat de deur open
    openDeur();    
  }
  //Als de knop wordt ingedrukt en de keypad niet geblokt moet worden
  else if(key && !keypadBlocked ){
      /*
      Serial.print("De waarde uit het keypad is: ");
      Serial.println(key);
      */

      //Als # (enter wordt ingedrukt)
      if(key =='#'){    
        if(c ==12){ //De positie is het laatste cijfer
        opgelost = true; //Controleer of de code klopt
        for(int i = 0;i<4;i++){
          if(code[i]!=cinput[i]){
            opgelost = false;
            lcd.setCursor(8,2);
            lcd.print("____");
            c = 8;
          }
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
  
      else{ //Als er iets anders (cijfer) wordt ingedrukt
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

