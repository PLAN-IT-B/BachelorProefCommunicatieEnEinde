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

//Start reset knoppen
const int startpin = 32;
const int resetpin = 33;

int startbutton_status = 0;
int resetbutton_status = 0;

//code slot
int cinput[4];
int code[] = {1,1,1,1};


//keypad configuratie
const byte ROWS = 4; 
const byte COLS = 3; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {18, 5, 17, 16}; // <= De pinnen van de arduino
                    //2, 7, 6, 4  <= de pinnen van de keypad
byte colPins[COLS] = {4, 15, 2}; // <= De pinnen van de arduino
                    //3, 1, 5 <= de pinnen van de keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);
int c =8; //pointer x-as

//Relais 
const int relaispin = 19;

// Clock display pins (digital pins)
#define CLK 26
#define DIO 25
TM1637Display display = TM1637Display(CLK, DIO);

//timer interrupt
int t1 = 0;
int t2 = 0;
int adc_read_counter = 0;
hw_timer_t * timer = NULL; 

// Global Variables
long int count_time = 3600000;       // 1000ms in 1sec, 60secs in 1min, 60mins in 1hr. So, 1000x60x60 = 3600000ms = 1hr
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
    client.publish("controlpanel/reset","Reset escaperoom");
    ESP.restart();
  }
  //for restart button
   void IRAM_ATTR ISR_start() {
    timerAlarmEnable(timer); 
  }


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

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // CREATE UNIQUE client ID!
    // in Mosquitto broker enable anom. access
    if (client.connect("ESP8266Client1"))
    {
      Serial.println("connected");
      // Subscribe
      // Vul hieronder in naar welke directories je gaat luisteren.
      //Voor de communicatie tussen de puzzels, check "Datacommunicatie.docx". (terug tevinden in dezelfde repository) 
      client.subscribe("controlpanel/status");
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
}

void setup() {
  pinMode(relaispin,OUTPUT); 
  pinMode(startpin, INPUT);
  pinMode(resetpin, INPUT);
  Serial.begin (115200);
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
  
  attachInterrupt(resetpin, ISR_restart, FALLING);
  attachInterrupt(startpin, ISR_start, FALLING);

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

  //Lezen wat de status is van de knoppen
  startbutton_status = digitalRead(startpin);
  resetbutton_status = digitalRead(resetpin);

  char key = customKeypad.getKey();
  
  if(opgelost == true){ //Als de code klopt 
    //gaat de deur open
    digitalWrite(19, HIGH);
    timerAlarmDisable(timer);
    for (size_t i = 0; i < 50; i++)
    {
      lcd.clear();
      delay(100);
      lcd.setCursor(4,1);
      lcd.print("Code correct!");
      lcd.setCursor(2,2);
      lcd.print("De deur is open!");
      delay(200);
    }
    opgelost = false;
    for (int i = 0; i < 4; i++)
    {
      cinput[i]=0;
    }
    digitalWrite(19, LOW);
  }
  else{

    //Als de knop wordt ingedrukt
    if(key){

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
    key == NULL; //Reset het key signaal
    }
    
  }

}