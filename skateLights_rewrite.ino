#include <Adafruit_NeoPixel.h>
#include <TimerOne.h>

// LED strip data pin
#define PIN 5 
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
#define NUMMODES 3
#define NUMCOLORS 12

typedef void (*colorModes)(uint32_t, uint8_t);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN, NEO_GRB + NEO_KHZ800);

const uint32_t COLORS[13] = {strip.Color(0, 0, 0), strip.Color(255, 0, 0), strip.Color(255, 0, 255), 
                             strip.Color(128, 0, 255), strip.Color(0, 0, 255), strip.Color(0, 128, 255), 
                             strip.Color(0, 255, 255), strip.Color(0, 255, 128), strip.Color(0, 255, 128), 
                             strip.Color(0, 255, 0), strip.Color(128, 255, 0), strip.Color(255, 255, 0), 
                             strip.Color(255, 128, 0)};
                             // BLACK, RED, MAGENTA, PURPLE, BLUE, CORNFLOWER, TURQUOISE, SEAFOAM, GREEN, LIME,
                             // YELLOW, ORANGE


colorModes modeArray[3] = {NULL, &rainbowCycle, &theaterChase}; 
uint8_t buttons[] = {A2, A3}; 

// we will track if a button is just pressed, just released, or 'currently pressed' 
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];



volatile boolean ini = false;              // mode reset trigger, 0 if strip is waiting for a new color/mode, 
                                           // 1 if running.
volatile uint8_t colorState = 0;           // current color palette mode                          
volatile uint8_t modeState = 0;            // current cycling pattern mode
boolean timerFlag = false;                 // false if timer based mode switching is off
uint8_t led = 13;                          // led pin to show timed switching mode enabled
uint8_t timerState = 0;                    // cycling mode while in timer based switching
uint32_t lastTime = millis();
uint8_t tmpColor = random(255);

void setup() {
   
  strip.begin();
  strip.setBrightness(200);
  strip.show(); // Initialize all pixels to 'off'
  
  // set up serial port
  Serial.begin(9600);
  pinMode(led, OUTPUT); 
  
  // Make input & enable pull-up resistors on switch pins
  for (byte i=0; i < NUMBUTTONS; i++) { 
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
  }
  
  #if defined (__AVR_ATmega328P__)  
    // Run timer2 interrupt every 15 ms 
    TCCR2A = 0;
    TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;
    //Timer2 Overflow Interrupt Enable
    TIMSK2 |= 1<<TOIE2; 
  #elif defined (__AVR_ATmega32U4__)  
    Timer1.initialize(15000);
    Timer1.attachInterrupt(TimerInterrupt);
  #endif
}

#if defined (__AVR_ATmega328P__)  
ISR(TIMER2_OVF_vect) {
  check_switches();
}
#elif defined (__AVR_ATmega32U4__)  

void TimerInterrupt() {
  check_switches();
}  
#endif  

  
void check_switches()
{
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];

  for (byte index = 0; index < NUMBUTTONS; index++){
    currentstate[index] = digitalRead(buttons[index]);   // read the button
    
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
          // just pressed
          justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
 
  // Color Increment Button
  if (justreleased[0]) {  
    if (modeState == 0) {
      modeState = 1; 
    }  
    if (NUMCOLORS > colorState) {
      colorState += 1;
    } else {
      colorState = 1;
    }  
    ini = false;  
    justreleased[0] = 0;
    Serial.print("colorState = ");
    Serial.println(colorState);
  }
   // Mode Increment Button
  if (justreleased[1]) {
    if (colorState == 0) {
      colorState = 1;
    }
    if (NUMMODES > modeState) {
      modeState += 1;
    } else {
      timerFlag = false;
      tmpColor = random(255);
      modeState = 1;
    }  
    justreleased[1] = 0;
    ini = false;
    Serial.print("modeState = ");
    Serial.println(modeState);
  } 
} 

void loop() {
  if (!ini && modeState) {
    ini = true;
    modeArray[modeState](COLORS[colorState], 75);
  }
  colorReset(COLORS[0], 0); 
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


void colorReset(uint32_t c, uint8_t wait) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, COLORS[0]);
      strip.show();
    }
}     

void rainbowCycle(uint32_t c, uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    if (0 == ini) {
      colorReset(COLORS[0], 0);
      break;
    } 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / 32) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int q=0; q < 3; q++) {
    for (int i=0; i < strip.numPixels(); i=i+3) {
      strip.setPixelColor(i+q, c);    //turn every third pixel on
    }
    strip.show();
   
    delay(wait);
   
    for (int i=0; i < strip.numPixels(); i=i+3) {
      strip.setPixelColor(i+q, 0);        //turn every third pixel off
    }
    if (0 == ini) {
      colorReset(COLORS[0], 0);
    break;
    }  
  }
}
