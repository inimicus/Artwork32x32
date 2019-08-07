/*
* Modified by Markus Lipp adding interleaved buffers, streaming, 32x32 & 24bit support
* Based on "_16x32_Matrix R3.0" by Creater Alex Medeiros, http://PenguinTech.tk
* Use code freely and distort its contents as much as you want, just remeber to thank the 
* original creaters of the code by leaving their information in the header. :)
*/


//PortC[0:11] = {15, 22, 23, 9, 10, 13, 11, 12, 28, 27, 29, 30}
//PortD[0:7] = {2, 14, 7, 8, 6, 20, 21, 5}
//Define pins
const uint8_t
//PortC
APIN      = 15, BPIN      = 22, CPIN      = 23, DPIN = 9,
CLOCKPIN  = 10, LATCHPIN  = 13, OEPIN     = 11,
//PortD
R1PIN     = 2, R2PIN     = 8,
G1PIN     = 14, G2PIN     = 6,
B1PIN     = 7, B2PIN     = 20;

uint8_t pinTable[13] = {
  R1PIN,R2PIN,G1PIN,G2PIN,B1PIN,B2PIN,
  APIN,BPIN,CPIN,DPIN,CLOCKPIN,LATCHPIN,OEPIN};

//Addresses 1/8 rows Through a decoder
uint16_t const A = 1, B = 2,C = 4, D=8;
//Acts like a 16 bit shift register
uint16_t const SCLK   = 16;
uint16_t const LATCH  = 32;
uint16_t const OE     = 64;

byte buffer[64];
uint8_t packetsTotal    = 64,
        packetsCount    = 0;

uint16_t const abcVar[16] = { //Decoder counter var
  0,A,B,A+B,C,C+A,C+B,A+B+C,
  0+D,A+D,B+D,A+B+D,C+D,C+A+D,C+B+D,A+B+C+D};

//Data Lines for row 1 red and row 9 red, ect.
uint16_t const RED1   = 1, RED2   = 8;
uint16_t const GREEN1 = 2, GREEN2 = 16;
uint16_t const BLUE1  = 4, BLUE2  = 32;

const uint8_t SIZEX = 32;
const uint8_t SIZEY = 32;



//BAM and interrupt variables
uint8_t     rowN    = 0;
uint16_t    BAM;
uint8_t     BAMMAX  = 7; //now 24bit color! (0-7)


void setup() {
  for(uint8_t i = 0; i < 13; i++){
        pinMode(pinTable[i], OUTPUT);
    }
  timerInit();
  Serial.begin(250000); 
}

uint8_t r,g, prevVal,val;
int dataPos=0;


byte interleavedBuffer[] = {0,16,8,26,18,58,18,62,24,16,32,0,8,20,28,32,16,8,24,38,56,44,44,16,8,48,8,20,16,16,8,24,
    9,9,25,63,55,55,63,51,9,41,53,33,33,1,57,25,9,17,17,27,49,61,57,17,17,1,57,41,17,33,9,49,
    0,16,16,10,2,42,42,10,48,16,24,16,8,40,40,8,32,8,36,26,16,24,24,40,16,40,16,48,32,16,32,32,
    62,22,22,60,20,4,12,4,30,6,6,6,30,54,6,14,30,14,22,4,14,6,6,22,46,30,22,14,54,14,38,62,
    44,4,4,44,4,12,44,44,12,4,4,4,4,12,4,4,12,52,44,4,4,4,4,12,20,60,4,44,36,20,52,12,
    20,60,60,20,60,44,12,20,12,12,12,12,12,4,4,4,4,20,4,12,12,12,12,12,4,36,60,20,28,60,28,20,
    24,24,24,24,24,8,8,8,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,0,8,56,24,24,24,24,24,24,
    32,32,32,32,32,48,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,32,32,32,32,32,

    37,23,23,22,1,63,27,20,51,40,6,56,5,49,17,29,49,35,7,29,31,40,34,17,33,13,1,13,35,13,9,57,
    11,49,38,36,54,45,46,63,61,6,48,2,59,29,53,3,27,57,50,59,51,10,63,47,27,23,43,55,37,31,51,7,
    8,4,11,48,35,54,11,52,12,45,52,33,28,48,0,12,44,56,63,0,36,8,37,52,60,32,20,32,32,48,44,16,
    54,38,61,54,50,10,42,61,14,2,23,30,30,10,2,14,30,6,4,17,30,43,6,30,46,46,22,46,54,22,6,30,
    12,52,10,4,59,19,11,42,20,4,20,28,12,12,12,4,28,28,12,14,28,8,0,28,28,36,60,44,36,36,52,20,
    20,28,20,27,28,60,12,20,4,28,8,4,12,4,4,4,12,12,0,12,4,20,28,4,12,28,36,52,60,28,28,4,
    24,24,24,24,24,24,8,8,8,8,8,8,0,0,0,0,0,0,0,0,8,8,8,8,0,56,56,56,56,24,24,24,
    32,32,32,32,32,32,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,32,32,

    14,56,43,25,28,45,49,40,52,14,19,29,10,5,57,53,35,28,9,36,43,46,59,49,53,21,31,54,13,57,7,21,
    42,39,42,15,11,30,47,60,32,38,53,41,45,1,31,36,0,58,34,61,3,35,53,52,8,56,34,13,10,12,10,0,
    21,51,11,55,48,34,28,35,24,1,20,20,7,36,4,3,39,27,16,8,17,6,26,31,39,47,21,7,28,7,21,39,
    30,38,22,34,46,35,9,29,21,31,10,3,30,18,0,26,50,12,1,20,7,24,2,30,14,30,22,22,17,54,22,14,
    52,44,11,47,23,52,61,22,14,4,1,4,12,9,8,24,8,12,0,0,4,1,5,12,28,52,60,4,58,4,28,12,
    36,52,20,51,27,32,41,3,12,8,8,8,12,8,0,8,8,0,8,2,8,8,8,12,12,28,36,28,24,28,60,44,
    56,56,24,56,24,3,26,8,0,0,0,0,0,0,0,4,28,0,0,9,0,0,0,0,0,8,56,24,28,24,24,8,
    0,0,32,0,32,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,32,32,32,48,

    4,3,5,4,7,18,28,40,14,59,1,53,42,5,56,33,7,3,20,37,27,36,41,44,51,41,18,4,35,35,39,37,
    15,59,38,55,26,26,60,2,42,38,59,29,56,6,47,47,22,62,44,57,1,10,57,31,23,63,20,35,1,39,47,15,
    3,52,10,10,42,12,5,54,38,17,61,56,60,57,0,47,11,20,42,49,54,44,30,42,23,35,36,26,11,13,59,27,
    42,37,19,19,23,59,50,59,3,15,10,11,4,58,24,52,44,26,22,13,3,7,15,23,42,34,17,26,47,40,42,10,
    48,62,28,28,31,44,36,4,27,12,16,4,3,8,25,8,28,0,13,3,8,20,12,0,0,16,34,24,13,13,8,40,
    0,4,32,32,35,48,16,24,12,0,4,8,12,1,8,16,16,9,0,8,9,0,0,12,24,32,56,32,53,50,48,48,
    4,0,3,3,0,3,51,11,0,0,8,0,0,0,0,16,8,0,0,0,0,8,0,0,12,20,4,3,6,4,4,4,
    56,56,56,56,56,56,8,0,0,0,0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,8,56,60,56,56,56,56,

    25,34,36,61,31,7,54,17,23,48,48,2,33,15,47,47,35,24,38,26,11,22,51,7,53,2,36,9,56,28,38,35,
    42,27,17,36,13,18,4,35,60,48,47,28,25,13,23,28,51,47,24,17,16,62,9,45,10,52,17,45,49,13,24,24,
    56,32,2,57,14,32,24,57,37,35,60,10,25,8,43,62,35,32,48,58,4,48,46,58,12,44,22,16,56,43,39,24,
    61,61,62,63,45,49,41,16,13,19,19,59,12,26,56,16,17,18,61,51,59,6,19,35,53,41,50,28,56,56,61,37,
    34,26,31,25,41,37,30,41,42,0,12,12,63,56,24,33,4,17,11,8,1,6,0,0,50,26,55,49,29,37,26,26,
    24,56,60,62,46,46,39,10,4,4,0,4,0,1,17,24,24,1,4,4,8,1,4,28,0,16,8,10,61,29,56,56,
    4,4,0,0,16,32,16,20,16,8,8,0,0,0,8,16,8,8,0,0,0,8,8,8,28,4,12,3,6,6,4,4,
    56,56,56,56,56,24,8,8,8,0,0,0,0,0,0,8,0,0,0,0,0,0,0,0,8,8,0,60,56,56,56,56,

    31,15,41,9,17,43,6,13,32,19,50,43,15,4,29,34,38,55,52,35,0,9,25,63,33,7,7,51,6,18,19,55,
    21,9,37,21,29,5,35,40,14,5,13,19,17,49,15,23,11,46,46,26,17,31,4,4,39,57,62,15,30,60,29,1,
    10,46,14,22,54,46,24,23,3,61,54,31,0,59,27,31,19,51,3,11,49,52,59,35,26,14,7,29,25,53,54,22,
    29,21,37,61,53,29,2,5,47,0,12,11,31,8,25,40,60,1,16,59,48,11,15,27,1,13,13,40,55,8,29,29,
    2,58,34,2,50,10,7,6,31,7,4,63,51,17,41,41,57,9,15,0,15,11,4,8,6,10,10,50,2,50,58,58,
    56,48,40,8,24,8,0,3,4,0,15,4,12,8,24,16,0,16,9,4,0,4,8,12,0,8,8,39,63,7,56,56,
    4,12,28,60,52,4,12,12,8,8,0,0,0,0,0,16,16,0,0,0,0,0,0,0,12,4,4,20,12,12,4,4,
    56,0,0,0,8,0,0,0,0,0,0,0,0,0,0,8,8,8,0,0,0,0,0,0,0,0,0,8,0,0,56,56,

    57,7,23,31,47,32,15,51,51,8,3,61,51,52,32,8,54,63,51,19,55,55,32,63,29,31,53,27,18,63,39,63,
    61,61,53,53,37,21,54,9,22,34,4,9,2,13,12,17,55,6,57,45,34,45,16,28,11,53,5,45,61,13,29,21,
    17,1,9,1,9,9,12,63,43,45,39,1,44,2,3,45,34,2,1,21,63,27,54,52,49,1,1,17,14,9,41,9,
    51,3,3,3,19,3,10,0,42,17,44,19,15,39,32,0,19,56,6,1,22,7,10,0,7,3,11,11,10,3,43,59,
    14,14,14,14,6,14,7,5,22,9,24,52,28,1,9,50,40,0,24,2,9,4,5,11,6,6,6,6,15,14,38,62,
    8,8,8,8,0,8,8,10,14,4,0,15,15,16,24,16,0,25,9,12,0,0,0,4,8,8,8,0,8,8,40,0,
    60,4,4,4,12,4,4,3,6,3,7,0,0,8,0,9,17,0,8,0,0,0,0,0,4,4,4,12,4,4,28,12,
    0,0,0,0,0,0,0,4,1,0,0,0,0,0,0,0,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    23,23,63,7,13,44,47,20,56,5,42,44,6,21,34,21,23,1,49,24,63,31,14,0,38,23,62,7,61,63,22,62,
    53,9,1,49,53,43,49,23,38,42,38,16,42,52,12,0,36,25,37,33,61,63,60,48,39,61,40,57,9,9,8,12,
    6,2,10,2,14,21,36,23,25,42,2,27,41,44,19,63,52,39,2,18,0,0,59,33,56,38,43,2,2,10,51,7,
    10,14,14,6,10,4,28,61,47,60,10,36,5,3,34,50,48,16,25,27,31,6,5,22,18,18,22,14,14,14,6,2,
    15,7,15,15,7,13,9,10,31,28,32,16,13,9,0,19,16,19,25,8,8,6,7,15,15,15,15,7,15,15,7,15,
    8,0,0,8,8,10,5,4,7,6,25,0,22,16,16,9,26,8,0,4,0,1,0,4,0,0,8,8,0,8,0,8,
    4,4,4,4,4,4,1,0,7,2,7,15,0,0,9,1,0,1,0,8,0,0,0,0,4,4,4,4,4,4,12,4,
    0,0,0,0,0,0,6,7,0,1,0,0,8,8,0,0,9,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,

    59,35,19,3,50,26,20,36,33,38,32,48,53,28,43,11,61,27,48,39,13,46,34,50,39,44,51,59,3,10,35,34,
    31,43,3,47,11,33,21,45,19,56,3,51,60,9,42,53,57,48,61,60,13,48,36,7,20,53,3,51,51,58,51,34,
    33,21,53,33,29,35,42,62,2,18,8,1,25,59,39,18,3,31,25,7,42,58,36,41,1,5,37,37,37,12,21,28,
    44,4,12,28,60,16,47,53,46,5,55,26,23,0,16,40,56,17,27,1,29,0,26,36,46,44,20,28,28,5,60,13,
    29,13,13,13,13,10,27,23,31,41,31,5,0,17,26,25,0,11,0,24,3,6,2,29,31,25,13,13,13,13,13,13,
    10,2,2,2,2,7,4,15,3,27,14,12,8,0,9,1,25,8,9,8,0,1,5,2,4,2,2,2,2,2,10,2,
    4,4,4,4,4,3,0,0,3,7,2,3,8,0,1,0,8,9,8,8,0,0,0,4,0,4,4,4,4,4,4,4,
    0,0,0,0,0,4,7,0,4,0,1,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    56,63,7,27,41,22,4,37,57,58,46,60,45,34,12,44,62,38,6,13,59,48,22,27,41,48,31,15,31,63,7,39,
    60,0,52,47,5,56,35,56,6,20,12,2,46,56,41,42,8,42,42,50,39,62,50,50,36,12,56,36,20,44,28,0,
    41,37,41,34,35,28,48,58,37,37,30,30,2,16,42,0,25,9,0,9,21,6,15,16,7,33,21,41,57,17,57,45,
    19,19,19,17,20,54,21,54,51,18,1,10,2,17,1,41,56,8,25,17,8,2,7,51,32,27,51,27,11,11,11,19,
    9,9,9,9,13,12,10,21,20,14,7,1,24,8,17,8,24,17,1,8,2,0,1,15,27,1,9,1,1,1,1,9,
    6,6,6,6,5,7,4,15,15,1,1,6,1,0,8,24,8,8,0,0,1,1,0,4,4,6,6,6,6,6,6,6,
    4,4,4,4,6,0,0,0,0,0,3,2,8,8,0,0,0,8,8,8,0,0,0,0,0,4,4,4,4,4,4,4,
    0,0,0,0,0,7,7,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    63,20,44,54,41,20,16,56,19,6,46,0,27,0,4,38,41,6,31,11,22,51,55,37,32,27,36,12,52,52,28,12,
    7,24,56,13,12,13,13,23,49,61,39,33,59,45,55,59,20,41,6,40,10,9,1,54,55,12,16,24,32,32,40,40,
    34,61,21,42,33,16,20,4,10,5,25,16,20,50,2,2,23,52,13,0,63,1,12,27,9,33,21,61,21,21,5,29,
    22,12,12,30,27,54,30,40,5,3,7,1,8,0,13,53,40,11,50,17,1,7,11,10,33,32,52,4,12,12,44,4,
    8,2,2,0,2,11,51,27,7,0,0,11,0,25,36,48,56,54,24,0,3,3,1,5,25,30,10,2,2,2,26,2,
    7,7,7,7,7,4,11,3,0,1,2,4,2,0,26,16,24,17,1,0,1,1,1,0,2,7,7,7,7,7,7,7,
    4,4,4,4,3,0,3,4,0,0,0,1,9,8,1,8,0,8,8,8,0,1,0,0,4,4,4,4,4,4,4,4,
    0,0,0,0,4,7,4,0,0,0,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    54,61,13,46,56,6,11,58,9,44,14,30,47,43,0,2,6,14,47,46,48,50,12,41,18,5,21,37,13,5,5,61,
    31,17,1,32,1,26,45,28,7,13,49,32,30,42,1,15,3,59,6,30,53,10,60,2,43,49,33,41,17,17,57,41,
    26,28,20,13,57,8,21,7,2,26,21,17,37,14,0,58,44,60,29,38,13,6,18,27,20,12,20,36,12,52,4,28,
    33,51,51,35,47,18,4,2,0,0,11,11,18,41,49,13,29,46,50,22,0,1,1,13,15,35,51,27,59,11,43,3,
    46,14,14,30,30,48,45,0,5,8,8,14,9,33,56,0,55,54,58,11,1,0,0,10,6,30,14,6,6,6,30,6,
    31,7,7,7,4,11,25,4,7,0,0,1,8,23,55,55,7,49,53,1,2,0,8,5,7,7,7,7,7,7,7,7,
    4,4,4,4,0,4,1,7,0,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,4,4,4,4,4,4,4,4,
    0,0,0,0,7,7,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    10,25,17,34,21,36,50,56,39,33,24,13,10,47,30,15,28,37,2,4,59,4,61,52,22,16,41,17,41,49,9,1,
    51,45,9,51,46,11,23,13,19,6,44,33,51,2,11,36,18,13,58,19,3,44,37,9,12,55,1,13,37,53,37,13,
    18,8,32,61,23,33,7,3,11,22,10,31,1,44,55,51,16,12,4,54,10,26,11,29,11,0,32,0,32,48,24,8,
    54,36,4,44,30,55,32,1,0,8,25,14,1,0,33,23,39,19,9,15,0,1,25,7,8,12,28,60,28,12,4,36,
    15,29,45,29,48,17,22,1,7,0,1,9,11,48,49,32,33,48,48,5,3,9,9,0,1,5,5,5,5,5,5,29,
    7,5,29,5,12,13,11,5,3,1,8,8,4,1,1,0,1,32,49,1,6,0,0,11,1,5,5,5,5,5,5,5,
    4,6,6,6,0,1,3,6,4,0,0,0,0,0,0,1,0,1,0,3,1,0,0,4,2,6,6,6,6,6,6,6,
    0,0,0,0,7,6,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,

    61,63,46,38,58,1,10,3,36,29,2,15,15,2,22,17,50,25,22,53,52,44,9,32,55,40,45,7,23,62,7,55,
    22,58,34,61,25,42,21,29,29,2,5,24,52,54,3,18,21,37,62,38,62,63,41,22,31,20,18,26,10,59,58,26,
    29,1,1,3,52,56,33,48,2,1,39,35,11,43,10,18,0,8,30,57,2,15,28,21,5,1,9,57,57,57,57,9,
    55,43,3,43,32,23,23,31,11,0,9,16,8,17,43,5,14,46,60,12,8,16,8,7,14,7,3,3,3,3,3,59,
    13,25,41,25,17,14,14,3,7,4,24,0,1,14,29,49,57,25,22,26,0,9,4,13,15,13,1,1,1,1,1,1,
    5,1,25,1,13,3,12,3,3,7,8,1,0,6,0,9,1,0,3,3,1,8,7,13,3,5,1,1,1,1,1,1,
    6,2,2,2,1,3,7,12,4,0,0,8,0,0,1,0,0,1,8,9,0,0,0,6,4,6,2,2,2,2,2,2,
    0,4,4,4,6,4,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,4,4,4,4,4,4,

    18,57,8,50,48,40,63,42,24,11,31,12,6,26,22,47,54,9,38,39,2,26,49,41,56,56,33,35,1,49,57,40,
    4,2,22,20,23,3,24,58,57,61,38,41,24,48,34,50,5,57,52,49,25,62,18,46,7,20,30,18,34,54,26,54,
    54,58,14,22,3,16,0,16,12,20,16,17,62,43,33,10,19,38,53,54,47,0,13,0,50,15,14,2,26,14,34,46,
    37,1,61,61,35,3,14,6,2,3,7,6,0,13,28,18,4,17,13,15,24,0,6,19,21,10,5,1,1,5,1,29,
    18,6,2,10,18,9,5,8,2,12,11,9,9,17,27,9,3,24,20,25,1,8,15,4,9,14,2,14,6,2,6,2,
    8,0,0,0,15,13,8,6,2,8,11,0,0,5,12,4,4,15,12,8,8,4,4,12,15,2,8,0,0,0,0,0,
    3,3,3,3,3,1,7,1,5,7,4,0,0,14,8,8,8,8,8,8,0,7,7,7,0,5,3,3,3,3,3,3,
    4,4,4,4,4,6,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,4,4,4,4,4,4,

    26,1,9,25,51,61,47,45,47,60,43,2,54,15,38,23,43,31,41,43,57,41,58,31,40,61,51,57,17,19,27,58,
    47,3,3,55,46,31,18,28,31,23,26,48,42,50,46,57,33,33,47,7,39,31,15,47,45,12,23,19,2,54,26,37,
    54,56,56,52,25,11,13,7,12,8,4,13,9,7,5,16,12,62,22,48,18,15,17,16,23,7,4,0,1,9,33,47,
    18,4,4,8,8,4,2,14,3,11,11,12,16,30,0,8,24,6,22,8,7,3,3,8,6,12,4,12,12,4,4,24,
    15,5,5,5,2,10,5,2,7,14,6,3,9,13,24,0,8,16,9,17,8,6,14,8,10,9,5,13,5,5,5,5,
    0,2,2,2,7,4,8,8,1,0,0,1,8,0,8,0,8,10,7,8,1,0,8,6,0,0,2,10,2,2,2,2,
    3,3,3,3,3,0,7,1,0,1,1,0,0,8,8,8,0,9,8,8,0,1,1,1,1,7,11,3,3,3,3,3,
    4,4,4,4,4,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,4,4};

byte tempInterleavedBuffer[4096];

//byte interleavedBuffer[4096];

void loop() {       

    int n;
    n = RawHID.recv(buffer, 0); // 0 timeout = do not wait
    if (n > 0) {

        if(strcmp("START", (const char*)buffer) == 0) {

            //packetsCount = buffer[3];

//            Serial.print("Received packet #");
//            Serial.println(packetsCount);


            Serial.println((const char*)buffer);
            packetsCount = 0;

        } else if (strcmp("STOP", (const char*)buffer) == 0) {

            Serial.println((const char*)buffer);
            packetsCount = 0;

            memcpy(interleavedBuffer, tempInterleavedBuffer, 4096);

//            for(uint16_t i = 0; i < 4096; i++) {
//                Serial.print(interleavedBuffer[i]);
//                Serial.print(",");
//            }

        } else {

            // Reconstruct data
            for (int i = 0; i < 64; i++) {
                uint16_t progress = (packetsCount * 64)+i;
                tempInterleavedBuffer[progress] = buffer[i];

                Serial.print("Writing: ");
                Serial.println(progress);
            }

            //Serial.println(packetsCount);
            packetsCount++;

        }

//        Serial.print("First byte: ");
//        Serial.println((int)buffer[0]);

//        packetsCount++;
//
//        if (packetsCount > packetsTotal) {
//            packetsCount = 0;
//        }

    }

}

IntervalTimer timer1;

void timerInit() {
    BAM = 0;
    attackMatrix();
}


//The updating matrix stuff happens here
//each pair of rows is taken through its BAM cycle
//then the rowNumber is increased and id done again
void attackMatrix() {
    timer1.end();    
    
    uint16_t portData;
    portData = 0; // Clear data to enter
    portData |= (abcVar[rowN]);//|OE; // abc, OE
    portData &=~ LATCH;        //LATCH LOW
    GPIOC_PDOR = portData;  // Write to Port
    
    uint8_t *start = &interleavedBuffer[rowN*SIZEX*8+BAM*32];    
    for(uint8_t _x = 0; _x < 32; _x++){
          GPIOD_PDOR = start[_x]; // Transfer data
          GPIOC_PDOR |=  SCLK;// Clock HIGH
          GPIOC_PDOR &=~ SCLK;// Clock LOW
    }

    GPIOC_PDOR |= LATCH;// Latch HIGH
    GPIOC_PDOR &=~ OE;// OE LOW, Displays line

    #define LOOPTIME 2 //trial&error to get both smooth gradients & little flicker
    #define CALLOVERHEAD 1
    timer1.begin(attackMatrix,((LOOPTIME+CALLOVERHEAD)<<BAM)-CALLOVERHEAD);
  
    if(BAM >= BAMMAX) { //Checks the BAM cycle for next time.    
        if(rowN == 15){
            rowN = 0;
        } else {
            rowN ++;
        }        
        BAM = 0;
      } else {
        BAM ++;
    }
}
