/* 
 *  
 *  Created by Drew Dunne
 *  on Aug 3, 2016
 *  Copyright © 2016 Drew Dunne
 *  
 *  Feel free to reuse this code, 
 *  just give credit where needed
 *  
 */

// Libraries for BLE Shield
#include <SPI.h>
#include <EEPROM.h>
#include <boards.h>
#include <RBL_nRF8001.h>

// Libraries for screen and keypad
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>

// Change this accordingly
#define PasswordLength 4

/*
 * Password management
 * Make sure master is the same length
 * as PasswordLength.
 */
char input[PasswordLength] = "";
char master[] = "22B4";
byte data_count = 0;

const byte Rows = 4;
const byte Cols = 4;

char keymap[Rows][Cols] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rPins[Rows] = { 26, 28, 30, 32};
byte cPins[Cols] = { 34, 36, 38, 40};

Keypad kpd = Keypad(makeKeymap(keymap), rPins, cPins, Rows, Cols);

LiquidCrystal_I2C lcd(0x27,20,4);

byte launchPin = 53; // The pin the launch setup is in
byte powerPin = 51; // Shows the system is on
byte activePin = 49; // The launch signal LED

bool codeCorrect = false;

void setup()
{  
  pinMode(launchPin, OUTPUT);
  pinMode(powerPin, OUTPUT);
  pinMode(activePin, OUTPUT);
  digitalWrite(activePin, LOW);
  digitalWrite(powerPin, HIGH);
  // Default pins set to 9 and 8 for REQN and RDYN
  // Set your REQN and RDYN here before ble_begin() if you need
  //ble_set_pins(3, 2);
  
  // Set your BLE Shield name here, max. length 10
  ble_set_name("Launcher");
  
  // Init. and start BLE library.
  ble_begin();
  
  // Enable serial debug
  Serial.begin(57600);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Enter Launch Code:");
  lcd.setCursor(0,1);
  clearInput();
}

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;

void loop()
{
  if (stringComplete && inputString == "launch();") {
    // Check if serial commmanded launch (for testing)
    Serial.println("Serial Launch Activated");
    launch();
    stringComplete = false;
    inputString = "";
  }

  if (codeCorrect) {
    // Wait for bluetooth launch signal
    if ( ble_available() ) {
      String bleCommand = "";
      while ( ble_available() ) {
        // Read the input from BLE
        char inChar = (char)ble_read();
        bleCommand += inChar;
        if (inChar == ';') {
          // End of line
          if (bleCommand == "launch();") {
            // Launch Command from BLE
            Serial.println("BLE Launch Activated");
            launch();

            // Send response
            unsigned char data[2];
            data[0] = 'L';
            data[1] = 'S';
            ble_write_bytes(data,2);
          } else if (bleCommand == "lock();") {
            // Lock Command from BLE
            Serial.println("BLE Lock Activated");
            lock();

            // Send response
            unsigned char data[2];
            data[0] = 'D';
            data[1] = 'S';
            ble_write_bytes(data,2);
          } else {
            // Incorrect command
            // Send response
            unsigned char data[1];
            data[0] = 'F';
            ble_write_bytes(data,1);
          }
        }
      } 
      Serial.println();
    }
    ble_do_events();
  } else {
    // Need code
    char keypressed = kpd.getKey();
    if (keypressed != NO_KEY) {
      if (data_count == 0) {
        lcd.setCursor(0,1);
        lcd.print("    ");
        lcd.setCursor(0,1);
      }
      Serial.print(keypressed);
//      lcd.print(keypressed);
      lcd.print("*");
      input[data_count] = keypressed;
      data_count++;
      if (data_count == PasswordLength) {
        Serial.println();
        Serial.println(input);
        Serial.println(master);
        if (!strcmp(input, master)) {
          // correct code entered!
          Serial.println("Correct");
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Correct");
          lcd.setCursor(0,1);
          lcd.print("Waiting for launch");
          codeCorrect = true;
          digitalWrite(activePin, HIGH);
          clearInput();
        } else {
          //incorrect
          Serial.println("Wrong code entered");
          lcd.setCursor(0,2);
          lcd.print("Incorrect");
          clearInput();
          codeCorrect = false;
        }
      }
    }
  }
}

void launch() {
  digitalWrite(activePin, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("RCVD Launch Signal");
  lcd.setCursor(0,1);
  lcd.print("Launching in:");
  for (int i = 10; i > 0; i--) {
    Serial.println(i);
    lcd.setCursor(9,2);
    if (i == 10) {
      lcd.print(i);
    } else {
      lcd.print("0");
      lcd.print(i);
    }
    delay(900);
    digitalWrite(activePin, LOW);
    delay(100);
    digitalWrite(activePin, HIGH);
  }
  lcd.setCursor(5,2);
  lcd.print("Launching!");
  digitalWrite(launchPin, HIGH);
  delay(1000);
  digitalWrite(launchPin, LOW);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Launch Successful");
  lcd.setCursor(0,1);
  lcd.print("Resetting");
  delay(5000);
  lock();
}

void lock() {
  codeCorrect = false;
  clearInput();
  digitalWrite(activePin, LOW);
  lcd.clear();
  lcd.print("Enter Launch Code:");
  lcd.setCursor(0,1);
}

// Clears keypad input
void clearInput() {
  while (data_count != 0) {
    input[data_count--] = 0;
  }
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    Serial.println(inputString);
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == ';') {
      stringComplete = true;
    }
  }
}

