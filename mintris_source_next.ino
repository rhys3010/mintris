/**
  * @file mintris_source.ino
  * @author Rhys Evans (rhe24@aber.ac.uk)
  * @version 3.0
  * @date 7 December 2016
  * @brief Mintris Game for the CGVG arduino shield - CC12020 Assignment 1
*/

#include <AberLED.h>
#include <EEPROM.h>
#define TESTING

///////////////////
// Screen Variables
///////////////////

#define EMPTY 0
#define REDBLOCK 1
#define X 0
#define Y 1

// Speed at wich the dot moves down, start at 300ms
int fallInterval;

// Array to store the locations of all red blocks
int redBlocks[8][8];

// Array to store pattern data of every space on
// the screen during end and start states
int screenData[8][8];

///////////////////
// Timing Variables
///////////////////

#define END_INTERVAL 1000
#define START_INTERVAL 1000
#define PAUSE_INTERVAL 250
#define GROW_INTERVAL 10000

unsigned long gameStartTime;
// Keep track of how long the user has spent in the paused state
unsigned long timePaused = 0;
unsigned long getGamePlayTime(){
  // will return the amount of time the user has been playing for
  return millis() - gameStartTime;
}

// Dot decsent timer
unsigned long timeOfLastFall = 0;

// Handles the timing of growing the tower
unsigned long timeOfLastGrow = 0;

////////////////////
// Scoring Variables
////////////////////

// Store current highest scores
unsigned long bestTimeSeconds;
byte bestRowsCleared;

// Store most recent scores
byte rowsCleared = 0;
unsigned long totalTimeSeconds;

////////////////////////////////////////////
//
// State machine code
//
////////////////////////////////////////////

// The program has 4 states.
// Start: Default state, LED Pattern will show until button 5 is pressed.
// Playing: The state in which the game runs
// Paused: This state is entered and exited when 'FIRE' is pressed
// End: display LED Pattern and can only be exited after 1 second has passed.

#define S_START 0
#define S_PLAYING 1
#define S_PAUSED 2
#define S_END 3

// Note: The timing functions for the state machine are adapted bits of code
// from worksheet 2 written by Jim Finnis.*
// * https://blackboard.aber.ac.uk/bbcswebdav/pid-751629-dt-content-rid-1107210_1/courses/CS12020_AB0_2016-17/CS12020_cgvg_ws2.pdf

unsigned long stateStartTime;
int state;
// This function will change the current state of the system
// and declare the start time of the state.
void gotoState(int s){
  state = s;
  stateStartTime = millis();
}

unsigned long getStateTime(){

  //Time the system has been in a specific state for
  return millis() - stateStartTime;
}

// Function to Display a smiley face with two horizontal lines
// on the start screen
void drawStartStatePattern(){

  // Setting draw locations for the smiley face
  screenData[2][2] = 2;
  screenData[5][2] = 2;
  screenData[1][5] = 2;
  screenData[6][5] = 2;

  for(int index = 2; index < 6; index++){
    screenData[index][6] = 2;
  }

  //setting draw locations for the border
  for(int index = 0; index < 8; index++){
    screenData[index][0] = 1;
    screenData[index][7] = 1;
  }

  // Drawing the pattern
  for(int x = 0; x < 8; x++){
    for(int y = 0; y < 8; y++){
      if(2 == screenData[x][y]){
        AberLED.set(x, y, GREEN);
      }
      if(1 == screenData[x][y]){
        AberLED.set(x, y, RED);
      }
    }
  }
}

// Function to Display a sad face with two horizontal lines at
// end screen for two seconds.
void drawEndStatePattern(){

  // Setting draw locations for the sad face
  screenData[2][2] = 2;
  screenData[5][2] = 2;
  screenData[1][6] = 2;
  screenData[6][6] = 2;

  for(int index = 2; index < 6; index++){
    screenData[index][5] = 2;
  }

  //setting draw locations for the border
  for(int index = 0; index < 8; index++){
    screenData[index][0] = 1;
    screenData[index][7] = 1;
  }

  if(getStateTime() < 2000){
    // Drawing the pattern
    for(int x = 0; x < 8; x++){
      for(int y = 0; y < 8; y++){
        if(2 == screenData[x][y]){
          AberLED.set(x, y, GREEN);
        }
        if(1 == screenData[x][y]){
          AberLED.set(x, y, RED);
        }
        if(0 == screenData[x][y]){
          AberLED.set(x, y, BLACK);
        }
      }
    }
  }
}

// Function to hang the program safely in the event of an error
// Also prints different error messages depending where the error occured
void error(int num){

  clearScreenData();
  // Print error message to serial monitor
  Serial.print("ERROR: ");
  Serial.print(num);
  Serial.println();

  // Show error Display on screen
  for(;;){
    AberLED.clear();

    for(int y = 1; y < 8; y++){
      for(int x = 3; x < 5; x++){
        AberLED.set(x, y, RED);
      }
    }

    // Render exclamation mark
    AberLED.set(3,5,BLACK);
    AberLED.set(4,5, BLACK);
    AberLED.swap();
  }
}

////////////////////////////////////////////
//
// Moving Dot code
//
////////////////////////////////////////////


// The position of the yellow falling dot.
// [0] = x , [1] = y
// The positions will be initialized as 4 and 0 : at the boot up  of the game the dot
// will always start at the middle of the screen
// The y position will decrease until it reaches either a red block or y=8
int dotPos[2] = {4, 0};

// Variable that holds the status of the game
bool lostGame = false;

// Initialize dot by moving to top of screen and assigning a new (random) x position
void initDot(){
  dotPos[Y] = 0;
  dotPos[X] = random(8);
}


// Check if the dot has collided with anything and reinitialize the falling dot.
// This makes the dot dissapear from current position and 'respawn' at top of screen
// Also deal with the dot's speed.
void updateDot(){

  if(checkForDotCollision()){

    initDot();
  }

  if(getStateTime() - timeOfLastFall > fallInterval){

    // reset timer
    timeOfLastFall = getStateTime();
    dotPos[Y]++; // Move dot
  }

  // If the dot is 'spawned' on the same locaiton as a red block end the game
  if(REDBLOCK == redBlocks[dotPos[X]][0]){
    lostGame = true;
  }

  checkForLostGame();

  // Slowly increase speed of dot, for every 5 rows cleared
  // the dot moves faster every time at un-uniform increments
  switch(rowsCleared){

    case 0 ... 5:
      fallInterval = 300;
      break;
    case 6 ... 10:
      fallInterval = 250;
      break;

    case 11 ... 15:
      fallInterval = 200;
      break;

    case 16 ... 20:
      fallInterval = 150;
      break;

    case 21 ... 25:
      fallInterval = 125;
      break;

    case 26 ... 31:
      fallInterval = 100;
      break;

    // Maximum speed
    case 32 ... 36:
      fallInterval = 75;
      break;
  }
}

void drawDot(){
  AberLED.set(dotPos[X], dotPos[Y], YELLOW);
}

// will be called when user presses DOWN button (extra feature)
void skipDotPath(){
  // If there is no red block at the bottom of the row that the
  // dot is falling in move the dot's position to that location
  if(EMPTY == redBlocks[dotPos[X]][7]){
    dotPos[Y] = 7;
  }

  // Check whole row top down for a red block
  // and move the dot position 1 above the first red block it detects (y - 1)
  for(int y = 0; y < 8; y++){
    if(REDBLOCK == redBlocks[dotPos[X]][y]){
      dotPos[Y] = y-1;
      break;
    }
  }
}


// Checks if the dot is in the same position as a red block and
// check if the dot has reached the bottom of the screen. Both these
// events will be treated as a collision.
bool checkForDotCollision(){

  if(dotPos[Y] > 7){
    return true;
  }

  if(REDBLOCK == redBlocks[dotPos[X]][dotPos[Y]]){
    return true;
  }

  else{
    return false;
  }
}


// Check if the game should end
void checkForLostGame(){

  if(true == lostGame){
    // Scoring code occurs in here to avoid looping
    updateScores();
    gotoState(S_END);
    displayScoreToSerial();
  }
}


////////////////////////////////////////////
//
// Red Blocks code
//
////////////////////////////////////////////


// To be called at every state change
void clearScreenData(){
  // Clears whole screen of any red blocks
  // or state screen patterns
  for(int x = 0; x < 8; x++){
    for(int y = 0; y < 8; y++){
      redBlocks[x][y] = EMPTY;
      screenData[x][y] = 0;
    }
  }
}


// Checks the redBlocks array at specific co-ordinates
bool checkForRedBlock(int x, int y){

  if(REDBLOCK == redBlocks[x][y]){
    return true;
  }

  else{
    return false;
  }
}

// This checks every single slot of the grid and draws a red block
// at every location where there is an allocated slot for one.
// The nested for loop was derived from Jim Finnis' code from worksheet 3.*
// * https://blackboard.aber.ac.uk/bbcswebdav/pid-751630-dt-content-rid-1107211_1/courses/CS12020_AB0_2016-17/main%283%29.pdf
void drawRedBlocks(){

  for(int x = 0; x < 8; x++){
    for(int y = 0; y < 8; y++){
      if(checkForRedBlock(x,y)){
        AberLED.set(x, y, RED);
      }
    }
  }
}

// This function scans the whole bottom row and increments
// a variable by 1 every time it finds a red block
// if the variable value is 8 (full bottom row) it will return true.
bool checkForFullBottomRow(){

  int counter = 0;

  // Checking every x position in the 7th row for a red block
  for(int x = 0; x < 8; x++){
    if(REDBLOCK == redBlocks[x][7]){
      counter++;
    }
  }
  // Checking if the bottom row is full of red blocks
  if(8 == counter){
    return true;
  }
  else{
    return false;
  }
}



// Same method to scroll blocks as was used in worksheet 3*
// *https://blackboard.aber.ac.uk/bbcswebdav/pid-751630-dt-content-rid-1107211_1/courses/CS12020_AB0_2016-17/main%283%29.pdf
// For every slot of the grid (apart from the bottom row) moves content
// of row above down 1 row.
void scrollRows(){
  // Scroll the tower down
  for(int y = 6; y >= 0; y--){
    for(int x = 0; x < 8; x++){
      redBlocks[x][y+1] = redBlocks[x][y];
    }
  }

  // Clear the top row after scrolling
  for(int x = 0; x < 8; x++){
    redBlocks[x][0] = EMPTY;
  }

}

// Scroll tower up
void growTower(){

  // Check every slot in the grid and for every
  // Red block add another red block above it
  for(int x = 0; x < 8; x++){
    for(int y = 1; y < 8; y++){
      if(REDBLOCK == redBlocks[x][y]){
        redBlocks[x][y-1] = REDBLOCK;
      }
    }
  }
}

// This will check if a red block exists in a specific location
// and will assign a location for the red block at the location of
// a collision
void updateBlocks(){

  if(checkForDotCollision()){

    redBlocks[dotPos[X]][dotPos[Y]-1] = REDBLOCK;
  }

  if(checkForFullBottomRow()){
    scrollRows();
    rowsCleared++;
  }

  // Add an extra layer of blocks every 10 seconds
  if((getGamePlayTime() - timePaused) - timeOfLastGrow > GROW_INTERVAL){ // Take pausing into account
    timeOfLastGrow = getGamePlayTime() - timePaused; // Reset timer
    growTower();
  }
}

////////////////////////////////////////////
//
// EEPROM Code
//
////////////////////////////////////////////
// note: Method used to split my best time score integer into 2 bytes
// for reading and writing to the EEPROM was based on code from Arduino.cc*
// *http://playground.arduino.cc/Code/EEPROMReadWriteLong

// Write high scores to EEPROM
void saveScore(){

  // Using bitwise operators to split the 16-bit integer
  // to two bytes of data
  byte first = ((bestTimeSeconds >> 0) & 0xFF);
  byte second = ((bestTimeSeconds >> 8) & 0xFF);

  // Save two parts of best time integer to
  // EEPROM at corresponding address
  EEPROM.update(0, first);
  EEPROM.update(1, second);

  // Save high score for rows cleared
  EEPROM.update(2, bestRowsCleared);
}

// Currently can only hold 255 row Clears
// as it is stored as a byte.
unsigned int loadTimeScore(){

  byte first = EEPROM.read(0);
  byte second = EEPROM.read(1);

  return ((first << 0) & 0xFF) + ((second << 8) & 0xFFFF);
}

// load high score of row clears.
int loadRowsScore(){

  return EEPROM.read(2);
}


////////////////////////////////////////////
//
// Scoring Code
//
////////////////////////////////////////////

#define HOUR 0
#define MIN 1
#define SEC 2

// Update the time and row values
void updateScores(){

  bestTimeSeconds = loadTimeScore();
  bestRowsCleared = loadRowsScore();

  totalTimeSeconds = (getGamePlayTime() - timePaused) / 1000; // Account for time spent in PAUSED state

  // Check if the current time is higher than the
  // high score and assign as high score if it is.
  if(totalTimeSeconds > bestTimeSeconds){
    bestTimeSeconds = totalTimeSeconds;
  }

  if(rowsCleared > bestRowsCleared){
    bestRowsCleared = rowsCleared;
  }

  saveScore();
}

// Print scoring data onto serial monitor
void displayScoreToSerial(){

  // Convert total time in seconds to hours minutes and seconds
  // in order: Hour [0] - Min [1] - Sec [2]
  int totalTime[3] = {(totalTimeSeconds / 3600), ((totalTimeSeconds / 60) % 60), (totalTimeSeconds % 60)};
  int bestTotalTime[3] = {(bestTimeSeconds / 3600), ((bestTimeSeconds / 60) % 60), (bestTimeSeconds % 60)};

  // Print notice of new high score
  if((totalTimeSeconds == bestTimeSeconds && 0 != totalTimeSeconds) || (rowsCleared == bestRowsCleared && 0 != rowsCleared) ){ // if there is no high score set yet best and current rows cleared will always be equal
    Serial.print("\nNEW HIGH SCORE!!");
  }

  // Print Time
  Serial.print("\n\n");
  Serial.print("===== TIME SURVIVED =====");
  Serial.print("\n");
  Serial.print(totalTime[HOUR]);
  Serial.print("hr ");
  Serial.print(totalTime[MIN]);
  Serial.print("min ");
  Serial.print(totalTime[SEC]);
  Serial.println("sec.");

  //Print rows cleared
  Serial.print("\n");
  Serial.print("===== ROWS CLEARED =====");
  Serial.print("\n");
  Serial.print(rowsCleared);

  // Print best time
  Serial.print("\n\n");
  Serial.print("-------------------------------");
  Serial.print("\n\n");
  Serial.print("===== BEST TIME SURVIVED =====");
  Serial.print("\n");
  Serial.print(bestTotalTime[HOUR]);
  Serial.print("hr ");
  Serial.print(bestTotalTime[MIN]);
  Serial.print("min ");
  Serial.print(bestTotalTime[SEC]);
  Serial.println("sec.");

  // Print best rows cleared
  Serial.print("\n");
  Serial.print("===== BEST ROWS CLEARED =====");
  Serial.print("\n");
  Serial.print(bestRowsCleared);

  Serial.print("\n_______________________________\n");

}

// Display the amount of rows cleared to the screen at END state
void displayScoreToScreen(){

  // Display score after 2 seconds has passed
  if(getStateTime() > 2000){
    clearScreenData();

    // Using the modulo operator to print a dot at a corresponding location on the screen to
    // the number of rows have been cleared.

    AberLED.set((rowsCleared % 8) - 1, rowsCleared / 8, GREEN);

    if(rowsCleared % 8 == 0){
      AberLED.set(7, (rowsCleared /8) - 1, GREEN);
    }
  }
}



////////////////////////////////////////////
//
// Main loop functions
//
////////////////////////////////////////////

// Setup code will begin the AberLED library,
// put the system in its default state (Start)
// and seed the random number generator
void setup(){

  Serial.begin(9600);
  AberLED.begin();
  gotoState(S_START);
  randomSeed(analogRead(4));

}

// Calling this function will check for button presses and execute as follows:
// Fire button: Starts the game if in start state
// Left / Right button: move dot in relevant direction
void handleInput(){

  switch (state){

    case S_START:

      // Change state to PLAYING if 1 second has passed since
      // state was entered and button 5 was pressed
      if(AberLED.getButtonDown(FIRE) && getStateTime() > START_INTERVAL){
        gotoState(S_PLAYING);
        // Set / Reset timer
        gameStartTime = millis();
        timePaused = 0;
        // Reset rows cleared score
        rowsCleared = 0;
        clearScreenData();
      }
      break;

    case S_PLAYING:

      // Move dot if appropriate button is pressed, checks if dot can move and
      // that it does not move out of the screen.
      if(AberLED.getButtonDown(LEFT) && dotPos[X] > 0){

        // Check if the space to the left of the dot is a red block
        if(REDBLOCK == redBlocks[dotPos[X]-1][dotPos[Y]]){
          lostGame = true;
        }
        dotPos[X]--;
      }

      if(AberLED.getButtonDown(RIGHT) && dotPos[X] < 7){

        if(REDBLOCK == redBlocks[dotPos[X]+1][dotPos[Y]]){
          lostGame = true;
        }
        dotPos[X]++;
      }

      // Move the dot to the end of it's path
      if(AberLED.getButtonDown(DOWN)){
        skipDotPath();
      }

      //Detect FIRE button to pause game
      if(AberLED.getButtonDown(FIRE) && getStateTime() > PAUSE_INTERVAL){
        gotoState(S_PAUSED);
      }
      break;

    case S_END:

      // Change state to START if 1 second has passed
      // and if button 5 has been pressed
      if(AberLED.getButtonDown(FIRE) && getStateTime() > END_INTERVAL){
        gotoState(S_START);
        clearScreenData();
      }
      break;

    // Check for button 5 press
    case S_PAUSED:
      if(AberLED.getButtonDown(FIRE) && getStateTime() > PAUSE_INTERVAL){
        timePaused = timePaused + getStateTime(); // Store how long the game was paused for
        gotoState(S_PLAYING);
      }
      break;

    default:
      error(1);
  }
}


void update(){

  switch (state){

    case S_START:
      lostGame = false;
      break;

    case S_PLAYING:
      updateBlocks();
      updateDot();
      break;

    case S_END:
      break;

    // Keep the dot stationary by not updating it
    case S_PAUSED:
      break;

    default:
      error(2);
  }
}

void render(){

  AberLED.clear();

  switch (state){

    case S_START:
      drawStartStatePattern();
      break;

    case S_PLAYING:
      drawDot();
      drawRedBlocks();
      break;

    case S_END:
      drawEndStatePattern();
      displayScoreToScreen();
      break;

    case S_PAUSED:
      drawDot();
      drawRedBlocks();
      break;

    default:
      error(3);
  }

  AberLED.swap();
}

void loop(){

  handleInput();
  update();
  render();
}
