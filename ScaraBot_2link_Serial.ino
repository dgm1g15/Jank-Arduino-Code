#include <Arduino.h>
#include <string.h>

#define stepPin 2
#define dirPin 5
#define stepPin_Y 3
#define dirPin_Y 6


// Variable definition:
const int target = 500;
int v = 2000;
double mini_step = 200.0/360.0;
double position = 0.0;
String angle_read;
double angle_d;
double angle_d_Y;
int steps = 0;
unsigned long timings[400] = {};    // The timing arrays will contain the timestamps in ms for each of the pulses (on and off)
unsigned long timings_X[400] = {};  // Axis 1 (X)
unsigned long timings_Y[400] = {};  // Axis 2 (Y)
int position_reached = 0;           // Int to track the number of positions reached (can be a boolean if you only intend to track one position)
long startMicros = 0; 
int interval = 200;
unsigned long elapsedMillis = 0;    // Timer value to track the elapsed time since the start of the motion commmand.
bool condition_HIGH_1;              // Boolean that sets to TRUE whenever a HIGH pulse needs to be sent to the first motor
bool condition_HIGH_2;              // Boolean that sets to TRUE whenever a HIGH pulse needs to be sent to the second motor
bool condition_LOW_1;               // Boolean that sets to TRUE whenever a LOW pulse needs to be sent to the first motor
bool condition_LOW_2;               // Boolean that sets to TRUE whenever a LOW pulse needs to be sent to the second motor
double c_high=0;
double c_low=0;
unsigned long timings_HIGH[400] = {};
unsigned long timings_LOW[400] = {};


void calcSteps(double angle) {
  /* 
   *  This function is to calculate the angle for each motor - this would be replaced by a proper kinematics model later on
   */
  double angle_to_take = angle - position;          // (position = last known angle, set to 0 to start with)
  if(angle_to_take<0){
    digitalWrite(dirPin, LOW);                      // Check if the stepper needs to move in the positive or negative direction and switch the direction pin accordingly.
    digitalWrite(dirPin_Y, HIGH);                   // In this particular setup (2-link SCARA), each motor's direction is always the opposite sign to the other's.
  }
  else{
    digitalWrite(dirPin,HIGH);
    digitalWrite(dirPin_Y, LOW);
  }
  steps = round(mini_step * abs(angle_to_take));    // Calculates the required number of steps for the movement.
  position = angle;
}

void prepareTimings(int steps){
  /*
   * This function generates two arrays, one for each motor, that contain the timestamps in ms
   * for each of the HIGH and LOW pulses. Apologies for the absolutely terrible naming with "foo"...
   */
  int foo [steps] = {};                       // Clear previous calculated steps
  int double_foo[steps*2] = {};

  for(int i =0; i <= steps;i++){              // Loop through all the steps:
    foo[i] = 2000+500*cos(i/(steps/(2*PI)));  // This is the "S-Curve" equation (not really). It's a tweaked cosine curve. This is NOT good, as it leads to smooth, long motions, yet VERY jerky short motions.
    double_foo[2*i] = foo[i];                 // Mirror the S-Curve to decelerate at the end of the motion.
    double_foo[2*i+1] = foo[i];
  }
  for(int i = 0; i <=steps*2;i++){ // Next we calculate the TIMING of the steps. Since the steps array contains DURATIONS (ie 1200, 1150, 1000...), we need to convert it to timestamps (ie 0, 1200, 2350, 3350...) to use interrupts.
    long sum = 0;
    for(int j = 0;j<=i;j++){
      sum +=double_foo[j];
    }
    timings[i] = sum;
  }
  
  for(int i = 1; i <=steps;i++){ // Now we split our timing array into a HIGH-only and LOW-only array. This is needed to use the logical statements used during motion.
    timings_HIGH[i] = timings[2*i-1];
    timings_LOW[i] = timings[2*i];
  }
  timings_HIGH[0] = 0;
  timings_LOW[0] = timings[0];
}

void runwithTimings(long timings) { 
  /*
   * Function to run the two motors given the previously generated timings arrays.
   */
  position_reached = 0;
  unsigned long elapsedMicros = 0;                          // Start counting ms
  startMicros = micros();                                   // Save the time prior to executing the move.
  while(position_reached != 1){                             // Loop until position is reached
    elapsedMicros = micros() - startMicros;                 // At each loop, calculate the current time.
    for(int i=0;i<=steps;i++){                              // Now loop through each step
      if(c_low>steps){                // c_low is the index of LOW pulses currently sent. If the index exceeds the number of steps, we have for sure reached our position.
        position_reached=1;
        break;
      }
      condition_HIGH_1 = elapsedMicros>=timings_HIGH[i];    // If the current elapsed time is higher than the currently selected timing for HIGH pulses - send a HIGH pulse
      condition_HIGH_2 = i >= c_high;                       
      condition_LOW_1 = elapsedMicros>=timings_LOW[i];      // If the current elapsed time is higher than the currently selected timing for LOW pulses - send a LOW pulse
      condition_LOW_2 = i >= c_low;                         
      
      if(condition_HIGH_1 && condition_HIGH_2) { // This is JANK - I ran out of memory on the mega so the steps need to be taken in sync - only when both motors need to take a step they take a step.
        digitalWrite(stepPin,HIGH);
        digitalWrite(stepPin_Y,HIGH);
        c_high+=1;
        break;
      }
      else if(condition_LOW_1 && condition_LOW_2){
        digitalWrite(stepPin,LOW);
        digitalWrite(stepPin_Y,LOW);
        c_low+=1;
        break;
      }
    }
  }
c_high = 0;
c_low = 0;

}


void setup() {
// Pin Setup:
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(stepPin_Y,OUTPUT); 
  pinMode(dirPin_Y,OUTPUT);
  digitalWrite(dirPin, HIGH);
  digitalWrite(dirPin_Y, HIGH);
  Serial.begin(9600);
}

void loop() {
  Serial.println("Set desired X angle:");   // Get angles from serial and execute movements.
  while (Serial.available() <= 1)   
    { //Wait for user input  } 
    }
  String angleStr = Serial.readString();  
  angle_d = angleStr.toDouble();
//  Serial.println("Set desired Y angle:");
  while (Serial.available() <= 1)   
    { //Wait for user input  } 
    }
  String angleStr_Y = Serial.readString();  
  angle_d_Y = angleStr_Y.toDouble();
  Serial.println(String(angle_d) + "   " + String(angle_d_Y));
  Serial.flush();
  delay(500);
  
}
