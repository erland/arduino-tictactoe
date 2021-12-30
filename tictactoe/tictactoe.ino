#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

int s0 = 3;
int s1 = 4;
int s2 = 5;
int s3 = 6;
int sig = A0;

int defaultValue[9] = {0,0,0,0,0,0,0,0,0};
int markers[9] = {0,0,0,0,0,0,0,0,0};

Adafruit_8x16matrix matrix = Adafruit_8x16matrix();

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

  for ( int y=0;y<3;y++) {
    for (int x=0;x<3;x++) {
      defaultValue[y*3+x] = readMux(y*3+x);
    }
  }

  Serial.println("Initialized");
}

void loop() {
  scanBoard();
  printBoard();

  delay(500);
}

void scanBoard() {
  for ( int y=0;y<3;y++) {
    for (int x=0;x<3;x++) {
      int current = readMux(y*3+x);
      int offset = defaultValue[y*3+x];
      if(current<offset-20) {
        markers[y*3+x] = -1;
      }else if(current>offset+20) {
        markers[y*3+x] = 1;
      }else {
        markers[y*3+x] = 0;
      }
    }
  }
}

void printBoard() {
  matrix.clear(); 
  for ( int y=0;y<3;y++) {
    for (int x=0;x<3;x++) {
      int m = markers[y*3+x];
      if(m>0) {
        matrix.drawPixel(2-y, 2-x, 1);  
        Serial.print("O");
      }else if(m<0) {
        matrix.drawPixel(2-y, 2-x, 1);  
        Serial.print("X");
      }else {
        Serial.print("_");
      }
    }
    Serial.println("");
  }
  matrix.writeDisplay();
  Serial.println("**********************");

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
