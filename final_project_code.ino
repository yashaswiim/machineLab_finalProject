#include <Servo.h>

// defining all the connected pin numbers
const int TRIGGER_PIN = 5;
const int DONE_PIN = 4;
const int DIVE_PIN = 9;
const int PREY_PIN = 10;
const int FLAP_PIN1 = 11;
const int FLAP_PIN2 = 12;
const int IN1 = 2;
const int IN2 = 3;
const int IN3 = 6;
const int IN4 = 7;

// defining constants for the diving and prey mechanisms (maximum and minimum angles for the motors)
const int MAX_DIVE = 90;
const int MAX_PREY = 180;
const int MIN_DIVE = 50;
const int MIN_PREY = 0;

//defining other variables used
int diveAngle = MIN_DIVE; //current angle for the diving motor
int preyAngle = MIN_PREY; //current angle for the prey mechanism motor
int trigger = 0; //flag for incoming signal
int dived = 0; //flag to mark if the diving mechanism has already happened after receiving signal, since it is supposed to happen only once
int timeToSendSignal = 0; //flag for sending back signal once done with mechanisms
unsigned long lastCheckedTime = 0; //to take timestamps for delaying/waiting purposes
unsigned long lastStepperTime = 0; //to take timestampsfor delaying/waiting purposes for the stepper motor i.e. background mechanism

//keep track of the trigger state and take only the first HIGH into consideration 
int lastTriggerState;                  
int currentTriggerState;

//servo motors
Servo dive_servo;
Servo prey_servo;

//start: code for the stepper motor
void step0(){
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void step1(){
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void step2(){
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void step3(){
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void oneCycle(int stepDelay){
  if(millis() - lastStepperTime >= 4*stepDelay){
    step3();
    lastStepperTime=millis();
  }
  else if(millis() - lastStepperTime >= 3*stepDelay){
    step2();
  }
  else if(millis() - lastStepperTime >= 2*stepDelay){
    step1();
  }
  else if(millis() - lastStepperTime >= stepDelay){
    step0();
  }
}
//end: code for the stepper motor

void setup() {
  Serial.begin(9600);

  //initiate the motors
  dive_servo.attach(DIVE_PIN);
  prey_servo.attach(PREY_PIN);
  dive_servo.write(diveAngle);
  prey_servo.write(preyAngle);

  //set up the pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(FLAP_PIN1, OUTPUT);
  pinMode(FLAP_PIN2, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(DONE_PIN, OUTPUT);

  lastStepperTime = millis();
  currentTriggerState = digitalRead(TRIGGER_PIN);
}

void loop() {
  lastTriggerState = currentTriggerState;
  currentTriggerState = digitalRead(TRIGGER_PIN);
  //take only the first HIGH into consideration
  if(lastTriggerState == LOW && currentTriggerState == HIGH){
    trigger = 1;
    lastCheckedTime = millis();
  }
  if(trigger == 1){
    //start background and flapping bird
    oneCycle(3);
    analogWrite(FLAP_PIN1, 170);
    digitalWrite(FLAP_PIN2, LOW);
    
    if(diveAngle == MIN_DIVE && !dived){
      //wait 5 seconds before starting diving bird
      if(millis() - lastCheckedTime > 5000){
        Serial.println("time to dive");
        diveAngle = MAX_DIVE;
        dive_servo.write(diveAngle);
        dived = 1;
        lastCheckedTime = millis();
      }
    }
    else if(diveAngle == MAX_DIVE){
      //wait 3 seconds before diving bird starts coming back up
      if(millis() - lastCheckedTime > 3000){
        lastCheckedTime = millis();
        diveAngle = diveAngle - 1;
        dive_servo.write(diveAngle);
      }
      //once the diving bird reaches its lowest point, wait 1 second to pop the prey out
      else if(millis() - lastCheckedTime > 1000){
        preyAngle = MAX_PREY;
        prey_servo.write(preyAngle);
      }
    }
    //decrease the angle of the diving bird by 1 degree evere 100ms
    else if (millis() - lastCheckedTime > 100){
      if (diveAngle > MIN_DIVE){
        lastCheckedTime = millis();
        diveAngle = diveAngle - 1;
        dive_servo.write(diveAngle);
      }
      if(diveAngle == MIN_DIVE){
        preyAngle = MIN_PREY;
        prey_servo.write(preyAngle);
        //once the diving bird reaches back up, wait 5 seconds to stop the background and flapping bird
        if(millis() - lastCheckedTime > 5000){
          trigger = 0;
          dived = 0;
          timeToSendSignal = 1;
          lastCheckedTime = millis();
        }
      }
    }
  }
  else{
    analogWrite(FLAP_PIN1, 0);
    digitalWrite(FLAP_PIN2, LOW);
    //if done with the mechanisms, send HIGH signal for 2 seconds
    if (timeToSendSignal == 1){
      if(millis() - lastCheckedTime < 200){
        digitalWrite(DONE_PIN, HIGH);
      }
      else{
        digitalWrite(DONE_PIN, LOW);
        timeToSendSignal = 0;
      }
    }
  }
}
