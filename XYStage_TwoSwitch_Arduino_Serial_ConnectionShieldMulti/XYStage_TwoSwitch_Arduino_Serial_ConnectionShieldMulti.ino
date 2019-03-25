
/*
Table of Contants
1. forwardstep1 function
2. backwardstep1 fucntion
3. forwardstep2 function
4. backwardstep2 function
5. populateLocations function
6. moveNext function - moves the motors to the next location on well plate.
7. moveLast function - opposite of moveNext
8. set function - moves to a user specified position on well plate
9. eject function - ejects the well plate
10. reset function - starts homing procedure for XY stage
11. setup function (Arduino)
12. main loop
*/

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

//INTIAL SETUP
const int delayRPM = 255;
const int rows = 8; // Amount of rows on well plate.
const int columns = 12; // Amount of columns on well plate.
const int numLocations = ((rows * columns) / 3) + 1; // Amount of triplet wells on well plate. (Plus 1 for the eject location).
const float maximumSpeed = 255.0; // Max Speed of the Stepper Motors.
const float acceleration = 2000.0; // Acceleration for the Stepper Motors.
const int stepsPerRevolution = 200; // Steps per revolution for the Stepper Motor.
const int stepsPerWell = 225; // Amount of steps needed from center of well to the next.
const int stepsToEject = 1000; // Amount of steps to eject entire well plate.
const int limitSwitchT = 37; // Digital Pin Number for Top Stepper Motor Limit Switch.
const int limitSwitchB = 36; // Digital Pin Number for Bottom Stepper Motor Limit Switch.
const int TwoSwitchA = 32; // Digital Pin Number for TwoSwitchA.
const int TwoSwitchB = 33; // Digital Pin Number for TwoSwitchB.
const int TwoSwitchC = 34; // Digital Pin Number for TwoSwitchC.

//Function Declarations
void forwardstep1();
void backwardstep1();
void forwardstep2();
void backwardstep2();
void populateLocations(long (&locations)[numLocations][2]);
void moveGen(Adafruit_StepperMotor *motor, int delay, int myDirection, int stepType);
void moveRight();
void moveLeft();
void moveDown();
void moveUp();
int moveNext();
int moveLast();
int set();
int eject();
void reset();

// Create an Adafruit Motor Shield Object and stepper motor objects.
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotorT = AFMS.getStepper(stepsPerRevolution, 1);
Adafruit_StepperMotor *myMotorB = AFMS.getStepper(stepsPerRevolution, 2);

// Now we'll wrap the 3 steppers in an AccelStepper object
AccelStepper stepperB(forwardstep1, backwardstep1);
AccelStepper stepperT(forwardstep2, backwardstep2);

// Create MultiStepper object.
MultiStepper steppers;

// Declarations for positioning.
// position[0] is for top motor.
// position[1] is for bottom motor.
// For Top Motor: Negative is to Left. Positive to Right.
// For Bottom Motor: Positive is Down. Negative is Up.
int myPosition = 0;
long locations[numLocations][2];
long positions[2];

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  //Start the Arduino Motor Shield.
  AFMS.begin();

  // Set Motor Speeds not used in MultiStepper and AccelStepper.
  // Only for independent motor control.
  myMotorT -> setSpeed(delayRPM);
  myMotorB -> setSpeed(delayRPM);

  //Setting Stepper Motors configuratins beforehand.
  stepperT.setMaxSpeed(maximumSpeed);
  stepperT.setAcceleration(acceleration);

  stepperB.setMaxSpeed(maximumSpeed);
  stepperB.setAcceleration(acceleration);

  // Adding both steppers to a MultiStepper manager.
  steppers.addStepper(stepperT);
  steppers.addStepper(stepperB);

  // Populate locations based on the well plate type.
  populateLocations(locations);

  // Multistepper intitialitze positions.
  positions[0] = 0;
  positions[1] = 0;

  // Setting Pin Modes for TwoSwitches.
  pinMode(TwoSwitchA, OUTPUT);
  pinMode(TwoSwitchB, OUTPUT);
  pinMode(TwoSwitchC, OUTPUT);

  // Setting Pin Modes for Limit Switches.
  pinMode(limitSwitchB, INPUT_PULLUP);
  pinMode(limitSwitchT, INPUT_PULLUP);

  // Sets the XY Stage at the Origin.
  //reset();

  //code to set system initially
  //Serial.println("XY Stage Control: Send Your Commands!");

}

//MAIN LOOP
void loop() {
  //ascii to decimal for XY Stage
  //? = 63 Request Identifier
  //Z = 90 Reset/Set function
  //N = 78 Next function
  //L = 76 Previous/Last function
  //E = 69 Done/Eject function

  //Ascii to Decimal for TwoSwitches
  //TwoSwitch A: Collection(A) = 65, Recirculation(S) = 83
  //TwoSwitch B: Collection(D) = 68, Recirculation(F) = 70
  //TwoSwitch C: Collection(G) = 71, Recirculation(H) = 72

  //Here we're looping non-stop checking the serial buffer each time.
  //If you send several serial commands while the system is in one loop
  //it will not catch on the next loop. If you send just one then it will.
  //The way it's currently set up it reads the whole buffer at once so
  //something like 8378 will not be recognized as S and then N afterwards.

//  while(!digitalRead(limitSwitchT)){
//    Serial.println("It is pressed! TOP");
//  }
//
//  while(!digitalRead(limitSwitchB)){
//    Serial.println("It is pressed! BOT");
//  }

  int incomingByte;
  if (Serial.available() > 0){
    incomingByte = Serial.read();
    //Serial.println(incomingByte);

//////////////////////////////////////////////
    //? Request Unique ID for Serial Connection.
    if (incomingByte == 63){
      //Serial.println("Requested/Sending Unique Identifier!");
      Serial.write("2xCS");
    }

    //S(et)
    if (incomingByte == 90) {
      //Serial.println("Setting XY Stage!");
      //unsigned long startTime = millis();
      reset();
      //unsigned long endTime = millis();
      //Serial.println(endTime - startTime);
    }

    //N(ext)
    if (incomingByte == 78) {
      //Serial.println("Moving to Next Position!");
      //unsigned long startTime = millis();
      myPosition = moveNext();
      //unsigned long endTime = millis();
      //Serial.println(myPosition);
      //Serial.println(endTime - startTime);
      Serial.println(myPosition);
    }

    //P(revious)
    if (incomingByte == 76) {
      //Serial.println("Moving to Previous Position!");
      //unsigned long startTime = millis();
      myPosition = moveLast();
      //unsigned long endTime = millis();
      //Serial.println(myPosition);
      //Serial.println(endTime - startTime);
      Serial.println(myPosition);
    }

    //D(one)
    if (incomingByte == 69) {
      //Serial.println("Finished Collecting!");
      eject();
    }
//////////////////////////////////////////////
    // Two Switch Collection Protocol.
    if (incomingByte == 65) {
      //Serial.println("Setting TwoSwitch A to Collection.");
      // code to switch the TwoSwitch so fluid will be collected on well plate.
      digitalWrite(TwoSwitchA, LOW);
    }

    if (incomingByte == 83) {
      //Serial.println("Setting TwoSwitch A to Recirculation");
      // code to switch the TwoSwitch so fluid will be recirculated to the chip.
      digitalWrite(TwoSwitchA, HIGH);
    }

    if (incomingByte == 68) {
      //Serial.println("Setting TwoSwitch B to Collection");
      // code to switch the TwoSwitch so fluid will be collected on well plate.
      digitalWrite(TwoSwitchB, LOW);
    }

    if (incomingByte == 70) {
      //Serial.println("Setting TwoSwitch B to Recirculation");
      // code to switch the TwoSwitch so fluid will be recirculated to the chip.
      digitalWrite(TwoSwitchB, HIGH);
    }

    if (incomingByte == 71) {
      //Serial.println("Setting TwoSwitch C to Collection");
      // code to switch the TwoSwitch so fluid will be collected on well plate.
      digitalWrite(TwoSwitchC, LOW);
    }

    if (incomingByte == 72) {
      //Serial.println("Setting TwoSwitch C to Recirculation");
      // code to switch the TwoSwitch so fluid will be reciruclated to the chip.
      digitalWrite(TwoSwitchC, HIGH);
    }
  }
}

//*******************************************************************
//FUNCTIONS

// You can change these to DOUBLE or INTERLEAVE or MICROSTEP!
// Wrappers for the first motor!
void forwardstep1() {
  myMotorB->onestep(FORWARD, SINGLE);
}
void backwardstep1() {
  myMotorB->onestep(BACKWARD, SINGLE);
}
// Wrappers for the second motor!
void forwardstep2() {
  myMotorT->onestep(FORWARD, SINGLE);
}
void backwardstep2() {
  myMotorT->onestep(BACKWARD, SINGLE);
}
// This function populates the location matrix with the layout
// for moving to any location on the well plate.
void populateLocations(long (&locations)[numLocations][2]){
  int tripletCol = 1;
	for (int i = 0; i < numLocations - 1; i++){
		if (i % rows == 0){
			for (int j = 0; j < rows; j++){
				switch (tripletCol){
					case 1: locations[i+j][0] = stepsPerWell * j;
									locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
									break;
					case 2: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 3: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 4: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 5: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 6: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
  		             break;
  				case 7: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 8: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 9: locations[i+j][0] = stepsPerWell * j;
                  locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                  break;
  				case 10: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
					case 11: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
  				case 12: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
  				case 13: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
					case 14: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
  		  	case 15: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
  				case 16: locations[i+j][0] = stepsPerWell * j;
                   locations[i+j][1] = -(tripletCol - 1) * (3 * stepsPerWell);
                   break;
  				default: break;
				}
			}
			tripletCol = tripletCol + 1;
		}
	}
  locations[numLocations - 1][0] = locations[numLocations - 2][0] + stepsToEject;
  locations[numLocations - 1][1] = -((columns / 3) - 1) * (3 * stepsPerWell);
}


//GENERIC MOVE FUNCTION
void moveGen(Adafruit_StepperMotor *motor, int delay, int myDirection, int stepType){
  //steps - total number of steps
  //myDirection - FORWARD 1, BACKWARD 2, BRAKE 3, RELEASE 4.
  //Stepper Motor Nema 17 1.8 degrees/step. 200 Step/Revolution
  //Delay is handled by setting the RPM.
  //Step Types include SINGLE, DOUBLE, INTERLEAVE, MICROSTEP.
  if (motor == myMotorT){
    motor -> setSpeed(delay);
    motor -> step(stepsPerWell, myDirection, stepType);
  }
  else if (motor == myMotorB){
    motor -> setSpeed(delay);
    motor -> step(stepsPerWell, myDirection, stepType);
  }

}

//MOVE RIGHT FUNCTION
void moveRight(){
  if (myMotorT != NULL){
    moveGen(myMotorT, delayRPM, FORWARD, SINGLE);
  }
}

//MOVE LEFT FUNCTION
void moveLeft(){
  if (myMotorT != NULL){
    moveGen(myMotorT, delayRPM, BACKWARD, SINGLE);
  }
}

//MOVE DOWN FUNCTION
void moveDown(){
  if (myMotorB != NULL){
    moveGen(myMotorB, delayRPM, BACKWARD, SINGLE);
  }
}

//MOVE UP FUNCTION
void moveUp(){
  if (myMotorB != NULL){
    moveGen(myMotorB, delayRPM, FORWARD, SINGLE);
  }
}

// This function moves the stepper motors to the next position and
// returns the position once its done getting there.
int moveNext(){
  /* code here */
  // if (myPosition < 31){
  //   positions[0] = locations[myPosition + 1][1];
  //   positions[1] = locations[myPosition + 1][0];
  //   steppers.moveTo(positions);
  //   steppers.runSpeedToPosition();
  //   myPosition = myPosition + 1;
  //   //Serial.println(myPosition);
  // }
  // return myPosition;

  //Snake Pattern.
  if (myPosition == 31){
    Serial.println("You have reached the end, can't go further!");
    return 31;
  }
  int right[3] = {7,15,23};
  int down[14] = {0,1,2,3,4,5,6,16,17,18,19,20,21,22};
  int up[14] = {8,9,10,11,12,13,14,24,25,26,27,28,29,30};

  for (int i = 0; i < 32; i++){ //Might only need to loop up until myPosition + 1?
    if (myPosition == down[i]){
      moveDown();
      myPosition = myPosition + 1;
      break;
    }

    if (myPosition == up[i]){
      moveUp();
      myPosition = myPosition + 1;
      break;
    }

    if (myPosition == right[i]){
      for (int j = 0; j < 3; j++){
        moveRight();
      }
      myPosition = myPosition + 1;
      break;
    }
  }

  return myPosition;
}

// This function moves the stepper motors to the last position and
// returns the position once its done getting there.
int moveLast(){
  /* code here */
  // if (myPosition > 0){
  //   positions[0] = locations[myPosition - 1][1];
  //   positions[1] = locations[myPosition - 1][0];
  //   steppers.moveTo(positions);
  //   steppers.runSpeedToPosition();
  //   myPosition = myPosition - 1;
  //   //Serial.println(myPosition);
  // }
  // return myPosition;

  // Snake Pattern.
  if (myPosition == 0){
    Serial.println("You are at the beginning can't go back a step!");
    return 0;
  }
  int left[3] = {8,16,24};
  int down[14] = {9,10,11,12,13,14,15,25,26,27,28,29,30,31};
  int up[14] = {1,2,3,4,5,6,7,17,18,19,20,21,22,23};

  for (int i = 0; i < 32; i++){  //Might only need to loop up until myPosition + 1?
    if (myPosition == down[i]){
      moveDown();
      myPosition = myPosition - 1;
      break;
    }

    if (myPosition == up[i]){
      moveUp();
      myPosition = myPosition - 1;
      break;
    }
    
    if (myPosition == left[i]){
      for (int j = 0; j < 3; j++){
        moveLeft();
      }
      myPosition = myPosition - 1;
      break;
    }
  }

  return myPosition;
}

// SETS TO A USER SPECIFIED POSITION.
// NOT SURE THIS FEATURE WILL BE NECCESARY AS OF YET.
int set(){
  /* code here */
  return myPosition;
}

// USER DONE WITH COLLECTION MOVES WELL PLATE TO EJECT.
int eject() {
  /* code here*/
//  positions[0] = locations[32][1];
//  positions[1] = locations[32][0];
//  steppers.moveTo(positions);
//  steppers.runSpeedToPosition();
  myPosition = 32;
  myMotorB -> setSpeed(delayRPM);
  myMotorB -> step(8*225, FORWARD, SINGLE);
  return myPosition;
}

// SETS POSITION BACK AT ORIGIN (Limit Swith Locations)
void reset() {

  //Start Homing procedure of Stepper Motors at startup.
  //Homing Procedure Top Steper Motor. SINGLE MOTOTS CONTROL.
//    while (digitalRead(limitSwitchT)) {  // Do this until the switch is activated
//      myMotorT -> onestep(FORWARD,SINGLE);
//      delay(5);                       // Delay to slow down speed of Stepper
//    }
//
//    while (!digitalRead(limitSwitchT)) { // Do this until the switch is not activated
//      myMotorT -> onestep(BACKWARD, SINGLE);
//      delay(10);                       // More delay to slow even more while moving away from switch
//    }
//    myMotorT -> step(50, BACKWARD, SINGLE);
//
//   while (digitalRead(limitSwitchB)) {  // Do this until the switch is activated
//     myMotorB -> onestep(BACKWARD, SINGLE);
//     delay(5);                       // Delay to slow down speed of Stepper
//     //Serial.println("Switch not activated");
//   }
//
//   while (!digitalRead(limitSwitchB)) { // Do this until the switch is not activated
//     myMotorB -> onestep(FORWARD, SINGLE);
//     delay(10);                       // More delay to slow even more while moving away from switch
//     //Serial.println("Switch activated");
//   }
//   myMotorB -> step(50, FORWARD, SINGLE);

  //Start Homing procedure of Stepper Motors at startup.
  //Homing Procedure Top Steper Motor. MULTI STEPPER.
 while (digitalRead(limitSwitchT) || digitalRead(limitSwitchB)){
   // Serial.println(digitalRead(limitSwitchT));
   // Serial.println(digitalRead(limitSwitchB));
   if (!digitalRead(limitSwitchT) && digitalRead(limitSwitchB)){
     positions[1] = positions[1] - 1;
     //Serial.println("Did if");
   }
   else if (!digitalRead(limitSwitchB) && digitalRead(limitSwitchT)){
     positions[0] = positions[0] + 1;
     //Serial.println("Did else if");
   }
   else {
     positions[0] = positions[0] + 1;
     positions[1] = positions[1] - 1;
     //Serial.println("Did else");

   }
   steppers.moveTo(positions);
   steppers.runSpeedToPosition();

  }

// while (!digitalRead(limitSwitchT) || !digitalRead(limitSwitchB)){
//   positions[0] = positions[0] - 1;
//   positions[1] = positions[1] + 1;
//   steppers.moveTo(positions);
//   steppers.runSpeedToPosition();
// }

  //CODE TO MOVE THE WELL PLATE TO WHERE THE OUTLETS ARE ON TOP LEFT THREE WELLS.
  positions[0] = positions[0] - 4349;
  positions[1] = positions[1] + 2570;

  steppers.moveTo(positions);
  steppers.runSpeedToPosition();

  stepperB.setCurrentPosition(0);
  stepperT.setCurrentPosition(0);

  myPosition = 0;  // Reset position variable to zero.
  positions[0] = locations[myPosition][1];
  positions[1] = locations[myPosition][0];
}