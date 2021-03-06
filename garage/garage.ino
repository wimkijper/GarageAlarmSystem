// Copyright Eddie Espinal 2016
/* @file garage.pde
 || @version 1.0
 || @author Eddie Espinal
 || @contact eddieespinal@gmail.com
 ||
 || @description
 || | Garage System using the Arduino to prevent the garage from staying opened over night.
 || #
 */
 
#include "Wire.h"
#include "Arduino.h"
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <inttypes.h>

// This is needed to fix a bug on LCD that was only displaying one character
#define printIIC(args) Wire.write(args)
inline size_t LiquidCrystal_I2C::write(uint8_t value) { 
  send(value, Rs);
  return 1;
}

DS3231 clock;
RTCDateTime dt;

// Trigger Time
const byte triggerHour = 23; //11:00 AM (24hr format)
const byte triggerMinute = 30;
const String triggerAMPM = "PM";
const unsigned int sirenDuration = 10000; // 10 Seconds

// Pins Setup
const int sirenPin = 2;
const int door1SensorPin = 5; // My garage door
const int door2SensorPin = 4; // My wife's garage door

const int relay1Pin = 6;
const int relay2Pin = 7;
const int relayTimeDelay = 10000; // delay in ms for on and off phases

boolean sirenState = false;
boolean door1SensorState = false;
boolean door2SensorState = false;
long previousMillis = 0;
long interval = 2*60*1000; // 2 minute interval to wait before we can ring the siren again.

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void triggerSiren() {
  if (sirenState == false) {
    digitalWrite(sirenPin, HIGH);
    sirenState = true;
  } else {
    digitalWrite(sirenPin, LOW);
    sirenState = false;
  }
}

void triggerRelay(int relayPin) {

  digitalWrite(relayPin, HIGH);  // turn the relay on
  delay(relayTimeDelay);              // wait for one second

  digitalWrite(relayPin, LOW);   // turn the relay off
  delay(relayTimeDelay);              // wait for one second
}

void displayDate(){
  dt = clock.getDateTime();

  lcd.setCursor(0,0);
  lcd.print(clock.dateFormat("m/d/y h:iA", dt));

  lcd.setCursor(0,1);

  // Display the door status on the LCD if we detect one of the doors opened.
  if (door1SensorState == true || door2SensorState == true) {
    String door1Status = (door1SensorState) ? "OPN" : "CLD";
    String door2Status = (door2SensorState) ? "OPN" : "CLD";
    lcd.print("D1:" + door1Status + "    D2:" + door2Status);
  } else {
    String alarmString = "ALARM: " + String(triggerHour-12) + ":" + String(triggerMinute) + " " + triggerAMPM + " ";
    lcd.print(alarmString);
  }
}

void displayDoorStatus(){

  lcd.clear();
  
  String door1Status = (door1SensorState) ? "OPN" : "CLD";
  String door2Status = (door2SensorState) ? "OPN" : "CLD";
  
  lcd.setCursor(0,0);
  lcd.print("WARNING CHK DOOR");

  lcd.setCursor(0,1);
  lcd.print("D1:" + door1Status + "    D2:" + door2Status);
}

void checkSensorState() {

  // Read Door #1 Sensor
  if(digitalRead(door1SensorPin) == HIGH){
    door1SensorState = true;
  } else {
    door1SensorState = false;
  }

  Serial.println("Sensor #1 State: " + String(door1SensorState));

  // Read Door #2 Sensor
  if(digitalRead(door2SensorPin) == HIGH){
    door2SensorState = true;
  } else {
    door2SensorState = false;
  }

  Serial.println("Sensor #2 State: " + String(door2SensorState));

  delay(100);
}

void checkGarageStatus() {

  dt = clock.getDateTime();
  char *am_pm = clock.dateFormat("A", dt);

  if (dt.hour == triggerHour && dt.minute == int(triggerMinute)) {
    
    Serial.println(clock.dateFormat("m/d/y h:iA", dt));
    
    unsigned long currentMillis = millis();

    // Let's check if one of the doors are opened and trigger the Siren if they are.
    if (door1SensorState == true || door2SensorState == true) {
        if(currentMillis - previousMillis >= interval || previousMillis == 0) {
          // save the last time the siren went ON 
          previousMillis = currentMillis; 

          // Turn Siren ON
          triggerSiren();

          // Change the display to show which door is opened or closed
          displayDoorStatus();
          
          delay(sirenDuration);

          // Turn Siren OFF
          triggerSiren();

          // Clear the LCD to get it ready to display the date/time again.
          lcd.clear();
        }
    }
  }
}

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  
  // Magnetic Contact Switches
  pinMode(door1SensorPin, INPUT);
  pinMode(door2SensorPin, INPUT);

  digitalWrite(door1SensorPin, HIGH);  // Turn on the internal pull-up resistor
  digitalWrite(door2SensorPin, HIGH);  // Turn on the internal pull-up resistor
  
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(sirenPin, OUTPUT);

  // Initialize the lcd 
  lcd.init();                      
  lcd.backlight();
  lcd.clear();
  
  lcd.setCursor(2,0); 
  lcd.print("GARAGE SYSTEM");

  lcd.setCursor(0,1);
  lcd.print("BY:EDDIE ESPINAL");
  delay(3000);
  
  lcd.clear();
}

void loop()
{
  displayDate();
  checkSensorState();
  checkGarageStatus();
  
}

