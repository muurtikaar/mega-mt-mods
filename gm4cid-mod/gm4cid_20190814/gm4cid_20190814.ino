/* megaMorseTutorGM4CID 2019_08_14
      Author:   Bruce E. Hall, w8bh.net
        Date:   05 Aug 2019
    Hardware:   STM32F103C "Blue Pill", 2.2" ILI9341 TFT display, Piezo
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
       Legal:   Copyright (c) 2019  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Practice sending & receiving morse code
                Inspired by Jack Purdum's "Morse Code Tutor"
                
                >> THIS VERSION UNDER DEVELOPMENT <<<

**************************************************************************/

/*
Modification: Ken, KM4NFQ "Not Fully Qualified"
        Date: 6 August 2019
    Hardware: Mega 2560 Pro Mini
    Software: Arduino IDE 1.8.9
       Legal: Open Source under the terms of the MIT License.
 Description: A 'port' of the W8BH 'Blue Pill' Morse Tutor to a Mega 2560
*/

/*
Modification: Bob, GM4CID
        Date: 14 August 2019
    Hardware: Mega 2560 R3, 2.4" TFT shield
    Software: Added MCUFRIRND_khv library for TFT shield 
 Description: The TFT shield requires less wiring. 
              It plugs directly in to the Mega2560 R3 full size board
              On rear of Mega board wire add 11 to 51 (MOSI), 12 to 50 (MISO)
              and 13 to 52 (SCK). Pins 11, 12 & 13 are set Hi Impedance.
 */

//===================================  INCLUDES ========================================= 
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h> 
 MCUFRIEND_kbv tft;
#include "EEPROM.h"
#include "SD.h"

//===================================  Hardware Connections =============================
#define LCD_RD             A0    // LCD RD  goes to Analog 0
#define LCD_WR             A1    // LCD WR  goes to Analog 1
#define LCD_CD             A2    // LCD RS  goes to Analog 2
#define LCD_CS             A3    // LCD CS  goes to Analog 3
#define LCD_RESET          A4    // LCD RST goes to Analog 4
                        // A8       GND/0V to Piezo element
#define PIEZO              A9    // CHG to Piezo element
                       // A11       GND/0V to Morse Paddle
#define PADDLE_A          A12    // CHG Morse Paddle "dit"
#define PADDLE_B          A13    // CHG Morse Paddle "dah"
#define LED                13    // CHG onboard LED pin
#define ENCODER_BUTTON     18    // CHG Rotary Encoder switch, interrupt pin
                        // 19       GND/0V to Encoder and switch
#define ENCODER_A          20    // CHG Rotary Encoder output  interrupt pin
#define ENCODER_B          21    // CHG Rotary Encoder output  interrupt pin            
#define SD_CS              10    // CHG SD card "CS"
#define SCREEN_ROTATION     1    // landscape mode: use '1' or '3'
//===================================  Morse Code Constants =============================
#define MYCALL          "GM4CID"                  // your callsign for splash scrn & QSO
#define DEFAULTSPEED       12                     // character speed in Words per Minute
#define MAXSPEED           50                     // fastest morse speed in WPM
#define MINSPEED            3                     // slowest morse speed in WPM
#define DEFAULTPITCH      600                     // default pitch in Hz of morse audio
#define MAXPITCH         2000                     // highest allowed pitch
#define MINPITCH          300                     // how low can you go
#define WORDSIZE            5                     // number of chars per random word
#define FLASHCARDDELAY   2000                     // wait in mS between cards
#define ENCODER_TICKS       3                     // Ticks required to register movement

//===================================  Color Constants ==================================
#define BLACK          0x0000
#define BLUE           0x001F
#define RED            0xF800
#define GREEN          0x07E0
#define CYAN           0x07FF
#define MAGENTA        0xF81F
#define YELLOW         0xFFE0
#define WHITE          0xFFFF
#define DKGREEN        0x03E0

// ==================================  Menu Constants ===================================
#define DISPLAYWIDTH      320                     // Number of LCD pixels in long-axis
#define DISPLAYHEIGHT     240                     // Number of LCD pixels in short-axis
#define TOPDEADSPACE       30                     // All submenus appear below top line
#define MENUSPACING       100                     // Width in pixels for each menu column
#define ROWSPACING         23                     // Height in pixels for each text row
#define COLSPACING         12                     // Width in pixels for each text character
#define MAXCOL   DISPLAYWIDTH/COLSPACING          // Number of characters per row    
#define MAXROW  (DISPLAYHEIGHT-TOPDEADSPACE)/ROWSPACING  // Number of text-rows per screen
#define FG              GREEN                     // Menu foreground color 
#define BG              BLACK                     // Menu background color
#define SELECTFG         BLUE                     // Selected Menu foreground color
#define SELECTBG        WHITE                     // Selected Menu background color
#define TEXTCOLOR      YELLOW                     // Default non-menu text color
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))    // Handy macro for determining array sizes

//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

//===================================  Rotary Encoder Variables =========================
volatile int      rotaryCounter    = 0;           // "position" of rotary encoder (increments CW) 
volatile boolean  button_pressed   = false;       // true if the button has been pushed
volatile boolean  button_released  = false;       // true if the button has been released (sets button_downtime)
volatile long     button_downtime  = 0L;          // ms the button was pushed before released

//===================================  Morse Code Variables =============================

                    // The following is a list of the 100 most-common English words, in frequency order.
                    // See: https://www.dictionary.com/e/common-words/
                    // from The Brown Corpus Standard Sample of Present-Day American English 
                    // (Providence, RI: Brown University Press, 1979)
                    
char *words[]     = {"THE", "OF", "AND", "TO", "A", "IN", "THAT", "IS", "WAS", "HE", 
                     "FOR", "IT", "WITH", "AS", "HIS", "ON", "BE", "AT", "BY", "I", 
                     "THIS", "HAD", "NOT", "ARE", "BUT", "FROM", "OR", "HAVE", "AN", "THEY", 
                     "WHICH", "ONE", "YOU", "WERE", "ALL", "HER", "SHE", "THERE", "WOULD", "THEIR", 
                     "WE", "HIM", "BEEN", "HAS", "WHEN", "WHO", "WILL", "NO", "MORE", "IF", 
                     "OUT", "SO", "UP", "SAID", "WHAT", "ITS", "ABOUT", "THAN", "INTO", "THEM", 
                     "CAN", "ONLY", "OTHER", "TIME", "NEW", "SOME", "COULD", "THESE", "TWO", "MAY", 
                     "FIRST", "THEN", "DO", "ANY", "LIKE", "MY", "NOW", "OVER", "SUCH", "OUR", 
                     "MAN", "ME", "EVEN", "MOST", "MADE", "AFTER", "ALSO", "DID", "MANY", "OFF", 
                     "BEFORE", "MUST", "WELL", "BACK", "THROUGH", "YEARS", "MUCH", "WHERE", "YOUR", "WAY"  
                    };
char *antenna[]   = {"DIPOLE", "VERTICAL", "BEAM"};
char *weather[]   = {"WARM", "SUNNY", "CLOUDY", "COLD", "RAIN", "SNOW", "FOGGY"};
char *names[]     = {"WAYNE", "JOHN", "TERRY", "JANE", "SUE", "LEON", "DARREN", "DOUG", "ZEB", "JOSH", "JILL", "LYNN"};
char *cities[]    = {"MEDINA, OH", "BILLINGS, MT", "SAN DIEGO", "WALLA WALLA, WA", "VERO BEACH, FL", "NASHVILLE, TN", "NYC", "CHICAGO", "LOS ANGELES", // 0-8
                    "POSSUM TROT, MS", "ASPEN, CO", "AUSTIN, TX", "RALEIGH, NC"};
char *rigs[]      = {"YAESU FT101", "KENWOOD 780", "ELECRAFT K3", "HOMEBREW", "QRPLABS QCX", "ICOM 7410", "FLEX 6400"};
char punctuation[]= "!@$&()-+=,.:;'/";
char prefix[]     = {'A', 'W', 'K', 'N'};
char koch[]       = "KMRSUAPTLOWI.NJEF0Y,VG5/Q9ZH38B?427CLD6X";

byte morse[] = {                                  // Each character is encoded into an 8-bit byte:
  0b01001010,        // ! exclamation        
  0b01101101,        // " quotation          
  0b01010111,        // # pound                   // No Morse, mapped to SK
  0b10110111,        // $ dollar or ~SX
  0b00000000,        // % percent
  0b00111101,        // & ampersand or ~AS
  0b01100001,        // ' apostrophe
  0b00110010,        // ( open paren
  0b01010010,        // ) close paren
  0b0,               // * asterisk                // No Morse
  0b00110101,        // + plus or ~AR
  0b01001100,        // , comma
  0b01011110,        // - hypen
  0b01010101,        // . period
  0b00110110,        // / slant   
  0b00100000,        // 0                         // Read the bits from RIGHT to left,   
  0b00100001,        // 1                         // with a "1"=dit and "0"=dah
  0b00100011,        // 2                         // example: 2 = 11000 or dit-dit-dah-dah-dah
  0b00100111,        // 3                         // the final bit is always 1 = stop bit.
  0b00101111,        // 4                         // see "sendElements" routine for more info.
  0b00111111,        // 5
  0b00111110,        // 6
  0b00111100,        // 7
  0b00111000,        // 8
  0b00110000,        // 9
  0b01111000,        // : colon
  0b01101010,        // ; semicolon
  0b0,               // <                         // No Morse
  0b00101110,        // = equals or ~BT
  0b0,               // >                         // No Morse
  0b01110011,        // ? question
  0b01101001,        // @ at or ~AC
  0b00000101,        // A 
  0b00011110,        // B
  0b00011010,        // C
  0b00001110,        // D
  0b00000011,        // E
  0b00011011,        // F
  0b00001100,        // G
  0b00011111,        // H
  0b00000111,        // I
  0b00010001,        // J
  0b00001010,        // K
  0b00011101,        // L
  0b00000100,        // M
  0b00000110,        // N
  0b00001000,        // O
  0b00011001,        // P
  0b00010100,        // Q
  0b00001101,        // R
  0b00001111,        // S
  0b00000010,        // T
  0b00001011,        // U
  0b00010111,        // V
  0b00001001,        // W
  0b00010110,        // X
  0b00010010,        // Y 
  0b00011100         // Z
};

int charSpeed = DEFAULTSPEED;
int codeSpeed = DEFAULTSPEED;
int ditPeriod = 100;
int ditPaddle = PADDLE_A;
int dahPaddle = PADDLE_B;
int pitch     = DEFAULTPITCH;
int kochLevel = 1;
bool paused   = false;

//===================================  Menu Variables ===================================
int  menuCol=0, textRow=0, textCol=0;
char *mainMenu[] = {" Receive ", "  Send   ", "  Config "};        
char *menu0[]    = {" Koch    ", " Letters ", " Words   ", " Numbers ", " Mixed   ", " SD Card ", " QSO     ", " Callsign", " Exit    "};
char *menu1[]    = {" Practice", " Copy One", " Copy Two", " Cpy Word", " Cpy Call", " Flashcrd", " Two-Way ", " Exit    "};
char *menu2[]    = {" Speed   ", " Char Spd", " Chk Spd ", " Tone    ", " Dit Pad ", " Defaults", " Exit    "};


//===================================  Rotary Encoder Code  =============================

/* 
   Rotary Encoder Button Interrupt Service Routine ----------
   Process encoder button presses and releases, including debouncing (extra "presses" from 
   noisy switch contacts). If button is pressed, the button_pressed flag is set to true. If 
   button is released, the button_released flag is set to true, and button_downtime will 
   contain the duration of the button press in ms.  Manually reset flags after handling event. 
*/

void buttonISR()
{  
  static boolean button_state = false;
  static unsigned long start, end;  
  boolean pinState = digitalRead(ENCODER_BUTTON);
  
  if ((pinState==LOW) && (button_state==false))                     
  {                                               // Button was up, but is now down
    start = millis();                             // mark time of button down
    if (start > (end + 10))                       // was button up for 10mS?
    {
      button_state = true;                        // yes, so change state
      button_pressed = true;
    }
  }
  else if ((pinState==HIGH) && (button_state==true))                       
  {                                               // Button was down, but now up
    end = millis();                               // mark time of release
    if (end > (start + 10))                       // was button down for 10mS?
    {
      button_state = false;                       // yes, so change state
      button_released = true;
      button_downtime = end - start;              // and record how long button was down
    }
  }
}

/* 
   Rotary Encoder Interrupt Service Routine ---------------
   This is an alternative to rotaryISR() above.
   It gives twice the resolution at a cost of using an additional interrupt line
   This function will runs when either encoder pin A or B changes state.
   The states array maps each transition 0000..1111 into CW/CCW rotation (or invalid).
   The rotary "position" is held in rotary_counter, increasing for CW rotation, decreasing 
   for CCW rotation. If the position changes, rotary_change will be set true. 
   You should set this to false after handling the change.
   To implement, attachInterrupts to encoder pin A *and* pin B 
*/

void rotaryISR()
{
  const int states[] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};
  static byte transition = 0;                     // holds current and previous encoder states   
  transition <<= 2;                               // shift previous state up 2 bits
  transition |= (digitalRead(ENCODER_A));         // put encoder_A on bit 0
  transition |= (digitalRead(ENCODER_B) << 1);    // put encoder_B on bit 1
  transition &= 0x0F;                             // zero upper 4 bits
  rotaryCounter += states[transition];            // update counter +/- 1 based on rotation
}

boolean buttonDown()                              // check CURRENT state of button
{
  return (digitalRead(ENCODER_BUTTON)==LOW);
}

void waitForButtonRelease()
{
  if (buttonDown())                               // make sure button is currently pressed.
  {
    while (!button_released) ;                    // wait for release
    button_released = false;                      // and reset flag
  }
}

void waitForButtonPress()
{
  if (!buttonDown())                              // make sure button is not pressed.
  {
    while (!button_pressed) ;                     // wait for press
    button_pressed = false;                       // and reset flag
  }  
}

/* 
   readEncoder() returns 0 if no significant encoder movement since last call,
   +1 if clockwise rotation, and -1 for counter-clockwise rotation
*/

int readEncoder(int numTicks = ENCODER_TICKS) 
{
  static int prevCounter = 0;                     // holds last encoder position
  int change = rotaryCounter - prevCounter;       // how many ticks since last call?
  if (abs(change)<=numTicks)                      // not enough ticks?
    return 0;                                     // so exit with a 0.
  prevCounter = rotaryCounter;                    // enough clicks, so save current counter values
  return (change>0) ? 1:-1;                       // return +1 for CW rotation, -1 for CCW   
}


//===================================  Morse Routines ===================================

bool ditPressed()
{
  return (digitalRead(ditPaddle)==0);             // pin is active low
}

bool dahPressed()
{
  return (digitalRead(dahPaddle)==0);             // pin is active low
}

int extracharDit()
{
  return (3158/codeSpeed) - (1958/charSpeed); 
}

int intracharDit()
{
  return (1200/charSpeed);
}

void ditSpaces(int spaces=1) {                    // user specifies #dits to wait
  for (int i=0; i<spaces; i++)                    // count the dits...
    delay(ditPeriod);                             // no action, just mark time
}

void characterSpace()
{
  int fudge = (charSpeed/codeSpeed)-1;            // number of fast dits needed to convert
  delay(fudge*ditPeriod);                         // single intrachar dit to slower extrachar dit
  delay(2*extracharDit());                        // 3 total (last element includes 1)
  //ditSpaces(2);                                 
}

void wordSpace()
{
  delay(4*extracharDit());                        // 7 total (last char includes 3)
  //ditSpaces(4);                                 
}

void dit() {
  digitalWrite(LED,1);                            // CHG turn on LED
  tone(PIEZO,pitch);                              // and turn on sound
  ditSpaces();
  digitalWrite(LED,0);                            // CHG turn off LED
  noTone(PIEZO);                                  // and turn off sound
  ditSpaces();                                    // space between code elements
}

void dah() {
  digitalWrite(LED,1);                            // CHG turn on LED
  tone(PIEZO,pitch);                              // and turn on sound
  ditSpaces(3);                                   // length of dah = 3 dits
  digitalWrite(LED,0);                            // CHG turn off LED
  noTone(PIEZO);                                  // and turn off sound
  ditSpaces();                                    // space between code elements
}

void sendElements(int x) {                        // send string of bits as Morse      
  while (x>1) {                                   // stop when value is 1 (or less)
    if (x & 1) dit();                             // right-most bit is 1, so dit
    else dah();                                   // right-most bit is 0, so dah
    x >>= 1;                                      // rotate bits right
  }
  characterSpace();                               // add inter-character spacing
}

void roger() {
  dit(); dah(); dit();                            // audible (only) confirmation
}

/* 
   The following routine accepts a numberic input, where each bit represents a code element:
   1=dit and 0=dah.   The elements are read right-to-left.  The left-most bit is a stop bit.
   For example:  5 = binary 0b0101.  The right-most bit is 1 so the first element is a dit.
   The next element to the left is a 0, so a dah follows.  Only a 1 remains after that, so
   the character is complete: dit-dah = morse character 'A'. 
*/

void sendCharacter(char c) {                      // send a single ASCII character in Morse
  if (button_pressed) return;                     // user wants to quit, so vamoose
  if (c<32) return;                               // ignore control characters
  if (c>96) c -= 32;                              // convert lower case to upper case
  if (c>90) return;                               // not a character
  addCharacter(c);                                // display character on LCD
  if (c==32) wordSpace();                         // space between words 
  else sendElements(morse[c-33]);                 // send the character
  checkForSpeedChange();                          // allow change in speed while sending
  do 
    checkPause(); 
  while (paused);                                 // allow user to pause morse output
}

void sendString (char *ptr) {             
  while (*ptr)                                    // send the entire string
    sendCharacter(*ptr++);                        // one character at a time
}

void displayWPM ()
{
  const int x=290,y=200;
  tft.fillRect(x,y,24,20,BLACK);
  tft.setCursor(x,y);
  tft.print(codeSpeed); 
}

void checkForSpeedChange()
{
  int dir = readEncoder();
  if (dir!=0)
  {
    bool farnsworth=(codeSpeed!=charSpeed);
    codeSpeed += dir;
    if (codeSpeed<MINSPEED) 
      codeSpeed=MINSPEED;
    if (farnsworth)
    {
      if (codeSpeed>=charSpeed) 
        codeSpeed=charSpeed-1;
    } else {
      if (codeSpeed>MAXSPEED) 
        codeSpeed=MAXSPEED;
      charSpeed = codeSpeed;      
    }
    displayWPM();
    ditPeriod = intracharDit();
  }  
}

void checkPause()
{
  if (dahPressed())                               // did user press "dah"?
    paused = true;                                // yes, so pause output
  if (ditPressed() || button_pressed)             // did user press "dit"?
    paused = false;                               // yes, so resume output
}


//===================================  Koch Method  =====================================

void setTopMenu(char *str)                        // erase menu & replace with new text
{
  tft.fillRect(0,0,DISPLAYWIDTH,ROWSPACING,BLACK); 
  showMenuItem(str,0,0,FG,BG);                    
}

void sendKochLesson(int lesson)                   // send letter/number groups...
{ 
  const int maxCount = 175;                       // full screen = 20 x 9
  int charCount = 0;
  eraseMenus();                                   // start with empty screen
  while (!button_pressed && 
  (charCount < maxCount))                         // full screen = 1 lesson 
  {                            
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
    {
      int c = koch[random(lesson+1)];             // pick a random character
      sendCharacter(c);                           // and send it
      charCount ++;                               // keep track of #chars sent
    }
    sendCharacter(' ');                           // send a space between words
  }
}

void introLesson(int lesson)                      // helper fn for getLessonNumber()
{
  eraseMenus();                                   // start with clean screen
  tft.print("You are in lesson ");
  tft.println(lesson);                            // show lesson number
  tft.println("\nCharacters: ");                  
  tft.setTextColor(CYAN);
  for (int i=0; i<=lesson; i++)                   // show characters in this lession
  {
    tft.print(koch[i]);
    tft.print(" ");
  }
  tft.setTextColor(TEXTCOLOR);
  tft.println("\n\nPress <dit> to begin");  
}

int getLessonNumber()
{
  int lesson = kochLevel;                         // start at current level
  introLesson(lesson);                            // display lesson number
  while (!button_pressed && !ditPressed())
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      lesson += dir;                              // ...so change speed up/down 
      if (lesson<1) lesson = 1;                   // dont go below 1 
      if (lesson>kochLevel) lesson = kochLevel;   // dont go above maximum
      introLesson(lesson);                        // show new lesson number
    }
  }
  return lesson;
}

void sendKoch()
{
  while (!button_pressed) {
    setTopMenu("Koch lesson");
    int lesson = getLessonNumber();               // allow user to select lesson
    if (button_pressed) return;                   // user quit, so sad
    sendKochLesson(lesson);                       // do the lesson                      
    setTopMenu("Get 90%? Dit=YES, Dah=NO");       // ask user to score lesson
    while (!button_pressed) {                     // wait for user response
       if (ditPressed())                          // dit = user advances to next level
       {  
         roger();                                 // acknowledge success
         if (kochLevel<ELEMENTS(koch))        
           kochLevel++;                           // advance to next level
         saveConfig();                            // save it in EEPROM
         delay(1000);                             // give time for user to release dit
         break;                                   // go to next lesson
       }
       if (dahPressed()) break;                   // dah = repeat same lesson
    }
  }
}


//===================================  Receive Menu  ====================================


void addChar (char* str, char ch)                 // adds 1 character to end of string
{                                            
  char c[2] = " ";                                // happy hacking: char into string
  c[0] = ch;                                      // change char 'A' to string "A"
  strcat(str,c);                                  // and add it to end of string
}

char randomLetter()                               // returns a random uppercase letter
{
  return 'A'+random(0,26);
}

char randomNumber()                               // returns a random single-digit # 0-9
{
  return '0'+random(0,10);
}

void randomCallsign(char* call)                   // returns with random US callsign in "call"
{  
  strcpy(call,"");                                // start with empty callsign                      
  int i = random(0, 4);                           // 4 possible start letters for US                                       
  char c = prefix[i];                             // Get first letter of prefix
  addChar(call,c);                                // and add it to callsign
  i = random(0,3);                                // single or double-letter prefix?
  if ((i==2) or (c=='A'))                         // do a double-letter prefix
  {                                               // allowed combinations are:
    if (c=='A') i=random(0,12);                   // AA-AL, or
    else i=random(0,26);                          // KA-KZ, NA-NZ, WA-WZ 
    addChar(call,'A'+i);                          // add second char to prefix                                                         
  }
  addChar(call,randomNumber());                   // add zone number to callsign
  for (int i=0; i<random(1, 4); i++)              // Suffix contains 1-3 letters
    addChar(call,randomLetter());                 // add suffix letter(s) to call
}

void randomRST(char* rst)
{
  strcpy(rst,"");                                 // start with empty string                     
  addChar(rst,'0'+random(3,6));                   // readability 3-5                      
  addChar(rst,'0'+random(5,10));                  // strength: 6-9 
  addChar(rst,'9');                               // tone usually 9
}

void sendNumbers()                                
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
      sendCharacter(randomNumber());              // send a number
    sendCharacter(' ');                           // send a space between number groups
  }
}

void sendLetters()                                
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)                // group letters into "words"
      sendCharacter(randomLetter());              // send the letter 
    sendCharacter(' ');                           // send a space between words
  }
}

void sendMixedChars()                             // send letter/number groups...
{ 
  while (!button_pressed) {                            
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
    {
      int c = '0' + random(43);                   // pick a random character
      sendCharacter(c);                           // and send it
    }
    sendCharacter(' ');                           // send a space between words
  }
}

void sendPunctuation()
{
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
    {
      int j=random(0,ELEMENTS(punctuation));      // select a punctuation char
      sendCharacter(punctuation[j]);              // and send it.
    }
    sendCharacter(' ');                           // send a space between words
  }
}


void sendWords()
{ 
  while (!button_pressed) {
    int index=random(0, ELEMENTS(words));         // eeny, meany, miney, moe
    sendString(words[index]);                     // send the word
    sendCharacter(' ');                           // and a space between words
  }
}

void sendCallsigns()                              // send random US callsigns
{ 
  char call[8];                                   // need string to stuff callsign into
  while (!button_pressed) {
    randomCallsign(call);                         // make it
    sendString(call);                             // and send it
    sendCharacter(' ');                           // with a space between callsigns         
  }
}

void sendQSO()
{
  char otherCall[8];
  char temp[20];                              
  char qso[300];                                  // string to hold entire QSO
  randomCallsign(otherCall);
  strcpy(qso,MYCALL);                             // start of QSO
  strcat(qso," de ");
  strcat(qso,otherCall);                          // another ham is calling you
  strcat(qso," K  TNX FER CALL= UR RST ");
  randomRST(temp);                                // add RST
  strcat(qso,temp);
  strcat(qso," ");
  strcat(qso,temp);                               
  strcat(qso,"= NAME HERE IS ");
  strcpy(temp,names[random(0,ELEMENTS(names))]); 
  strcat(qso,temp);                               // add name
  strcat(qso," ? ");
  strcat(qso,temp);                               // add name again
  strcat(qso,"= QTH IS ");
  strcpy(temp,cities[random(0,ELEMENTS(cities))]);
  strcat(qso,temp);                               // add QTH
  strcat(qso,"= RIG HR IS ");
  strcpy(temp,rigs[random(0,ELEMENTS(rigs))]);    // add rig
  strcat(qso,temp);                               
  strcat(qso," ES ANT IS ");                      // add antenna
  strcat(qso,antenna[random(0,ELEMENTS(antenna))]);             
  strcat(qso,"== WX HERE IS ");                   // add weather
  strcat(qso,weather[random(0,ELEMENTS(weather))]);     
  strcat(qso,"= SO HW CPY? ");                    // back to other ham
  strcat(qso,MYCALL);
  strcat(qso," de ");
  strcat(qso,otherCall);
  strcat(qso," KN");
  sendString(qso);                                // send entire QSO
}

int getFileList  (char list[][13])                // gets list of files on SD card
{
  File root = SD.open("/");                       // open root directory on the SD card
  int count=0;                                    // count the number of files
  while (count < MAXROW)                          // only room for so many on our small screen!
  {
    File entry = root.openNextFile();             // get next file in the SD root directory
    if (!entry) break;                            // leave if there aren't any more
    if (!entry.isDirectory())                     // ignore directory names
      strcpy(list[count++],entry.name());         // add name of SD file to the list
    entry.close();                                // close the file
  }
  root.close(); 
  return count;   
}

int fileMenu(char menu[][13], int itemCount)      // Display list of files & get user selection
{
  int index=0, x,y; 
  button_pressed = false;                         // reset button flag
  x = 30;                                         // x-coordinate of this menu
  for (int i = 0; i < itemCount; i++)             // for all items in the menu...
  {
     y = TOPDEADSPACE + i*ROWSPACING;             // calculate y coordinate
     showMenuItem(menu[i],x,y,FG,BG);             // and show the item.
  }
  showMenuItem(menu[index],x,TOPDEADSPACE,        // highlight selected item
    SELECTFG,SELECTBG);
  while (!button_pressed)                         // exit on button press
  {
    int dir = readEncoder();                      // check for encoder movement
    if (dir)                                      // it moved!    
    {
      y = TOPDEADSPACE + index*ROWSPACING;        // calc y-coord of current item
      showMenuItem(menu[index],x,y,FG,BG);        // deselect current item
      index += dir;                               // go to next/prev item
      if (index > itemCount-1) index=0;           // dont go past last item
      if (index < 0) index = itemCount-1;         // dont go before first item
       y = TOPDEADSPACE + index*ROWSPACING;       // calc y-coord of new item
      showMenuItem(menu[index],x,y,
      SELECTFG,SELECTBG);                         // select new item
    }
  }
  return index;  
}

void sendFile(char* filename)                     // output a file to screen & morse
{
  const int pageSkip = 250;
  eraseMenus();                                   // clear screen below menu
  button_pressed = false;                         // reset flag for new presses
  File book = SD.open(filename);                  // look for book on sd card
  if (book) {                                     // find it? 
    while (!button_pressed && book.available())   // do for all characters in book:
    {                                    
      char ch = book.read();                      // get next character
      sendCharacter(ch);                          // and send it
      if (ch=='\r') sendCharacter(' ');           // treat CR as a space
      if (ditPressed() && dahPressed())           // user wants to 'skip' ahead:
      {
        sendString("=  ");                        // acknowledge the skip with ~BT
        for (int i=0; i<pageSkip; i++)
          book.read();                            // skip a bunch of text!
      }
    }
    book.close();                                 // close the file
  } 
}

void sendFromSD()                                 // show files on SD card, get user selection & send it.
{
  char list[MAXROW][13];                          // hold list of SD filenames (DOS 8.3 format, 13 char)
  int count = getFileList(list);                  // get list of files on the SD card
  int choice = fileMenu(list,count);              // display list & let user choose one
  sendFile(list[choice]);                         // output text & morse until user quits
}


//===================================  Send Menu  =======================================

void checkSpeed()
{
  long start=millis();
  sendString("PARIS ");                           // send "PARIS"                                     
  long elapsed=millis()-start;                    // see how long it took
  dit();                                          // sound out the end.
  float wpm = 60000.0/elapsed;                    // convert time to WPM
  tft.setTextSize(3);
  tft.setCursor(100,80);
  tft.print(wpm);                                 // display result
  tft.print(" WPM");
  while (!button_pressed) ;                       // wait for user
}

int decode(int code)                              // convert code to morse table index
{
  int i=0;                                        
  while (i<ELEMENTS(morse) && (morse[i]!=code))   // search table for the code
    i++;                                          // ...from beginning to end. 
  if (i<ELEMENTS(morse)) return i;                // found the code, so return index
  else return -1;                                 // didn't find the code, return -1
}

char receivedChar()                               // monitor paddles & return a decoded char                          
{
  int bit = 0;
  int code = 0;
  unsigned long start = millis();
  while (!button_pressed)
  {
    if (ditPressed())                             // user pressed dit paddle:
    {
      dit();                                      // so sound it out
      code += (1<<bit++);                         // add a '1' element to code
      start = millis();                           // and reset timeout.
    }
    if (dahPressed())                             // user pressed dah paddle:
    {
      dah();                                      // so sound it tout
      bit++;                                      // add '0' element to code
      start = millis();                           // and reset the timeout.
    }
    if (bit && (millis()-start > ditPeriod))      // waited for more than a dit
    {                                             // so that must be end of character:
      code += (1<<bit);                           // add stop bit
      int result = decode(code);                  // look up code in morse array
      if (result<0) return ' ';                   // oops, didn't find it
      else return '!'+result;                     // found it! return result
    }
    if (millis()-start > 5*ditPeriod)             // long pause = word space  
      return ' ';                                 
  }
  return ' ';                                     // return on button press
}

void receiveCode()                                // get Morse from user & display it
{
  char oldCh = ' ';                               
  while (!button_pressed) 
  {
    char ch = receivedChar();                     // get a morse character from user
    if (!((ch==' ') && (oldCh==' ')))             // only 1 word space at a time.
      addCharacter(ch);                           // show character on display
    oldCh = ch;                                   // and remember it 
  }
}

void copyCallsigns()                              // show a callsign & see if user can copy it
{
  char call[8];
  while (!button_pressed)                      
  {  
    randomCallsign(call);                         // make a random callsign       
    mimick(call);
  }
}

void copyCharacters()
{
  char text[8]; 
  while (!button_pressed)                      
  {
    strcpy(text,"");                              // start with empty string  
    addChar(text,randomLetter());                 // add a random letter
    mimick(text);                                 // compare it to user input
  }
}

void copyTwoChars()
{
  char text[8]; 
  while (!button_pressed)                      
  {
    strcpy(text,"");                              // start with empty string  
    addChar(text,randomLetter());                 // add a random letter
    addChar(text,randomLetter());                 // make that two random letters
    mimick(text);                                 // compare it to user input
  }
}

void copyWords()                                  // show a callsign & see if user can copy it
{
  char text[10];
  while (!button_pressed)                      
  { 
    int index=random(0, ELEMENTS(words));         // eeny, meany, miney, moe
    strcpy(text,words[index]);                    // pick a random word       
    mimick(text);                                 // and ask user to copy it
  }
}

void showScore(int score)                         // helper fn for mimick()
{
  const int x=200,y=50,wd=105,ht=80;              // posn & size of scorecard
  int bkColor = (score>0)?GREEN:RED;              // back-color green unless score 0
  tft.setCursor(x+15,y+20);                       // position text within scorecard
  tft.setTextSize(6);                             // use big text,
  tft.setTextColor(BLACK,bkColor);                // in inverted font,
  tft.fillRect(x,y,wd,ht,bkColor);                // on selected background
  tft.print(score);                               // show the score
  tft.setTextSize(2);                             // resume usual size
  tft.setTextColor(TEXTCOLOR,BLACK);              // resume usual colors  
}

void mimick(char *text)
{
  char ch, response[20]; 
  static int score=0;                              
  textRow=1; textCol=10;                          // set position of text 
  sendString(text);                               // display text & morse it
  strcpy(response,"");                            // start with empty response
  textRow=2; textCol=10;                          // set position of response
  while (!button_pressed && !ditPressed()         // wait until user is ready
    && !dahPressed()) ;
  do {                                            // user has started keying...
    ch = receivedChar();                          // get a character
    if (ch!=' ') addChar(response,ch);            // add it to the response
    addCharacter(ch);                             // and put it on screen
  } while (ch!=' ');                              // space = word timeout
  if (!strcmp(text,response))                     // did user match the text?
    score++;                                      // yes, so increment score
  else score = 0;                                 // no, so reset score to 0
  showScore(score);                               // display score for user
  delay(FLASHCARDDELAY);                          // wait between attempts
  eraseMenus();                                   // erase screen for next attempt
}

void flashcards()
{
  tft.setTextSize(7);
  while (!button_pressed)
  {
     int index = random(0,ELEMENTS(morse));       // get a random character
     int code = morse[index];                     // convert to morse code
     if (!code) continue;                         // only do valid morse chars
     sendElements(code);                          // sound it out
     delay(1000);                                 // wait for user to guess
     tft.setCursor(120,70);
     tft.print(char('!'+index));                  // show the answer
     delay(FLASHCARDDELAY);                       // wait a little
     eraseMenus();                                // and start over.
  }
}

void twoWay()                                     // wireless QSO between units
{
}



//===================================  Config Menu  =====================================

void saveConfig()
{
  EEPROM.update(0,42);                            // the answer to everything
  EEPROM.update(1,charSpeed);                     // save the character speed in wpm
  EEPROM.update(2,codeSpeed);                     // save overall code speed in wpm
  EEPROM.update(3,pitch/10);                      // save pitch as 1/10 of value
  EEPROM.update(4,ditPaddle);                     // save pin corresponding to 'dit'
  EEPROM.write(5,kochLevel);                      // save current Koch lesson #
}

void loadConfig()
{
  int flag = EEPROM.read(0);                      // saved values been saved before?
  if (flag==42)                                   // yes, so load saved parameters
  {
     charSpeed  = EEPROM.read(1);
     codeSpeed  = EEPROM.read(2);
     pitch      = EEPROM.read(3)*10;
     ditPaddle  = EEPROM.read(4); 
     kochLevel  = EEPROM.read(5); 
  } 
}

void useDefaults()                                // if things get messed up...
{
  charSpeed = DEFAULTSPEED;
  codeSpeed = DEFAULTSPEED;
  pitch     = DEFAULTPITCH;
  ditPaddle = PADDLE_A;
  dahPaddle = PADDLE_B;
  kochLevel = 1;
  saveConfig();
  roger();
}

void setCodeSpeed()
{
  const int x=160,y=80;                           // screen posn for speed display
  tft.print("Overall Code Speed (WPM)");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(codeSpeed);                           // display current speed
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      codeSpeed += dir;                           // ...so change speed up/down          
      if (codeSpeed<MINSPEED) 
		    codeSpeed = MINSPEED;                     // dont go below minimum
      if (codeSpeed>MAXSPEED) 
		    codeSpeed = MAXSPEED;                     // dont go above maximum
      tft.fillRect(x,y,50,50,BLACK);              // erase old speed
      tft.setCursor(x,y);
      tft.print(codeSpeed);                       // and show new speed    
    }
  }
  if (codeSpeed>charSpeed)
    charSpeed = codeSpeed;                        // keep charSpeed >= codeSpeed                 
  ditPeriod = intracharDit();                     // adjust dit length
  saveConfig();                                   // save the new speed
  roger();                                        // and acknowledge
}

void setCharSpeed()
{
  const int x=160,y=80;                           // screen posn for speed display
  tft.print("Character Code Speed (WPM)");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(charSpeed);                           // display current speed
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      charSpeed += dir;                           // ...so change speed up/down 
      if (charSpeed<MINSPEED) 
		    charSpeed = MINSPEED;                     // dont go below minimum
      if (charSpeed>MAXSPEED) 
		    charSpeed = MAXSPEED;                     // dont go above maximum
      tft.fillRect(x,y,50,50,BLACK);              // erase old speed
      tft.setCursor(x,y);
      tft.print(charSpeed);                       // and show new speed 
    }
  }
  if (charSpeed < codeSpeed)
    codeSpeed = charSpeed;                        // keep codeSpeed <= charSpeed
  ditPeriod = intracharDit();                     // adjust dit length
  saveConfig();                                   // save the new speed
  roger();                                        // and acknowledge
}

void setPitch()
{
  const int x=120,y=80;                           // screen posn for pitch display
  tft.print("Tone Frequency (Hz)");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(pitch);                               // show current pitch                 
  while (!button_pressed)
  {
    int dir = readEncoder(2);
    if (dir!=0)                                   // user rotated encoder knob
    {
      pitch += dir*50;                            // so change pitch up/down, 50Hz increments
      if (pitch<MINPITCH) pitch = MINPITCH;       // dont go below minimum
      if (pitch>MAXPITCH) pitch = MAXPITCH;       // dont go above maximum
      tft.fillRect(x,y,100,40,BLACK);             // erase old value
      tft.setCursor(x,y);
      tft.print(pitch);                           // and display new value
      dit();                                      // let user hear new pitch
    }
  }
  saveConfig();                                   // save the new pitch 
  roger();                                        // and acknowledge
}

void setDitPaddle()
{
  tft.setCursor(0,60);
  tft.print("Press Dit Paddle NOW");
  while (!button_pressed && 
    !ditPressed() && !dahPressed()) ;             // wait for user input
  ditPaddle = !digitalRead(PADDLE_A)? 
    PADDLE_A: PADDLE_B;                           // dit = whatever pin went low.
  dahPaddle = !digitalRead(PADDLE_A)? 
    PADDLE_B: PADDLE_A;                           // dah = opposite of dit.
  saveConfig();                                   // save it
  roger();                                        // and acknowledge
}

//===================================  Menu Routines ====================================

void showCharacter(char c, int row, int col)      // display a character at given row & column
{
  int x = col * COLSPACING;                       // convert column to x coordinate
  int y = TOPDEADSPACE + (row * ROWSPACING);      // convert row to y coordinate
  tft.setCursor(x,y);                             // position character on screen
  tft.print(c);                                   // and display it 
}

void addCharacter(char c)
{
  showCharacter(c,textRow,textCol);               // display character & current row/col position
  textCol++;                                      // go to next position on the current row
  if ((textCol>=MAXCOL) ||                        // are we at end of the row?
     ((c==' ') && (textCol>MAXCOL-7)))            // or at a wordspace thats near end of row?
  {
    textRow++; textCol=0;                         // yes, so advance to beginning of next row
    if (textRow >= MAXROW) eraseMenus();          // if no more rows, clear & start at top.
  }
}

void eraseMenus()                                 // clear the text portion of the display
{
  tft.fillRect(0, TOPDEADSPACE, DISPLAYWIDTH, DISPLAYHEIGHT, BLACK);
  tft.setTextColor(TEXTCOLOR,BLACK);
  tft.setCursor(0,TOPDEADSPACE);
  textRow=0; textCol=0;                           // start text below the top menu
}

int getMenuSelection()                            // Display menu system & get user selection
{
  int item;
  eraseMenus();                                   // start with fresh screen
  menuCol = topMenu(mainMenu,ELEMENTS(mainMenu)); // show horiz menu & get user choice
  switch (menuCol)                                // now show menu that user selected:
  {
   case 0: 
     item = subMenu(menu0,ELEMENTS(menu0));       // "receive" dropdown menu
     break;
   case 1: 
     item = subMenu(menu1,ELEMENTS(menu1));       // "send" dropdown menu
     break;
   case 2: 
     item = subMenu(menu2,ELEMENTS(menu2));       // "config" dropdown menu
  }
  return (menuCol*10 + item);                     // return user's selection
}

void showMenuItem(char *item, int x, int y, int fgColor, int bgColor)
{
  tft.setCursor(x,y);
  tft.setTextColor(fgColor, bgColor);
  tft.print(item);  
}

int topMenu(char *menu[], int itemCount)          // Display a horiz menu & return user selection
{
  int index = menuCol;                            // start w/ current row
  tft.setTextSize(2);                             // sets menu text size
  button_pressed = false;                         // reset button flag

  for (int i = 0; i < itemCount; i++)             // for each item in menu                         
    showMenuItem(menu[i],i*MENUSPACING,0,FG,BG);  // display it 

  showMenuItem(menu[index],index*MENUSPACING,     // highlight current item
    0,SELECTFG,SELECTBG);
  tft.drawLine(0,TOPDEADSPACE-4,DISPLAYWIDTH,
    TOPDEADSPACE-4, YELLOW);                      // horiz. line below menu

  while (!button_pressed)                         // loop for user input:
  {
    int dir = readEncoder();                      // check encoder
    if (dir) {                                    // did it move?
      showMenuItem(menu[index],index*MENUSPACING,
	    0, FG,BG);                                  // deselect current item
      index += dir;                               // go to next/prev item
      if (index > itemCount-1) index=0;           // dont go beyond last item
      if (index < 0) index = itemCount-1;         // dont go before first item
      showMenuItem(menu[index],index*MENUSPACING,
	    0, SELECTFG,SELECTBG);                      // select new item         
    }  
  }
  return index;  
}

int subMenu(char *menu[], int itemCount)          // Display drop-down menu & return user selection
{
  int index=0, x,y; 
  button_pressed = false;                         // reset button flag

  x = menuCol * MENUSPACING;                      // x-coordinate of this menu
  for (int i = 0; i < itemCount; i++)             // for all items in the menu...
  {
     y = TOPDEADSPACE + i*ROWSPACING;             // calculate y coordinate
     showMenuItem(menu[i],x,y,FG,BG);             // and show the item.
  }
  showMenuItem(menu[index],x,TOPDEADSPACE,        // highlight selected item
    SELECTFG,SELECTBG);

  while (!button_pressed)                         // exit on button press
  {
    int dir = readEncoder();                      // check for encoder movement
    if (dir)                                      // it moved!    
    {
      y = TOPDEADSPACE + index*ROWSPACING;        // calc y-coord of current item
      showMenuItem(menu[index],x,y,FG,BG);        // deselect current item
      index += dir;                               // go to next/prev item
      if (index > itemCount-1) index=0;           // dont go past last item
      if (index < 0) index = itemCount-1;         // dont go before first item
       y = TOPDEADSPACE + index*ROWSPACING;       // calc y-coord of new item
      showMenuItem(menu[index],x,y,
	    SELECTFG,SELECTBG);                         // select new item
    }
  }
  return index;  
}


//================================  Main Program Code ================================

void initEncoder()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON),buttonISR,CHANGE);   
  attachInterrupt(digitalPinToInterrupt(ENCODER_A),rotaryISR,CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_B),rotaryISR,CHANGE); 
}

void initMorse()
{
  pinMode(LED,OUTPUT);                            // LED, but could be keyer output instead
  pinMode(PADDLE_A, INPUT_PULLUP);                // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);
  ditPeriod = intracharDit();                     // set up character timing from WPM 
  dahPaddle = (ditPaddle==PADDLE_A)?              // make dahPaddle opposite of dit
     PADDLE_B: PADDLE_A;
  roger();                                        // acknowledge morse startup with an 'R'                
}

void initScreen()
{
    uint16_t ID;
  ID = tft.readID();
  tft.begin (ID);
  //tft.setRotation(1);
  tft.setRotation(SCREEN_ROTATION);               // landscape mode: use '1' or '3'
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
}

void splashScreen()                               // not splashy at all!
{                                                 // have fun sprucing it up.
  tft.setTextSize(3);
  tft.setTextColor(TEXTCOLOR);
  tft.setCursor(100, 50); 
  tft.print(MYCALL);                              // add your callsign (set MYCALL at top of sketch)
  tft.setTextColor(CYAN);
  tft.setCursor(15, 90);      
  tft.print("Morse Code Tutor");                  // add title
  tft.setTextSize(1);
  tft.setCursor(50,220);
  tft.setTextColor(WHITE);
  tft.print("Copyright (c) 2019, Bruce E. Hall"); // legal small print
  delay(2000);                                    // keep it on screen for a while
  tft.fillScreen(BLACK);                          // then erase it.
}

void setup()   
{ 
  pinMode(A8, OUTPUT);
  digitalWrite(A8, LOW);                          // 0V for wire to PIEZO 
  pinMode(A11, OUTPUT);
  digitalWrite(A11, LOW);                         // 0V for wire to Morse Pads   
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);                          // 0V for wire to ENCODER and ENCODER_BUTTON
  pinMode(11, INPUT);                             // Set to HiZ. hard wired on MEGA rear to 51 MOSI
  pinMode(12, INPUT);                             // Set to HiZ, hard wired on MEGA rear to 50 MISO
  pinMode(13, INPUT);                             // Set to HiZ, hard wired on MEGA rear to 52 SCK 
  SD.begin(SD_CS);                                // initialize SD library
  loadConfig();                                   // get saved values from EEPROM
  initEncoder();                                  // attach encoder interrupts
  initScreen();                                   // blank screen in landscape mode
  initMorse();                                    // attach paddles & adjust speed
  splashScreen();                                 // show we are ready
}
void loop()
{
  int selection = getMenuSelection();             // get menu selection from user
  eraseMenus();                                   // clear screen below menu
  button_pressed = false;                         // reset flag for new presses
  randomSeed(millis());                           // randomize!
  switch(selection)                               // do action requested by user
  {
    case 00: sendKoch(); break;
    case 01: sendLetters(); break;
    case 02: sendWords(); break;
    case 03: sendNumbers(); break;
    case 04: sendMixedChars(); break;
    case 05: sendFromSD(); break;
    case 06: sendQSO(); break;
    case 07: sendCallsigns(); break;

    case 10: receiveCode(); break;
    case 11: copyCharacters(); break;
    case 12: copyTwoChars(); break;
    case 13: copyWords(); break;
    case 14: copyCallsigns(); break;
    case 15: flashcards(); break;
    case 16: twoWay(); break;
    
    case 20: setCodeSpeed(); break;
    case 21: setCharSpeed(); break;
    case 22: checkSpeed(); break;
    case 23: setPitch(); break;
    case 24: setDitPaddle(); break;
    case 25: useDefaults(); break;
    default: ;
  }
}
