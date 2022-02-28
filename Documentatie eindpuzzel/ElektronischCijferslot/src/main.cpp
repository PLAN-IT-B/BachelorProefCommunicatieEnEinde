// Demo based on:
// UTFT_Demo_320x240 by Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
/*

 This sketch uses the GLCD and font 2 only.
 
 Make sure all the display driver and pin connections are correct by
 editing the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
 */

#include "SPI.h"

#include "TFT_eSPI.h"

#define TFT_DARK 0x41E7
#define TFT_LIGHT 0xBD55

TFT_eSPI myGLCD = TFT_eSPI();       // Invoke custom library

unsigned long runTime = 0;
void setup()
{
  randomSeed(analogRead(A0));
// Setup the LCD
  myGLCD.init();
  myGLCD.setRotation(0);

//Code screen
  myGLCD.fillScreen(TFT_DARK);

  int x = 10;
  int y = 80; 

  for (int i = 0; i < 5; i++)
  {
    for (int i = 0; i < 4; i++)
    {
      myGLCD.fillRoundRect(10,100, 240, 160, 3,TFT_LIGHT);
      x=x+40;
    }
    y=y+50;

  }
  
  myGLCD.fillRoundRect(10,100, 240, 160, 3,TFT_LIGHT);
  
  myGLCD.setTextColor(TFT_WHITE,TFT_RED);
  /*
  myGLCD.drawCentreString("Sander!", 160, 80, 2 );
  myGLCD.drawCentreString("Dit heb ik zelf geschreven!", 160, 100,2);
  myGLCD.drawCentreString("Restarting in a", 160, 119,2);
  myGLCD.drawCentreString("few seconds...", 160, 132,2);
  */

}

void loop()
{
  
// Clear the screen and draw the frame
/*  myGLCD.fillScreen(TFT_BLACK);

  myGLCD.fillRect(0, 0, 319, 14,TFT_RED);

  myGLCD.fillRect(0, 226, 319, 14,TFT_GREY);

  myGLCD.setTextColor(TFT_BLACK,TFT_RED);
  myGLCD.drawCentreString("* TFT_eSPI *", 160, 4, 1);
  myGLCD.setTextColor(TFT_YELLOW,TFT_GREY);
  myGLCD.drawCentreString("Adapted by Bodmer", 160, 228,1);

  myGLCD.drawRect(0, 14, 319, 211, TFT_BLUE);

delay(1000);

  myGLCD.fillRect(1,15,317,209,TFT_BLACK);

int col = 0;
*/
/*
// Draw a moving sinewave
  x=1;
  for (int i=1; i<(317*20); i++) 
  {
    x++;
    if (x==318)
      x=1;
    if (i>318)
    {
      if ((x==159)||(buf[x-1]==119))
        col = TFT_BLUE;
      else
      myGLCD.drawPixel(x,buf[x-1],TFT_BLACK);
    }
    y=119+(sin(((i*1.1)*3.14)/180)*(90-(i / 100)));
    myGLCD.drawPixel(x,y,TFT_BLUE);
    buf[x-1]=y;
  }
*/


 

}


