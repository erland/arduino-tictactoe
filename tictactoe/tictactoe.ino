#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <NoDelay.h>

int s0 = 3;
int s1 = 4;
int s2 = 5;
int s3 = 6;
int sig = A0;

#define EMPTY ' '
#define RING 'O'
#define CROSS 'X'

int defaultValue[9] = {0,0,0,0,0,0,0,0,0};
char markers[9] = {EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY};

Adafruit_8x16matrix matrix = Adafruit_8x16matrix();

noDelay ledBlinkRate(1000);
noDelay boardScanRate(500);

void setup() {
  Serial.begin(9600);
  matrix.begin(0x70); 
  matrix.setBrightness(1);

  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);

  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);

  Serial.print("Initializing with: ");
  for ( int y=0;y<3;y++) {
    for (int x=0;x<3;x++) {
      defaultValue[y*3+x] = readMux(y*3+x);
      Serial.print(defaultValue[y*3+x]);
      if(x!=2 || y!=2 ) {
        Serial.print(", ");
      }
    }
  }
  Serial.println("");
  drawReadySignal();
  Serial.println("Ready to play");
}

void drawReadySignal() {
  matrix.clear();
  drawBlinkPixel(0,0,100);
  drawBlinkPixel(1,0,100);
  drawBlinkPixel(2,0,100);
  drawBlinkPixel(2,1,100);
  drawBlinkPixel(2,2,100);
  drawBlinkPixel(1,2,100);
  drawBlinkPixel(0,2,100);
  drawBlinkPixel(0,1,100);
  drawBlinkPixel(1,1,100);
  drawBlinkPixel(1,1,50);
  drawBlinkPixel(1,1,100);
}

void drawBlinkPixel(int x, int y, int rate) {
  drawPixel(x,y,1);
  matrix.writeDisplay();
  delay(rate);
  drawPixel(x,y,0);
  matrix.writeDisplay();
  delay(rate);
}

bool isOdd = true;

void loop() {
  if(boardScanRate.update()) {
    scanBoard();
  }
  if(ledBlinkRate.update()) {
    if(isOdd) {
      printBoard(true);
    }else {
      printBoard(false);
    }
    isOdd = !isOdd;
  }
}

void scanBoard() {
  for ( int y=0;y<3;y++) {
    for (int x=0;x<3;x++) {
      int current = readMux(y*3+x);
      int offset = defaultValue[y*3+x];
      if(current<offset-20) {
        setMarker(x, y, CROSS);
      }else if(current>offset+20) {
        setMarker(x, y, RING);
      }else {
        setMarker(x,y, EMPTY);
      }
    }
  }
}

void printBoard(bool skipRing) {
  matrix.clear(); 
  for ( int y=0;y<3;y++) {
    for (int x=0;x<3;x++) {
      char m = getMarker(x, y);
      if(m == RING) {
        if(skipRing) {
          drawPixel(x,y,1);
        }
        //Serial.print("O");
      }else if(m == CROSS) {
        drawPixel(x,y,1);
        //Serial.print("X");
      }else {
        //Serial.print("_");
      }
    }
    //Serial.println("");
  }
  matrix.writeDisplay();
  //Serial.println("**********************");

}

char getMarker(int x, int y) {
  return markers[y*3+x];
}

int setMarker(int x, int y, char value) {
  markers[y*3+x] = value;
}

void drawPixel(int x, int y, int value) {
  matrix.drawPixel(2-y, 2-x, value);  
}

int readMux(int channel) {
  int controlPin[] = {s3, s2, s1, s0};
  int muxChannel[9][4] = {
    {0,0,0,0},
    {0,0,0,1},
    {0,0,1,0},
    {0,0,1,1},
    {0,1,0,0},
    {0,1,0,1},
    {0,1,1,0},
    {0,1,1,1},
    {1,0,0,0}
  };

  for (int i = 0;i<4;i++) {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  int val = analogRead(sig);

  return val;
}
