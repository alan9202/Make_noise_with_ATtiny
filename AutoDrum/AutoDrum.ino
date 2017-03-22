// AutoDrum
// Drum machine automatica, utilizza l'algoritmo euclideo per la generazione dei pattern ritmici
// Pangrus 2017
// Basato su 'AVR cheap bass drum synthesis' di Alex Allmont http://www.alexallmont.com/?p=178con con permesso dell'autore

int gateState;
int lastGateState;
int clock;
int playing;
int startPos;   
int incrementSpeed = 30;
int impactPos; 
int sinePos;   
int sample;   
int timer;
long tempo;
int lastClockState;
char pattern[16];
int pos=0;
byte numeroColpi;

PROGMEM prog_uchar sinusoide[256] = {
  128,129,131,134,137,140,143,146,149,152,155,158,161,164,167,170,173,176,179,182,185,187,190,
  193,195,198,201,203,206,208,210,213,215,217,219,222,224,226,228,230,231,233,235,236,238,240,
  241,242,244,245,246,247,248,249,250,251,251,252,253,253,254,254,254,254,254,255,254,254,254,
  254,254,253,253,252,251,251,250,249,248,247,246,245,244,242,241,240,238,236,235,233,231,230,
  228,226,224,222,219,217,215,213,210,208,206,203,201,198,195,193,190,187,185,182,179,176,173,
  170,167,164,161,158,155,152,149,146,143,140,137,134,131,128,125,122,119,116,113,110,107,104,
  101,98,95,92,89,86,83,80,77,74,71,69,66,63,61,58,55,53,50,48,46,43,41,39,37,34,32,30,28,26,
  25,23,21,20,18,16,15,14,12,11,10,9,8,7,6,5,5,4,3,3,2,2,2,2,2,1,2,2,2,2,2,3,3,4,5,5,6,7,8,9,
  10,11,12,14,15,16,18,20,21,23,25,26,28,30,32,34,37,39,41,43,46,48,50,53,55,58,61,63,66,69,
  71,74,77,80,83,86,89,92,95,98,101,104,107,111,114,119,123,127};

PROGMEM prog_uchar inviluppo[256]  = {
  255, 253, 250, 248, 245, 243, 240, 238, 235, 233, 230, 228, 226, 223, 221, 219,
  216, 214, 212, 210, 208, 205, 203, 201, 199, 197, 195, 193, 191, 189, 187, 185,
  183, 181, 179, 178, 176, 174, 172, 170, 168, 167, 165, 163, 161, 160, 158, 156,
  155, 153, 152, 150, 148, 147, 145, 144, 142, 141, 139, 138, 136, 135, 133, 132,
  130, 129, 128, 126, 125, 123, 122, 121, 119, 118, 117, 116, 114, 113, 112, 111,
  109, 108, 107, 106, 104, 103, 102, 101, 100, 99, 98, 96, 95, 94, 93, 92,
  91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76,
  75, 74, 73, 72, 71, 71, 70, 69, 68, 67, 66, 65, 64, 64, 63, 62,
  61, 60, 60, 59, 58, 57, 57, 56, 55, 54, 54, 53, 52, 51, 51, 50,
  49, 49, 48, 47, 46, 46, 45, 45, 44, 43, 43, 42, 41, 41, 40, 40,
  39, 38, 38, 37, 37, 36, 35, 35, 34, 34, 33, 33, 32, 32, 31, 31,
  30, 30, 29, 29, 28, 28, 27, 27, 26, 26, 26, 25, 25, 24, 24, 23,
  23, 23, 22, 22, 21, 21, 21, 20, 20, 20, 19, 19, 19, 18, 18, 18,
  17, 17, 17, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 13, 13, 13,
  13, 13, 12, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10,
  10, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 8, 8};

void CreaPattern(char *pattern, int colpi, int steps) {
  int cur = steps;
  for (int i = 0; i < steps; i++) {
    *pattern = '.';
    if (cur >= steps) {
      cur -= steps;
      *pattern = 'x';
    } 
    if (colpi==0) *pattern = '.';
    pattern++;
    cur += colpi;
  }
}

void hitDrum(){
  playing = false;
  impactPos = startPos << 8;
  sinePos = 0;
  playing = true;
}

void update()
{
  if (playing)
  {
    impactPos += incrementSpeed;
    sinePos += pgm_read_byte(&inviluppo[(impactPos >> 8) & 0xff]);
    sample = pgm_read_byte(&sinusoide[(sinePos >> 4) & 0xff]);
    if (impactPos > 0xff00) playing = false;
  }
  else if (sample) sample--; // Fade out 
}


// Routine di interrupt relativa al timer 0
ISR(TIM0_COMPA_vect){
  OCR1A = sample;
  update();
  if (timer == 0) clock= !clock;
  timer++;
  if (timer > tempo) timer=0;
}

void setup() {
  pinMode(1, OUTPUT);                                // Pin 6 corrisponde a OC1A,PB1
  TCCR0A = 0;                                        // Reset del TCCR0A registro di controllo dei timer
  TCCR0B = 0;                                        // Reset del TCCR0B registro di controllo dei timer
  TIMSK = 1<<OCIE0A;                                 // Setto OCIEOA per abilitare l'interrupt del timer 0
  TCCR0A = 1<<WGM00 | 1<<WGM01;                      // Fast PWM
  TCCR0B = 1<<WGM02 | 1<<CS02| 0<<CS01 | 0<<CS00;    // Fast PWM | Setto CS02 per avere fClock/256 = 8000KHz / 256 = 31.250 KHz  
  TCCR1  = 1<<PWM1A | 2<<COM1A0 | 1<<CS10; 
  CreaPattern((char *)&pattern, random (3,15), 16);
}

void loop() {
  tempo = 20500 - analogRead (2)*20;
  if (clock != lastClockState ) {
    startPos = 130;
    if ( pos == 4 || pos == 12) startPos = 100;
    numeroColpi = map (analogRead(3),0,1023,4,13);
    CreaPattern((char *)&pattern,numeroColpi, 16);
    if (numeroColpi > 12) CreaPattern((char *)&pattern,random (7,11),16);
    if (pattern[pos] == 'x') gateState = HIGH;
    else gateState = LOW;
    if (++pos > 15) pos=0;
    if (gateState == HIGH) hitDrum();
  }  
  lastClockState = clock;
} 














