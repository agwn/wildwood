//Example to control RGB LED Modules with Spectrum Analyzer
//Bliptronics.com
//Ben Moyes 2010
//Use this as you wish, but please give credit, or at least buy some of my LEDs!
//

const int ledPin = 13; //On board LED

const int SDI = 9;
const int CKI = 8;

#define STRIP_LENGTH 14 //14 LEDs on this strip

//For spectrum analyzer shield, these three pins are used.
//You can move pinds 4 and 5, but you must cut the trace on the shield and re-route from the 2 jumpers. 
int spectrumReset=5;
int spectrumStrobe=4;
int spectrumAnalog=0;  //0 for left channel, 1 for right.

long strip_colors[STRIP_LENGTH];

#define PWM_MIN_VAL  0
#define PWM_MAX_VAL  255
#define PWM_DEF_VAL  ((int)((PWM_MAX_VAL+PWM_MIN_VAL)/2.0))

const float rOffset = PWM_DEF_VAL/(float)PWM_MAX_VAL;
const float gOffset = PWM_DEF_VAL/(float)PWM_MAX_VAL;
const float bOffset = PWM_DEF_VAL/(float)PWM_MAX_VAL;

int rValue = PWM_MAX_VAL;
int gValue = PWM_MAX_VAL;
int bValue = PWM_MAX_VAL;


// Spectrum analyzer read values will be kept here.
int spectrum[7];

void setup() {
  pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Serial.begin(57600);

  //Setup pins to drive the spectrum analyzer. 
  pinMode(spectrumReset, OUTPUT);
  pinMode(spectrumStrobe, OUTPUT);

  //Init spectrum analyzer
  digitalWrite(spectrumStrobe,LOW);
  delay(1);
  digitalWrite(spectrumReset,HIGH);
  delay(1);
  digitalWrite(spectrumStrobe,HIGH);
  delay(1);
  digitalWrite(spectrumStrobe,LOW);
  delay(1);
  digitalWrite(spectrumReset,LOW);
  delay(5);
  // Reading the analyzer now will read the lowest frequency.

  //Clear out the array
  for(int x = 0 ; x < STRIP_LENGTH ; x++) {
    strip_colors[x] = ((long)0xff)<<((x%3)*8);
  }
  post_frame();

  delay(1000);
}


void loop() {
  showSpectrum();
  delay(25);  //We wait here for a little while until all the values to the LEDs are written out.
  //This is being done in the background by an interrupt.
}


// Read 7 band equalizer.
void readSpectrum()
{
  // Band 0 = Lowest Frequencies.
  for(int Band=0; Band<7; Band++)
  {
    spectrum[Band] = (analogRead(spectrumAnalog) + analogRead(spectrumAnalog) ) >>1; //Read twice and take the average by dividing by 2

    digitalWrite(spectrumStrobe,HIGH);
    digitalWrite(spectrumStrobe,LOW);     
  }
  Serial.println();
}


void showSpectrum()
{
  float tm;

  readSpectrum();

  // do something useful here

  Serial.print("spectrum: ");
  for( int i=0; i<7; i++) {
    Serial.print( spectrum[i] );
    Serial.print (", ");

    tm  = (float) TWO_PI*(spectrum[i]/1024.0)-i/7*TWO_PI;
    //Serial.println(tm);

    bValue = (int)constrain((PWM_MAX_VAL*(sin(tm)/2+rOffset)), PWM_MIN_VAL, PWM_MAX_VAL);
    gValue = (int)constrain((PWM_MAX_VAL*(sin(tm+TWO_PI/3)/2+gOffset)), PWM_MIN_VAL, PWM_MAX_VAL);
    rValue = (int)constrain((PWM_MAX_VAL*(sin(tm+2*TWO_PI/3)/2+bOffset)), PWM_MIN_VAL, PWM_MAX_VAL);

    //Now form a new RGB color
    long new_color = 0;
    //  for(x = 0 ; x < 3 ; x++){
    //    new_color <<= 8;
    //    new_color |= random(0xFF); //Give me a number from 0 to 0xFF
    //    //new_color &= 0xFFFFF0; //Force the random number to just the upper brightness levels. It sort of works.
    //  }
    new_color |= rValue;
    new_color <<= 8; 
    new_color |= gValue;
    new_color <<= 8; 
    new_color |= bValue;

    strip_colors[2*i] = new_color;
    strip_colors[2*i+1] = new_color;
  }
  Serial.println();

  post_frame();

}


//Takes the current strip color array and pushes it out
void post_frame (void) {
  //Each LED requires 24 bits of data
  //MSB: R7, R6, R5..., G7, G6..., B7, B6... B0 
  //Once the 24 bits have been delivered, the IC immediately relays these bits to its neighbor
  //Pulling the clock low for 500us or more causes the IC to post the data.

  for(int LED_number = 0 ; LED_number < STRIP_LENGTH ; LED_number++) {
    long this_led_color = strip_colors[LED_number]; //24 bits of color data

    for(byte color_bit = 23 ; color_bit != 255 ; color_bit--) {
      //Feed color bit 23 first (red data MSB)
      digitalWrite(CKI, LOW); //Only change data when clock is low

      long mask = 1L << color_bit;
      //The 1'L' forces the 1 to start as a 32 bit number, otherwise it defaults to 16-bit.

      if(this_led_color & mask) { 
        digitalWrite(SDI, HIGH);
      } 
      else {
        digitalWrite(SDI, LOW);
      }

      digitalWrite(CKI, HIGH); //Data is latched when clock goes high
    }
  }

  //Pull clock low to put strip into reset/post mode
  digitalWrite(CKI, LOW);
  delayMicroseconds(500); //Wait for 500us to go into reset
}




