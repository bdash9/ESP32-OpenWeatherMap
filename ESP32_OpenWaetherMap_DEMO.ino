// ESP32_ILI9341_openweathermap

// downloads json string from openweather.com
// for environmental conditions of (any) town
// displays alphanumerically, with gauges and with icons 
//
// updates every five minutes in Loop -  check the variable named timerDelay
//
// microcontroller ESP32-WROOM-32
// current display TFT 018  =  320*240 ILI9341 controller
// implements Bodmer's TFT_ESPI library 
// implements Bodmer's rainbow scale gauge
//
// json instructions by Rui Santos
// at https://RandomNerdTutorials.com/esp32-http-get-open-weather-map-thingspeak-arduino/
//
// public domain
// July 6, 2021 - minor issues fixed, compass pointer fixed
// Floris Wouterlood
//
// Make sure all the display driver and pin conections are correct by
// editing the User_Setup.h file in the TFT_eSPI library folder.
//
// ######################################################################################
// ######   DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE TFT_eSPI LIBRARY    #####
// ######################################################################################

// Add support for Adafruit ESP32-S2 TFT Feather - With 135x240 screen - Ben

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
// #include <TFT_eSPI.h>   
#include <WiFi.h>
#include <HTTPClient.h>
//#include <ArduinoJson.h>
#include <Arduino_JSON.h>
#include "Free_Fonts.h"                                                                  // include the header file attached to this sketch - part of TFT_eSPI package

// TFT_eSPI tft = TFT_eSPI();   
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


   #define WHITE       0xFFFF
   #define BLACK       0x0000
   #define BLUE        0x001F
   #define RED         0xF800
   #define GREEN       0x07E0
   #define CYAN        0x07FF
   #define MAGENTA     0xF81F
   #define YELLOW      0xFFE0
   #define GREY        0x2108 
   #define SCALE0      0xC655                                                               // accent color for unused scale segments                                   
   #define SCALE1      0x5DEE                                                               // accent color for unused scale segments
   #define TEXT_COLOR  0xFFFF                                                               // is currently white 

// rainbow scale ring meter color scheme  
   #define RED2RED     0
   #define GREEN2GREEN 1
   #define BLUE2BLUE   2
   #define BLUE2RED    3
   #define GREEN2RED   4
   #define RED2GREEN   5

   #define DEG2RAD 0.0174532925                                                             // conversion factor degrees to radials
   
// data connection and acquisition variables
   const char* ssid =            "xxxxxxxxxx";                                // network wifi credentials  
   const char* password =        "xxxxxxxxxx";                                // wifi network key
   String openWeatherMapApiKey = "xxxxxxxxxxxxxxxxxxxxxxx";                   // API Openweathermap account https://openweathermap.org
   String city =                 "Newton";
   String countryCode =          "US";                                            // "NL" for The Netherlands
   String jsonDocument (1024);                                                          // memory allocation json string 

// timer internet access for server download
   unsigned long lastTime = 0;                                                              // testing purposes = 30 seconds
   unsigned long timerDelay;                                                                // is set at 1 seconds in Setup and to 5 minutes in Loop  
                                                        
// joint variables
   float temp_01=10;
   float hum_01;
   int   hum_02; 

// rainbow scale ring meter variables
   uint32_t runTime = -99999;                                                               // time for next update
   int   reading = 10;                                                                      // value to be displayed in circular scale
   int   tesmod = 0;
   int   rGaugePos_x = 0;                                                                   // these two variables govern the position
   int   rGaugePos_y = 155;                                                                 // of the square + gauge on the display
   int   ringmeterRadius = 65;                                                              // governs diameter of rainbow gauge
   char* ringlabel[] = {"","*F","%","mBar"};                                                // some custom labels
   float tempRainbowgauge;  
   int   t = 40;                                                                            // governs position of numerical output rainbow scale

// small needle meter 
   int   j;
   int   pivotNeedle_x = 165;                                                               // pivot coordinates needle of small gauge
   int   pivotNeedle_y = 222;      
   float center_x1 = 160;                                                                   // center x of edge markers circle left gauge 
   float center_y1 = 228;                                                                   // center y of edge markers circle left gauge         
   int   radius_s = 65;                                                                     // for scale markers
   int   needleLength = 45;                                                                 // gauge needle length
   int   edgemarkerLength = 5;                                                              // edge marker length               
   float edge_x1, edge_y1, edge_x1_out, edge_y1_out;   
   float angleNeedle = 0;
   float needle_x, needle_y;                                                                        
   float needle_x_old, needle_y_old;
   float angleCircle = 0;
   int   pivot_x = 165;                                                                     // pivot coordinates needle of small gauge
   int   pivot_y = 222;  

// circle segment 'pie chart' meter variables
   byte inc = 0;
   unsigned int col = 0;
   float pivotHumcircle__x = 198;
   float pivotHumcircle__y = 274;  
   float startAngle = 0;
   float subAngle;                                                                          // subtended angle
   int   r = 34;                                                                            // circle radius 

// compass wind direction pointer
   float compassPivot_x = 158;
   float compassPivot_y = 65;
   float c_x1, c_x2,c_x3, c_x4;
   float c_y1, c_y2,c_y3, c_y4;
   float c_x1_old,c_x2_old,c_x3_old, c_x4_old;
   float c_y1_old,c_y2_old,c_y3_old, c_y4_old; 
   
   float windDir_01;
   float compassAngle;
   int   compass_r = 22;
   char* sector [] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW", "N"};                     // wind sector labels
   int   h;                                                                                 // indicator for wind sector


void setup() {
   
   Serial.begin (9600);
   /*
   tft.init ();
   tft.setRotation (2);                                                        
   tft.fillScreen (TFT_BLACK);
   tft.setTextSize (2);
   */
   // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize (2);
   //tft.println ("Network ");
   //tft.println ("Connecting");

   WiFi.begin (ssid, password);
   tft.setTextColor (ST77XX_RED);
   tft.print ("DASH");
   tft.setTextColor (ST77XX_YELLOW);
   tft.print ("9");
   tft.setTextColor (ST77XX_BLUE);
   tft.print ("COMPUTING");
   tft.println ();
   tft.println ();
   tft.setTextColor (ST77XX_WHITE);
   tft.println ("WIFI Connecting");
     while(WiFi.status() != WL_CONNECTED) {
     delay (500);
     tft.print (".");
     tft.print ("."); 
   }
   tft.println ();
   //tft.print ("Connected!");
   //tft.println ("WIFI: ");  
   tft.print ("SSID: ");
   tft.print (ssid);
   //tft.println (ssid);
   tft.println ();    
   tft.println ();
   tft.print ("IP address: "); 
   tft.println ();
   tft.setTextColor (ST77XX_GREEN);
   tft.println (WiFi.localIP());
   delay(4000);
   tft.fillScreen(ST77XX_BLACK);

// Maybe change this to 1st screen windowframes
//
   drawAllWindowFrames ();                                                               // instructions moved to subroutine
   tft.setTextColor (ST77XX_YELLOW,BLACK);
 
   //
   // Screen 1
   // 

   //tft.setFreeFont(FF1);
   tft.setFont(FF1);

   tft.setTextSize (1);
   //tft.setCursor (21,20);
   tft.setCursor (14,20);   
   tft.print ("OpenWeather Forcast");

   tft.setCursor (9,44);
   tft.print ("Temp");
   tft.setCursor (10,65);
   tft.print ("Baro");
   tft.setCursor (10,87);
   tft.print ("Hum");
   tft.setCursor (10,109);
   tft.print ("Wind");
   //tft.setCursor (10,133);
   tft.setCursor (10,131);
   tft.print ("From:");

   //tft.setFreeFont(FF0);
   tft.setFont(FS9);                                                                    // note - units in Free Font 0 = text size 1
   //tft.setCursor (133,37);
   tft.setCursor (128,45);
   //tft.print (char(247));                                                                   // degree character
   tft.print ("F");  
   //tft.setCursor (133,59);
   tft.setCursor (126,67);
   tft.print ("mB");
   //tft.setCursor (133,81);
   tft.setCursor (128,89);
   tft.print ("%");
   //tft.setCursor (133,102);
   tft.setCursor (124,111);
   //tft.print ("m/s"); 
   tft.print("mph");
   
  // Text for DASH9 box, upper right
  //tft.setCursor (170,33);
   tft.setCursor (170,41);
   tft.setTextSize (1); 
   tft.setTextColor (ST77XX_RED);  
   tft.print ("DASH9");
   //tft.print ("v1.0 - FGW");
   tft.setTextColor (ST77XX_WHITE);  

  // Text for wind sector window above of compass on right
  // tft.setCursor (171, 51);
  // tft.setCursor (171, 59);
   tft.setCursor (164, 59);
   tft.setTextSize (1);  
   tft.setTextColor (ST77XX_BLUE); 
   tft.print ("Wind:");


// Screen 2 Text for Temperature Rainbow guage on left
   //tft.setCursor (10,133);  
   tft.setCursor (10,145);
   tft.print ("tempr");                                                                      // in rainbow scale 

   tft.setCursor (125,160);                                                                 // left upper corner = anchoring point of rainbow gauge scale
   tft.print (char(247));                                                                   // degree character
   tft.setTextSize (2);
   tft.print ("F");   
   tft.setTextSize (1); 

   tft.setCursor (55,290);                                                                  // 0  is bottom rainbow scale temperature 
   tft.print ("0");
   tft.setCursor (98,290);                                                                  // 50 is top rainbow scale temperature 
   tft.print ("50");
                                              

// Screen 2 Text for Humidity Guage with needle (small on right upper)
   tft.setCursor (220,160);                                                                 // display percent sign in  small gauge scale
   tft.print ("%");

   drawSaleSmallGauge ();
   tft.fillCircle (pivot_x, pivot_y, 2, MAGENTA);                                           // pivot needle middle small gauge                                                                         // arbitrary seeding temp - avoids drawing black line from position 0.0
   needleMeter ();                                                                          // calculate position and draw the needle
   tft.drawRoundRect (158,150,80,80,4, GREEN);                                              // correction right middle small frame from buildup needle
   tft.setCursor (216,217);                        
   tft.setTextSize (1);  
   tft.print ("40");                                                                        // scale starts with 40% and ends with 100%
   tft.setCursor (164,169);   
   tft.print ("100");    
   tft.setTextSize (2);


   timerDelay = 1000;                                                                       // this is for fast initial query OpenWeather server for json string
   doTheHardWork ();                                                                        // run this here - in void loop the timerDelay refreshes every 5 minutes 

   drawAllWindowFrames ();                                                                  // drawover to correct for all drawing errors caused by needles and markers  
   rainbowScaleMeter ();                                                                    // Screen 2 - prepare the rainbow scale meter
   needleMeter ();                                                                          // Screen 2 - prepare right, small meter with the needle
   circleSegmentMeter ();                                                                   // Screen 2 - prepare the circle segment pie meter
   compassGauge ();                                                                         // Screen 1 - prepare the wind compass meter
   compassPointer ();                                                                       // Screen 1 - add pointer to the wind compass meter
   windSectorReporter ();                                                                   // Screen 1 - add wind sector reporter
}

void loop() {

   timerDelay = 600000;  // connect to OpenWeather and refresh every ten minutes
   doTheHardWork ();
  
}


// ######################################################################################
// #               functions section                                                    #
// ######################################################################################


void doTheHardWork (){
// Send an HTTP GET request
  if ((millis () - lastTime) > timerDelay) 
      {
      // Check WiFi connection status
      if(WiFi.status ()== WL_CONNECTED)
         {
         // Use the commented out line below if you want to use the location variables above: String city =  and String countyCode =  
         // String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;    
         String serverPath = "http://api.openweathermap.org/data/2.5/weather?id=4945283&appid=" + openWeatherMapApiKey;   // You are going to want to change id=524901 to your location id=
         jsonDocument = httpGETRequest(serverPath.c_str ());
         JSONVar myObject = JSON.parse (jsonDocument);                                      // JSON.typeof(jsonVar) can be used to get the type of the var
         double temp_01 = (myObject ["main"]["temp"]); 
         double hum_01  = (myObject ["main"]["humidity"]); 
  
      if (JSON.typeof (myObject) == "undefined")
         {
         Serial.println ("Parsing input failed!");
         return;
         }
         
   Serial.println ("*******************************************");  
   Serial.print ("JSON object = ");
   Serial.println (myObject);
   Serial.println ("*******************************************");  
   
   Serial.println ("extracted from JSON object:");        
   Serial.print ("Temperature:    ");    
   Serial.print (myObject["main"]["temp"]);
   Serial.println (" *Kelvin");       
   Serial.print ("Pressure:       ");
   Serial.print (myObject["main"]["pressure"]);
   Serial.println (" mB"); 
   Serial.print ("Humidity:       ");
   Serial.print(myObject["main"]["humidity"]);
   Serial.println (" %");  
   Serial.print ("Wind Speed:     ");
   Serial.print (myObject["wind"]["speed"]);
   Serial.println (" m/s");  
   Serial.print("Wind Direction: ");
   Serial.print (myObject["wind"]["deg"]);
   Serial.println (" degrees"); 

   double windDir = (myObject ["wind"]["deg"]);
   windDir_01 = windDir; 

   compassGauge ();
   compassPointer ();
   windSectorReporter ();
   
  // print some json data in upper window of the display 
  
   //temp_01 = temp_01-273;                                    // convert temp from Kelvin to Celsius
   temp_01 = (temp_01-273) * 9/5 + 32;                     //convert Kelvin to Farenheight
   tempRainbowgauge = temp_01;
   hum_02 = hum_01;

/*
  //Not using Serial
   Serial.print ("Wind from:      ");
   Serial.println (sector[h]);   

   Serial.print ("Rainbowgauge:   ");
   Serial.print (tempRainbowgauge,1);
   Serial.println (" *C");
   Serial.println ("");
*/
  // tft.setFreeFont (FF1);                                  // start display data representation 
   tft.setFont (FF1);                                        

  // Set box space for words: temp baro hum wind from: (wind direction)  
   //tft.fillRect (60,35,70,15,BLACK);
   tft.fillRect (60,35,65,15,BLACK);
   //tft.setCursor (72,45);
   tft.setCursor (65,44);
   tft.print (temp_01,1);

   //tft.fillRect (60,55,70,15,BLACK);
   tft.fillRect (60,55,65,15,BLACK);
   //tft.setCursor (67,67);
   tft.setCursor (65,65);
   tft.print (myObject["main"]["pressure"]);
   
   //tft.fillRect (60,77,70,15,BLACK); 
   tft.fillRect (60,77,65,15,BLACK);
   //tft.setCursor (75,89);
   tft.setCursor (75,87);
   tft.print (myObject["main"]["humidity"]);
   
   //tft.fillRect (60,99,70,15,BLACK);
   tft.fillRect (60,99,65,15,BLACK);
   //tft.setCursor (69,110);
   tft.setCursor (65,109);
   //For wind speed in m/s:      
   //tft.print (myObject["wind"]["speed"]);   
   //For MPH wind speed (and comment out line above:
   double windSpeed = double(myObject["wind"]["speed"]) * 2.23694; // Conversion factor from m/s to mph is 2.23694
   tft.print(windSpeed, 1); // print the wind speed with one decimal place

  //box where wind direction -Ie. from: n,s,e,w
   tft.fillRect (65,121,40,15,BLACK); 
   //tft.setCursor (78,133); 
   tft.setCursor (76,131);
   tft.print (sector[h]);                                                                   // wind sector

   //tft.setFreeFont(FF0);                                                                    // end display numerical data representation 
   tft.setFont(FSS9);                                                                    // end display numerical data representation 

   rainbowScaleMeter ();
   needleMeter ();                                                                          // calculate needle position middle right, small meter
   circleSegmentMeter ();                                                                   // calculate and display the pie chart
    }
    else {
      Serial.println( "WiFi Disconnected");
    }
    lastTime = millis ();
  }  
}

// ######################################################################################
// #  draw window frames                                                                #
// ######################################################################################

void drawAllWindowFrames (){ 

   tft.drawRoundRect  (  4,   2, 234, 25, 4, GREEN);                                        // Screen 1 - title frame top -OpenWeather Forcast
   //tft.drawRoundRect  (  4,  28, 150,118, 4, GREEN);                                        // left upper numerical window   
   tft.drawRoundRect  (  4,  28, 150,107, 4, GREEN);                                        // Screen 1 - left numerical window  - temp, baro, etc 
   tft.drawRoundRect  (158,  28,  80, 16, 4, GREEN);                                        // Screen 1 - DASH9  
   tft.drawRoundRect  (158,  47,  80, 16, 4, GREEN);                                        // Screen 1 - Wind: sector indicator frame 
   //tft.drawRoundRect  (158,  65,  80, 81, 4, GREEN);                                        // compass frame
   tft.drawRoundRect  (158,  65,  80, 70, 4, GREEN);                                        // Screen 1 - Right lower compass frame
   tft.drawRoundRect  (  4, 150, 150,165, 4, GREEN);                                        // Screen 2- left lower (big) frame for rainbow scale gauge
   tft.drawRoundRect  (158, 150,  80, 80, 4, GREEN);                                        // Screen 2 - right upper small frame
   tft.drawRoundRect  (158, 235,  80, 80, 4, GREEN);                                        // Screen 2 - right lower small frame - needle meter - wind: SW         
}


// ######################################################################################
// #  HTTP GET sequence                                                                 #
// ######################################################################################

String httpGETRequest(const char* serverName) {

  HTTPClient http;
  http.begin(serverName);                                                                   // your IP address with path or Domain name with URL path 
  int httpResponseCode = http.GET();                                                        // send HTTP POST request
  String payload = "{}"; 
  if (httpResponseCode>0)
     {
     Serial.print ("HTTP Response code: ");
     Serial.println (httpResponseCode);
     payload = http.getString ();
     }
  else {
    Serial.print( "Error code: ");
    Serial.println (httpResponseCode);
  }
  http.end ();                                                                              // free microcontroller resources
  return payload;
}


// ######################################################################################
// #  rainbow scale meter                                                               #
// ######################################################################################
// Screen 2 - temp on left
void  rainbowScaleMeter (){
   
   if (millis () - runTime >= 100)                                                          // originally 500 = delay                                       
     {                                    
      runTime = millis ();
      if( tesmod==0)
        {
         reading =  99;
        }
      if( tesmod==1)
        {
         reading = tempRainbowgauge*2;                                                      // important: here ring is seeded with value      
        } 
 
     int xpos = 10, ypos = 240, gap = 100;                                                  // position of upper ring and proportion
     
     ringMeter (reading,0,100, (rGaugePos_x+15),(rGaugePos_y+17),ringmeterRadius,ringlabel[0],GREEN2RED);    
     tesmod = 1;
     }
}


// ######################################################################################
// #  rainbow scale: draw the rainbox ring meter, returns x coord of righthand side     #
// ######################################################################################
// Screen 2 - temp on left

int ringMeter(int value,int vmin,int vmax,int x,int y,int r, char *units, byte scheme){
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
 
   x += r; y += r;                                                                          // calculate coordinates of center of ring
   int w = r / 3;                                                                           // width of outer ring is 1/4 of radius
   int angle = 150;                                                                         // half the sweep angle of the meter (300 degrees)
   int v = map (value, vmin, vmax, -angle, angle);                                          // map the value to an angle v
   byte seg = 3;                                                                            // segments are 3 degrees wide = 100 segments for 300 degrees
   byte inc = 6;                                                                            // draw segments every 3 degrees, increase to 6 for segmented ring
   int colour = BLUE;                                                                       // variable to save "value" text color from scheme and set default
 
 
   for (int i = -angle+inc/2; i < angle-inc/2; i += inc)                                    // draw color blocks every increment degrees
     {           
      float sx = cos((i - 90) * DEG2RAD);                                                   // calculate pair of coordinates for segment start
      float sy = sin((i - 90) * DEG2RAD);
      uint16_t x0 = sx * (r - w) + x;
      uint16_t y0 = sy * (r - w) + y;
      uint16_t x1 = sx * r + x;
      uint16_t y1 = sy * r + y;
    
      float sx2 = cos((i + seg - 90) * DEG2RAD);                                            // calculate pair of coordinates for segment end
      float sy2 = sin((i + seg - 90) * DEG2RAD);
      int x2 = sx2 * (r - w) + x;
      int y2 = sy2 * (r - w) + y;
      int x3 = sx2 * r + x;
      int y3 = sy2 * r + y;

      if (i < v) 
         {                                                                                  // fill in coloured segments with 2 triangles
          switch (scheme)
             {
              case 0: colour = RED; break;                                                  // fixed color
              case 1: colour = GREEN; break;                                                // fixed color
              case 2: colour = BLUE; break;                                                 // fixed colour
              case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break;               // full spectrum blue to red
              case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break;              // green to red (high temperature etc)
              case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break;              // red to green (low battery etc)
              default: colour = BLUE; break;                                                // fixed color
             }
              tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
              tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
             }
      else                                                                                  // fill in blank segments
             {
              tft.fillTriangle(x0, y0, x1, y1, x2, y2, SCALE1);                             // color of the unoccupied ring scale 
              tft.fillTriangle(x1, y1, x2, y2, x3, y3, SCALE0);                             // color of the unoccupied ring scale
             }
             }

   tft.fillRect (t,y,72,15,BLACK);
   tft.setTextSize (2);
   if (tempRainbowgauge<-9.9) t = 33;
   if (tempRainbowgauge>-9.9) t = 38; 
   if (tempRainbowgauge > 0 ) t = 50;
   if (tempRainbowgauge >9.9) t = 42;
   tft.setCursor (t+15,y);  
   tft.setTextColor (GREEN);     
   tft.print (tempRainbowgauge,1);
}


// ######################################################################################
// #  rainbow scale: 16-bit rainbow color mixer                                         #
// ######################################################################################
// Screen 2 - temp on left

unsigned int rainbow (byte value) {                                                         // value is expected to be in range 0-127
                                                                                            // value is converted to a spectrum color from 0 = blue through to 127 = red
   byte red = 0;                                                                            // red is the top 5 bits of a 16 bit colour value
   byte green = 0;                                                                          // green is the middle 6 bits
   byte blue = 0;                                                                           // blue is the bottom 5 bits
   byte quadrant = value / 32;

   if (quadrant == 0)
     {
      blue = 31;
      green = 2 * (value % 32);
      red = 0;
     }
   if (quadrant == 1)
     {
      blue = 31 - (value % 32);
      green = 63;
      red = 0;
     }
   if (quadrant == 2)
     {
      blue = 0;
      green = 63;
      red = value % 32;
     }
   if (quadrant == 3)
     {
      blue = 0;
      green = 63 - 2 * (value % 32);
      red = 31;
     }
   return (red << 11) + (green << 5) + blue;
}


// ######################################################################################
// # rainbox scale: return a value in range -1 to +1 for a given phase angle (degrees)  #
// ######################################################################################
// Screen 2 - temp on left

float sineWave(int phase) {
  
   return sin(phase * 0.0174532925);
}


// ######################################################################################
// #  small needle meter       - scale creation                                         #
// ######################################################################################
//Screen 2 - needle meter (cont below) - upper right

void drawSaleSmallGauge (){
 
   j = 270;                                                                                 // start point of cirle segment
   do {
       angleCircle = (j* DEG2RAD);                                                          // angle expressed in radians - 1 degree = 0,01745331 radians      

       edge_x1 = (center_x1+4 + (radius_s*cos (angleCircle)));                              // scale - note the 4 pixels offset in x      
       edge_y1 = (center_y1 + (radius_s*sin (angleCircle)));                                // scale
         
       edge_x1_out = (center_x1+4 + ((radius_s+edgemarkerLength)*cos (angleCircle)));       // scale - note the 4 pixels offset in x   
       edge_y1_out = (center_y1 + ((radius_s+edgemarkerLength)*sin (angleCircle)));         // scale
        
       tft.drawLine (edge_x1, edge_y1, edge_x1_out, edge_y1_out,MAGENTA); 
 
       j = j+6; 
   } 
   while (j<356);                                                                           // end of circle segment
}


// ######################################################################################
// #  small needle meter       - dynamic needle part                                    #
// ######################################################################################
//Screen 2 -Humidity needle upper right 

void needleMeter (){                                                                         

   tft.drawLine (pivotNeedle_x, pivotNeedle_y, needle_x_old, needle_y_old, 0);              // remove old needle by overwritig in white
   
   angleNeedle = (420*DEG2RAD - 1.5*hum_02*DEG2RAD);                                        // contains a 1.5 stretch factor to expand 60 percentage points over 90 degrees of scale

   if (angleNeedle > 6.28) angleNeedle = 6.28;                                              // prevents the needle from ducking below horizontal    
   needle_x = (pivotNeedle_x + ((needleLength)*cos (angleNeedle)));                         // calculate x coordinate needle point
   needle_y = (pivotNeedle_y + ((needleLength)*sin (angleNeedle)));                         // calculate y coordinate needle point
   needle_x_old = needle_x;                                                                 // remember previous needle position
   needle_y_old = needle_y;

   tft.drawLine (pivotNeedle_x, pivotNeedle_y, needle_x, needle_y,MAGENTA); 
   tft.fillCircle (pivotNeedle_x, pivotNeedle_y, 2, MAGENTA);                               // restore needle pivot
}


// ######################################################################################
// #  circle segment meter - main body                                                  #
// ######################################################################################
// Screen 2 - Humidity Pie Chart in bottom right

 void circleSegmentMeter (){

   tft.fillCircle (pivotHumcircle__x, pivotHumcircle__y,34,BLACK);       
   fillSegment (pivotHumcircle__x, pivotHumcircle__y, 0, (360*hum_02/100), 34, ST77XX_RED);    // draw pie chart segment
   tft.drawCircle (pivotHumcircle__x, pivotHumcircle__y,35,YELLOW);                         // correct for faulty pixels circle edge
 
   tft.setCursor (pivotHumcircle__x+10, pivotHumcircle__y-5); 
   tft.setTextColor (ST77XX_YELLOW);
   tft.setTextSize (1);
   tft.print ("hum");
   tft.setCursor (pivotHumcircle__x+10, pivotHumcircle__y+5);  
   tft.print (hum_02);
   tft.print ("%");  
   tft.setTextSize (2);
}


// ######################################################################################
// #  circle segment meter - draw segments                                              #
// ######################################################################################
//Screen 2 - Temp left side

// x,y == coords of centre of circle
// startAngle = 0 - 359
// subAngle   = 0 - 360 = subtended angle
// r = radius
// colour = 16 bit colour value

int fillSegment (int x, int y, int startAngle, int subAngle, int r, unsigned int colour)
{
  float sx = cos((startAngle - 90) * DEG2RAD);                                              // calculate first pair of coordinates for segment start
  float sy = sin((startAngle - 90) * DEG2RAD);
  uint16_t x1 = sx * r + x;
  uint16_t y1 = sy * r + y;

  for (int i = startAngle; i < startAngle + subAngle; i++)                                  // draw color blocks every inc degrees 
     {    
     int x2 = cos((i + 1 - 90) * DEG2RAD) * r + x;                                          // calculate pair of coordinates for segment end
     int y2 = sin((i + 1 - 90) * DEG2RAD) * r + y;
     tft.fillTriangle(x1, y1, x2, y2, x, y, colour);
     x1 = x2;                                                                               // copy segment end to segment start for next segment
     y1 = y2;
     }
}


// ######################################################################################
// #  constructs the compass gauge     - static part of compass                         #
// ######################################################################################
//Screen 1 - Wind direction guage bottom right

void compassGauge (){

   tft.drawCircle ((compassPivot_x+40), (compassPivot_y+40), compass_r+11, BLUE);           // outer circle compass 
   tft.drawCircle ((compassPivot_x+40), (compassPivot_y+40), compass_r+2, CYAN);            // inner circle compass - pointer fits in here
   tft.setTextSize (1);
   //tft.setCursor (compassPivot_x+38, compassPivot_y+4);
   tft.setCursor (compassPivot_x+36, compassPivot_y+12);
   tft.setTextColor (WHITE, BLACK);
   tft.print ("N");
   //tft.setCursor (compassPivot_x+38, compassPivot_y+69);
   tft.setCursor (compassPivot_x+36, compassPivot_y+69);
   tft.print ("S");
   //tft.setCursor (compassPivot_x+71, compassPivot_y+35);
   tft.setCursor (compassPivot_x+68, compassPivot_y+43);
   tft.print ("E");
   //tft.setCursor (compassPivot_x+5,  compassPivot_y+35);
   tft.setCursor (compassPivot_x+2,  compassPivot_y+43);
   tft.print ("W");  
}


// ######################################################################################
// #  constructs the compass pointer     - dynamic part of compass                      #
// ######################################################################################
// Screen 1 - Wind compass bottom right

void compassPointer (){                                                                         
  
   tft.fillTriangle (c_x1_old,c_y1_old,c_x2_old,c_y2_old,c_x4_old,c_y4_old,BLACK);          // remove old compass pointer by overwritig in white
   tft.fillTriangle (c_x1_old,c_y1_old,c_x3_old,c_y3_old,c_x4_old,c_y4_old,BLACK);          // remove old compass pointer by overwritig in white
  
   compassAngle = ((windDir_01-90)*DEG2RAD);                                        

   c_x1 = ((compassPivot_x+40) + (compass_r * cos (compassAngle)));                         // calculate coordinates compass pointer tip 
   c_y1 = ((compassPivot_y+40) + (compass_r * sin (compassAngle)));                       

   c_x2 = ((compassPivot_x+40) + (compass_r * cos (compassAngle + 163*DEG2RAD)));           // calculate coordinates compass pointer base point
   c_y2 = ((compassPivot_y+40) + (compass_r * sin (compassAngle + 163*DEG2RAD)));                       

   c_x3 = ((compassPivot_x+40) + (compass_r * cos (compassAngle - 163*DEG2RAD)));           // calculate coordinates compass pointer base point
   c_y3 = ((compassPivot_y+40) + (compass_r * sin (compassAngle - 163*DEG2RAD)));                       

   c_x4 = ((compassPivot_x+40) + ((compass_r-4) * cos (compassAngle - 180*DEG2RAD)));       // calculate coordinates compass pointer base point
   c_y4 = ((compassPivot_y+40) + ((compass_r-4) * sin (compassAngle - 180*DEG2RAD)));                 

   c_x1_old = c_x1; c_x2_old = c_x2; c_x3_old = c_x3;   c_x4_old = c_x4;
   c_y1_old = c_y1; c_y2_old = c_y2; c_y3_old = c_y3;   c_y4_old = c_y4;
   
   tft.fillTriangle (c_x1, c_y1,c_x3, c_y3, c_x4, c_y4, 0x3A72);                            // print the new pointer to display                              
   tft.fillTriangle (c_x1, c_y1,c_x2, c_y2, c_x4, c_y4, BLUE);                              // print the new pointer to display
}


// ######################################################################################
// #  calculates wind sector                                                            #
// ######################################################################################
//Screen 1 - bottom right

void windSectorReporter (){

   h = 0;
   if (windDir_01 <22.5) h = 0;
   if (windDir_01> 22.5) h = 1; 
   if (windDir_01> 67.5) h = 2;
   if (windDir_01>112.5) h = 3;
   if (windDir_01>157.5) h = 4;  
   if (windDir_01>202.5) h = 5;
   if (windDir_01>247.5) h = 6;
   if (windDir_01>292.5) h = 7; 
   if (windDir_01>337.5) h = 8;

   tft.fillRect (208,51,16,8,BLACK);
 
 //wind:  Ie. SW
   //tft.setCursor (210,51); 
   tft.setCursor (209,59); 
   tft.print (sector[h]);    
}
