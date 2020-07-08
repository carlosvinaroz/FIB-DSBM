#include <LiquidCrystal.h>
#include <Keypad.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

unsigned long time1=0;
unsigned long time2=0;

//LCD
const int rs = 4;
const int rv = 3;
const int en = 2;
const int D0 = 22;
const int D1 = 23;
const int D2 = 24;
const int D3 = 25;
const int D4 = 26;
const int D5 = 27;
const int D6 = 28;
const int D7 = 29;

LiquidCrystal lcd(rs,rv,en,D0,D1,D2,D3,D4,D5,D6,D7);

//BUTTONS
const int button_start=30;
const int button_manual=31;
const int button_auto=32;
const int button_stop=33;
const int button_emergency=34;

//PRESENCE
const int presence_trigger=13;
const int presence_echo=12;

//KEYPAD
char keys[4][3]={
 {'1','2','3'},
 {'4','5','6'},
 {'7','8','9'},
 {'*','0','#'}};
 
byte rowPin[4]={38,39,40,41};
byte colPin[3]={37,36,35};

Keypad keypad=Keypad(makeKeymap(keys),rowPin,colPin,4,3);

//TEMPERATURE
#define ONE_WIRE_BUS 42  //pin 2 of arduino
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//PULSE HEART
int HBperMin = 0;
int HBMax= 0;
int HBMin = 205;

// TIMER
int TimeinSec=0;

// MOTOR
const int pinA0 = A0;





//VARIABLES
int distanceCm;
//String age[2];
int peso;
int pos_key;
int kcal;
boolean manual_pressed;
boolean auto_pressed;
const int MIN=256;
int velocity;
int velocityMax;
String state; 
String sam;
int temp;                  
void setup() 
{
  
  //RT & TX
  Serial.begin(9600);
  //analogReference(DEFAULT);
  //VARIABLES
  temp=0;
  pos_key=0;
  peso=0; 
  velocity=0;  
  velocityMax=0;
  kcal=0;
  manual_pressed=false;
  auto_pressed=false;
  
  //LCD
  state="STANDBY";
  lcd.begin(40,2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("State:");
  lcd.setCursor(0,1);
  lcd.print(state);
  delay(100);
  //PRESENCE SENSOR
  pinMode(presence_trigger, OUTPUT);
  pinMode(presence_echo, INPUT);
  //BUTTONS
  pinMode(button_start,INPUT);
  pinMode(button_manual,INPUT);
  pinMode(button_auto, INPUT);
  pinMode(button_stop, INPUT);
  pinMode(button_emergency, INPUT); 
  //TEMPERATURE
  sensors.begin(); 
  // PULSE HEART

  // MOTOR DC
  pinMode(51,OUTPUT); 
  pinMode(A0,INPUT);
  pinMode(A1,INPUT); 
  // TIMER
  Timer1.initialize(1000000); 

}

void loop()
{  
  // DISPLAY STATE EVERY TIME
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("State:");
   lcd.setCursor(0,1);
   lcd.print(state);  
   delay(50);
   
  //EXCEPTIONS
  presence_distance();
    
  //STANDBY  
  if(state=="STANDBY"){
    standby();
    time1= millis();
  }
  
  //IDENTIFICATION
  else if(state=="IDENTIFICATION"){
    identification();    
    if(state=="SELECT MODE"){
      lcd.clear();
      lcd.setCursor(20,0);
      lcd.print("Weight introduced:");
      lcd.setCursor(20,1);
      lcd.print(peso);
      delay(1500);
      time1=millis();
      
    }
  }

  //SELECT MODE
  else if(state=="SELECT MODE"){
    select_mode();
    if(state=="TRAINING SEASON"){
      TimeinSec=0;
      Timer1.attachInterrupt( timerIsr );
    }
  }

  //TRAINING SEASON
  else if(state=="TRAINING SEASON"){
    //DISPLAY ALL DATA
      lcd.clear();
      //Temp                  
        lcd.setCursor(0,0);    
        lcd.print(temp); 
        lcd.print(" Grados");           
       //Pulse Heart
        lcd.setCursor(0,1);
        lcd.print("Pulse Heart: ");
        lcd.print(HBperMin);
       //Velocity
        lcd.setCursor(20,0);
        lcd.print("Velocity: ");
        lcd.print(velocity);
        analogWrite(51,velocity);          
       //Time Season
       
       //Wait 1s for display all
        delay(1000);
       
          
    //STOP => No presence or Emeregency or Stop and velocity=MIN
    //PRESENCE SENSOR, MOTOR, velocity=weight y pulse heart si AUTO o potentiometre sensor si MANUAL
    if(distanceCm >= 1000 or !digitalRead(button_emergency) or (!digitalRead(button_stop) and velocity<257) ){
      state="STOP";
    }
    
    //PULSE HEART
    int hb = analogRead(A2);
    HBperMin= hb/5;
    if(HBperMin > HBMax){
      HBMax = HBperMin; 
    }
    else if(HBperMin < HBMin){
      HBMin = HBperMin;
    }
  
    //TEMPERATURE
      temp = analogRead(A1);
      float mv = (temp/1024.0)*500;
      temp = int(mv);   
      delay(50);
    // MOTOR
      // AUTO
        if(auto_pressed){
          velocity = (HBperMin*5 + 0.25*peso); 
        }
      // MANUAL
        if(manual_pressed){
          int velManual=analogRead(A0);
          velocity=(velManual+0.25*peso);   
          if(velocity>velocityMax){
            velocityMax=velocity;
          }
          delay(5);    
         } 
  }

  else if(state=="STOP"){
    //Stop Timer
    Timer1.detachInterrupt();
    //Stop Motor
    velocity=0;
    analogWrite(51,velocity); 
    state="DOWNLOAD";
    delay(1000);
    time1=millis();
    }

   else if(state=="DOWNLOAD"){
    // Calculo kcal
    kcal = (((velocityMax-1)/100)*0.01925*peso)*(TimeinSec/60);
    // pasar velocityMax, TimeinSec, HBperMin max, HPperMin min, kcal BLUETOOTH
     if(Serial.available() >= 0){
        Serial.print("Receiving data...");
        Serial.println();
        Serial.print("Velocity Max: ");
        Serial.print(velocityMax);
        Serial.println();
        Serial.print("Time: ");
        Serial.print(TimeinSec);
        Serial.println();
        Serial.print("HBCount Max/Min: ");
        Serial.print(HBMax);
        Serial.print(" / ");
        Serial.print(HBMin);
        Serial.println();
        Serial.print("kcal: ");
        Serial.print(kcal);
        Serial.println();
        Serial.print("DATA RECEIVED");
        delay(1000);          
        } 
    //download finish
    //Time expired 30s
    time2=millis();
    if(time2-time1){
      state="STANDBY";
    }
   }
  
  
}

void presence_distance(){ 
  digitalWrite(presence_trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(presence_trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(presence_trigger, LOW);
  long duration = pulseIn(presence_echo, HIGH);
  distanceCm= duration*0.034/2;                                                                  
  delay(10);
}

void standby(){
  //If START & Presence < 1m then go to IDENTIFICATION
  if(!digitalRead(button_start) and distanceCm < 1000){    
  state="IDENTIFICATION";
  reset();
  }  
}

void reset(){
  pos_key=0;
  peso=0;  
  time1=0;
  time2=0;
  manual_pressed=false;
  auto_pressed=false;
}

void identification(){
  lcd.clear();
  lcd.setCursor(18,0);
  lcd.print("Weight:");     
  delay(50);
 // TIME EXPIRED (15S), come back to STANDBY
  time2 = millis(); 
  if((time2-time1) >= 15000){
    state="STANDBY";    
  }   
  // KEYPAD
  char key = keypad.getKey();
  delay(10);
  if(key and pos_key<2){
    time1 = millis();
    if(pos_key==1){
      peso+=int(key)-48;
      state="SELECT MODE";
    }
    else{
      peso+=(int(key)-48)*10;
    }
    ++pos_key;
    lcd.setCursor(18,1);
    lcd.write(key);
    delay(1000);
   }    
}

void select_mode(){
    time2=millis();
    lcd.clear();
    // BUTTONS
    if(!digitalRead(button_auto)){
      auto_pressed=true;
      lcd.setCursor(18,0);
      lcd.write("AUTO");
      delay(1000);
      state="TRAINING SEASON";
    }
    if(!digitalRead(button_manual)){
      manual_pressed=true;
      lcd.setCursor(18,0);
      lcd.write("MANUAL");
      delay(1000);
      state="TRAINING SEASON";
    }
    if((time2-time1) >= 15000){
      state="STANDBY";    
    }  
}

void timerIsr(){
      TimeinSec = TimeinSec + 1;
        lcd.setCursor(20,1);
        lcd.print("Time: ");
        lcd.print(TimeinSec);
}
