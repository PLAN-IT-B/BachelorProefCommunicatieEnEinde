#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Keypad.h"

bool bl = false;

//Start reset knoppen
const int startpin = 32;
const int resetpin = 33;

int startbutton_status = 0;
int resetbutton_status = 0;

//code 
int cinput[4];
int code[] = {1,1,1,1};


//keypad 
const byte ROWS = 4; 
const byte COLS = 3; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {18, 5, 17, 16}; 
                    //2, 7, 6, 4
byte colPins[COLS] = {4, 15, 2}; 
                    //3, 1, 5 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);
int c =8;

void setup() {
  pinMode(19,OUTPUT);
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


  lcd.init();
  lcd.clear();         
  lcd.backlight();

    //Zet het beginscherm op de lcd
    lcd.setCursor(2,0);
    lcd.print("Voer de code in:");
    lcd.setCursor(8,2);
    lcd.print("____");
}



void loop() {
  // put your main code here, to run repeatedly:

  startbutton_status = digitalRead(startpin);
  resetbutton_status = digitalRead(resetpin);

  if(startbutton_status == HIGH){
    //doe iets om de timer te starten
    Serial.println("startknop");
    startbutton_status = LOW;
    
  }
  if(resetbutton_status == HIGH){
    //doe iets om een signaal te sturen naar de rest van de puzzels
    Serial.println("resetknop");
    resetbutton_status = LOW;
  }
  char key = customKeypad.getKey();
  
  if (key){
    Serial.println(key);
  }

    // //Tegen flikkeren
    // if(bl ==false){
    //   lcd.backlight();
    //   bl = true;
    // }
    
    //Als de knop wordt ingedrukt
    if(key){
      
      //Als # (enter wordt ingedrukt)
      if(key =='#'){    
        if(c ==12){ //De positie is het laatste cijfer
        boolean check = true; //Controleer of de code klopt
        for(int i = 0;i<4;i++){
          if(code[i]!=cinput[i]){
            check = false;
          }
        }

        if(check == true){ //Als de code klopt wordt de puzzel actief
          digitalWrite(19, HIGH);
          while (true)
          {
          lcd.clear();
          delay(200);
          lcd.setCursor(4,1);
          lcd.print("Code correct!");
          lcd.setCursor(2,2);
          lcd.print("De deur is open!");
          delay(1000);

          }  
        }

        else{
          lcd.setCursor(8,2);
          lcd.print("____");
          c = 8;
        }
      }
    }
   

    else if(key == '*'){ //Als * (terug) wordt ingevuld
      
        //Ga 1 terug, vervang het getal door _ en vervang de code door 0 (standaard getal in rij)
        c--;
        lcd.setCursor(c,2);
        lcd.print("_");
        cinput[c-8] = 0;


      
    }
  
    
    else{ //Als er iets anders (cijfer) wordt ingedrukt
      if(c<8){
      Serial.println("Cijfer");
      Serial.println(c);
      }

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
