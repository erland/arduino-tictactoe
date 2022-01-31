#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <NoDelay.h>

int s0 = 3;
int s1 = 4;
int s2 = 5;
int s3 = 6;
int sig = A0;
int buttonPin = 9;

#define EMPTY ' '
#define RING 'O'
#define CROSS 'X'

int defaultValue[9] = {0,0,0,0,0,0,0,0,0};
char markers[9] = {EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY};

Adafruit_8x16matrix matrix = Adafruit_8x16matrix();

noDelay ledBlinkRate(1000);
noDelay boardScanRate(500);
noDelay resetTimer(5000);

void setup() {
  Serial.begin(9600);
  matrix.begin(0x70); 
  matrix.setBrightness(1);

  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(buttonPin, INPUT);

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

void drawWinningPos(const int positions[3]) {
  matrix.clear();
  for(int i=0;i<3;i++) {
    int y = positions[i]/3;
    int x = positions[i]%3;
    drawBlinkPixel(x,y,75);
  }
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
bool prepareForReset = false;
char nextMarkerType = ' ';
enum GameType {
  NotStarted,
  Human2Human,
  Human2Computer
};
enum GameType gameType = NotStarted;
bool computerTurn = false;
int waitingForComputerMarker = -1;
int lastPlaced = -1;
int lastPlacedIndicator = 0;
bool buttonPressed = false;
bool computerStarted = false;

void loop() {
  if(digitalRead(buttonPin)==HIGH) {
    if(!buttonPressed) {
      delay(100); // Debounce delay
    }
    buttonPressed = true;
  }else {
    if(buttonPressed) {
      delay(100); // Debounce delay
      if(gameType == NotStarted) {
        gameType = Human2Computer;
        computerTurn = true;
        computerStarted = true;
        waitingForComputerMarker = -1;
      }else if(gameType == Human2Computer && isBoardEmpty()) {
        gameType = NotStarted;
        computerTurn = false;
      }
    }
    buttonPressed = false;
  }
  if(boardScanRate.update()) {
    scanBoard();
  }
  if(resetTimer.update()) {
    if(prepareForReset && isBoardEmpty()) {
      Serial.println("Resetting, ready for game");
      prepareForReset = false;
      gameType = NotStarted;
      drawReadySignal();
    }else if(isBoardEmpty() && gameType!=Human2Computer) {
      Serial.println("Board empty, preparing for reset");
      prepareForReset = true;
    }else {
      prepareForReset = false;
    }
  }
  if(gameType == NotStarted) {
    if(isNewGameStarted(CROSS)) {
      Serial.println("Human to computer game started");
      gameType = Human2Computer;
      computerTurn = true;
      waitingForComputerMarker = -1;
      computerStarted = false;
    }else if(isNewGameStarted(RING)) {
      gameType = Human2Human;
      nextMarkerType = CROSS;
    }
  }
  
  if(gameType == Human2Computer) {
    if(computerTurn && waitingForComputerMarker<0) {
      calculateAndPlaceComputerMarker();
      Serial.print("Selecting computer position (");
      Serial.print(waitingForComputerMarker%3);
      Serial.print(",");
      Serial.print(waitingForComputerMarker/3);
      Serial.println(") waiting for placement");
    }else if(computerTurn) {
      if(getMarker(waitingForComputerMarker%3, waitingForComputerMarker/3) == RING) {
        Serial.println("Computer marker placed, your turn");
        waitingForComputerMarker = -1;
        computerTurn = false;
      }
    }else {
      if(markerCount(CROSS)>markerCount(RING) && markerCount(EMPTY)>0) {
        Serial.println("Your marker registered, waiting for computer");
        computerTurn = true;
      }else if(computerStarted && markerCount(CROSS)==markerCount(RING)) {
        Serial.println("Your marker registered, waiting for computer");
        computerTurn = true;
      }
    }
  }

  const int *winningPos = isWinner(markers, RING);
  if(winningPos != NULL) {
    drawWinningPos(winningPos);
  }else {
    winningPos = isWinner(markers, CROSS);
    if(winningPos != NULL) {
      drawWinningPos(winningPos);
    }else {
      if(lastPlaced >= 0 && lastPlacedIndicator == 0 && (waitingForComputerMarker>=0 || gameType == Human2Human)) {
        lastPlacedIndicator = 1;
      }
      if(ledBlinkRate.update()) {
        matrix.clear(); 
        if(gameType == Human2Computer && waitingForComputerMarker>=0 && lastPlacedIndicator==0) {
            if(isOdd) {
              drawPixel(waitingForComputerMarker%3, waitingForComputerMarker/3, 1);
            }
        }
        if(lastPlacedIndicator>0) {
            drawPixel(lastPlaced%3, lastPlaced/3, 1);
            lastPlacedIndicator--;
            if(lastPlacedIndicator==0) {
              lastPlaced = -1;
            }
        }
        matrix.writeDisplay();
        isOdd = !isOdd;
      }
    }
  }
}

bool isNewGameStarted(char markerType) {
  return markerCount(EMPTY)==8 && markerCount(markerType)==1;
}

void calculateAndPlaceComputerMarker() {
  // Check if we can win
  char tempBoard[9];
  for(int i=0;i<9;i++) {
    if(getMarker(i%3,i/3)==EMPTY) {
      memcpy(tempBoard, markers, sizeof(char)*9);
      tempBoard[i] = RING;
      if(isWinner(tempBoard, RING) != NULL) {
        waitingForComputerMarker = i;
        break;
      }
    }
  }
  if(waitingForComputerMarker<0) {
    // Check if opponent can win
    char tempBoard[9];
    for(int i=0;i<9;i++) {
      if(getMarker(i%3,i/3)==EMPTY) {
        memcpy(tempBoard, markers, sizeof(char)*9);
        tempBoard[i] = CROSS;
        if(isWinner(tempBoard, CROSS) != NULL) {
          waitingForComputerMarker = i;
          break;
        }
      }
    }
  }
  if(waitingForComputerMarker<0) {
    // Take best available move
    int bestMoves[9] = {4,0,2,6,8,1,3,5,7};
    for(int i=0;i<9;i++) {
      int pos = bestMoves[i];
      if(getMarker(pos%3,pos/3)==EMPTY) {
        waitingForComputerMarker = pos;
        break;
      }
    }
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

int availablePositions(int positionBuffer[9]) {
  int nextFree = 0;
  for(int i=0;i<9;i++) {
    if(markers[i] == EMPTY) {
      positionBuffer[nextFree++] = i;
    }
  }
  return nextFree;
}

const int winningPositions[][3] = {
  {0,1,2},
  {3,4,5},
  {6,7,8},
  {0,3,6},
  {1,4,7},
  {2,5,8},
  {0,4,8},
  {2,4,6}
};

const int* isWinner(char *board, char markerType) {
  for(int i=0;i<8;i++) {
    int matching = 0;
    for(int pos=0;pos<3;pos++) {
      if(board[winningPositions[i][pos]]==markerType) {
        matching++;
      }
    }
    if(matching==3) {
      Serial.print("Winner(");
      Serial.print(markerType);
      Serial.print(") at: ");
      Serial.print(winningPositions[i][0]);
      Serial.print(",");
      Serial.print(winningPositions[i][1]);
      Serial.print(",");
      Serial.print(winningPositions[i][2]);
      Serial.println("");
      return winningPositions[i];
    }
  }
  return NULL;
}

bool isBoardEmpty() {
  for(int i=0;i<9;i++) {
    if(markers[i]!=EMPTY) {
      return false;
    }
  }
  return true;
}

char getMarker(int x, int y) {
  return markers[y*3+x];
}

int setMarker(int x, int y, char value) {
  if(markers[y*3+x] == EMPTY && value != EMPTY) {
    if(lastPlaced != y*3+x) {
      Serial.print("Placed ");
      Serial.print(value);
      Serial.print(" at ");
      Serial.print(x);
      Serial.print(",");
      Serial.println(y);
      lastPlaced = y*3+x;
    }
  }
  markers[y*3+x] = value;
}

int markerCount(char markerType) {
  int count = 0;
  for(int i=0;i<9;i++) {
    if(markers[i] == markerType) {
      count++;
    }
  }
  return count;
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
