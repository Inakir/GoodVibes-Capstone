#include <Time.h>
#include <TimeLib.h>
#include <ephemera_common.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <LiquidCrystal.h>
#include "SimpleTimer.h"
SimpleTimer time_keeper;

//size of the arrays that hold our data
const int data_size = 100;

//to avoid divisions in the code I multipy this constant
double divider = 1/(double)data_size;

//sampling variables
const int sample_time_size = 2000;
int sampling_rate = sample_time_size/data_size;

//array to hold the buzzer pins
uint8_t buzzers[6] = {9, 10, 11, 12, 13, 8};

//array that holds the inputs at each iteration
double incomingAudio[6] = {0,0,0,0,0,0};

//holds the data that is used to calculate the averages and standard deviations for the code
double data[6][data_size];

//holds the spot that is being sampled in the arrays
int spot[6] = {0,0,0,0,0,0};

//holds the values used to analyze data
double sum[6] = {0,0,0,0,0,0};
double average[6] = {0,0,0,0,0,0};
double varience[6] = {0,0,0,0,0,0};
double std_dev[6] = {0,0,0,0,0,0};

//when outputing vibrating feedback, used to ensure microphones are next to each other
int adj[2] = {-2,-2};

//used when a sound has been picked up and needs to be represented by the buzzers
bool split = false;

//counts how many periods a buzzer has been going off
int periods = 0;

//holds the position of the buzzers that are going to go off
int spot1 = -1;
int spot2 = -1;

//used to alternate between two buzzers going off
int alt = 0;

//keeps track of time
unsigned long timer;

//keeps track of battery
int val = 0;
int new_val=0;

//used to check adjacent buzzers
bool adj0less;
bool adj1less;


//sampling function, when called the values read into the microphones are used to update averages and std_dev
void sample()
{
    //if spot1 and spot2 are offset, synchornizes them
    if(spot1!=-1 && spot2==-1)
    {
      spot1=-1;
    }

  //goes through all the mics
     for(int i=0; i<6; i++)
     {
      //updates microphone values average, sum, and varience without having to traverse the entire list
      //by subtracting the portion of the previous entry and adding the new entry in 
      //start---------------------
       
       sum[i] -= data[i][spot[i]];
       varience[i] -= (data[i][spot[i]] - average[i])*(data[i][spot[i]] - average[i]);

       
       data[i][spot[i]] = incomingAudio[i];
       
       sum[i] += data[i][spot[i]];
       average[i] = sum[i]*divider; 
       varience[i]+=(data[i][spot[i]] - average[i])*(data[i][spot[i]] - average[i]);
       
       //finish-------------------

       //updates the spot
       spot[i] = spot[i]+1;
       if(spot[i] >= data_size) {
         spot[i] = 0;
       }
      
       //recalculates std_dev
       std_dev[i] = sqrt(varience[i]*divider);
       
       //fifty is our minimum value that std_dev can be to ensure proper output
       //if(std_dev[i]<80) accidentally ran main demo with this line ooops
       if(std_dev[i]<50) {
        //Serial.print(std_dev[i]); Serial.print("\n");
          std_dev[i] = 50;
       }
     }
}


LiquidCrystal lcd(42, 44, 46, 48, 50, 52); //these pins can be changed but we will need to figure out what goes where
//Call this function to update the LCD screen with the current battery charge
void battery_check(){
  val = analogRead(A15);
  lcd.clear();
  lcd.setCursor(0,0);
  new_val= val - 699;
  val = (new_val/290)*100;
  lcd.print(val);
}

//inits everything and sets up pins
void setup() {
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  
  pinMode(buzzers[0], OUTPUT);
  pinMode(buzzers[1], OUTPUT);
  pinMode(buzzers[2], OUTPUT);
  pinMode(buzzers[3], OUTPUT);
  pinMode(buzzers[4], OUTPUT);
  pinMode(buzzers[5], OUTPUT);
  //battery_check();
 
// battery code
  lcd.begin(16,2);
  pinMode(A15, INPUT);
  battery_check();
  //lcd.print("hi");
//

  Serial.begin(230400);

  //samples at the given rate we wanted to
  time_keeper.setInterval(sampling_rate, sample);
  
  //turn below off if getting buggy, sometimes does, used to check battery
  time_keeper.setInterval(600000, battery_check);

//initializes data
  for(int i = 0; i < 6; i++) {
     for(int j = 0; j < data_size; j++) {
        data[i][j] = 0;
    }
  }
}

void loop() {
   //makes sure sample and battery code is ran at the interrupt times
   time_keeper.run();

   //reads in the values at the pins at every iteration
   incomingAudio[0] = analogRead(A1);
   incomingAudio[1] = analogRead(A2);
   incomingAudio[2] = analogRead(A3); 
   incomingAudio[3] = analogRead(A4);
   incomingAudio[4] = analogRead(A5);
   incomingAudio[5] = analogRead(A6);

   //loop starts at three because mics 3 and 2 are the least important since they are in the front
   //and the first and last to be checked have the largest chance of having sound not being picked up
   //as adjecent
   int i = 3;

   //traverses loop from 3-5 then 0-2;
   while(i!=2)
   {
     //checks to see if sound is strange
     if((incomingAudio[i] > average[i] + std_dev[i] || incomingAudio[i] < average[i] - std_dev[i])&&!split) {

      /*Serial.print("\nAverage: "); Serial.print(average[i]); 
      //Serial.print("\nStd. Dev: "); Serial.print(std_dev[i]);
      Serial.print("mic:  "); Serial.print(i+1); 
      Serial.print(" audio: "); Serial.print(incomingAudio[i]);
      Serial.print(" ----------------------------------------------Spike");
      Serial.print("\n\n");*/

      //if sound is strange, records the microphone that picked it up first
      if(spot1 == -1) {
        spot1 = i;
      }

      //records second mic to pic up strange sound
      else if(spot2 == -1 && spot1 != i) {

        //sometimes the second closest microphone won't pick up the sound second due to computing overhead
        //that happens after first sound is picked, code below analyes the microphone that pics up the second 
        //sound and normalizes it to a microphone adjecent to the first one in the direction closest to the
        //second one that picked it up
        
        adj[0] = spot1-1; if(adj[0]==-1) {adj[0] = 5;}
        adj[1] = spot1+1; if(adj[0]==6) {adj[0] = 0;}
        
        adj0less = (adj[0]-i)*(adj[0]-i) < (adj[1]-i)*(adj[1]-i);
        adj1less = (adj[0]-i)*(adj[0]-i) > (adj[1]-i)*(adj[1]-i);
        
        if(adj0less)      { spot2 = adj[0]; }
        else if(adj1less) { spot2 = adj[1]; }
        else              { spot2 = spot1;  }

       // Serial.print("BUZZ:  "); Serial.print(spot1+1); Serial.print(" audio: "); Serial.print(1); Serial.print("\n");
       // Serial.print("BUZZ:  "); Serial.print(spot2+1); Serial.print(" audio: "); Serial.print(.5); Serial.print("\n");

        //buzzing output can now happen
        split = true;
      }
    }
    i++;
    if(i==6) { i = 0; }
   }

   //if an abornal sound has been detected, only runs once each loop so that data can continue being sampled while buzzers are going off
   if(split) {

    //following two lines write out the intensities the buzzers vibrate at
      if(alt == 1) {
        digitalWrite(buzzers[spot2], LOW);
        digitalWrite(buzzers[spot1], HIGH);
      }
      else {
        digitalWrite(buzzers[spot1], LOW);
        alt = 0;
        digitalWrite(buzzers[spot2], HIGH);
      }
      
      alt++;

     //counts periods
      periods++;

      //after certain periods, renormalizes and shuts buzzers off
      if(periods == 500) {
        digitalWrite(buzzers[spot1], LOW);
        digitalWrite(buzzers[spot2], LOW);
        
        periods = 0;
        spot1 = -1;
        spot2 = -1;
        alt = 0;
        
        split = false;
      
      }
   }
}
