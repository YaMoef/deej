#include <Wire.h>              //include the I²C library including it before the lcd library since it might happen that a normal lcd gets used (non I²C)
#include <LiquidCrystal_I2C.h> //library for an i2C display, can be installed from the arduino library manager
// #include <LiquidCrystal.h>//library for an display, can be installed from the arduino library manager
#include <RotaryEncoder.h> //library for the rotarty encoder, can be installed from the arduino library manager
#include <EEPROM.h>        //library to use the eeprom

LiquidCrystal_I2C lcd(0x27, 16, 2); // when using a I2C LCD
// if the I2C lcd is not working, check you connection and the address above (0x27);
// LiquidCrystal lcd(12, 10, 5, 4, 3, 2);  //when using a normal LCD

// you can tweak following values for you needs

// Please read the README
const int useEEPROM = 0; // 0: use external eeprom
                         // 1: use internal eeprom
const int PIN_ENCODER_A = 8;
const int PIN_ENCODER_B = 9;
const int MULTIPLIER = 5;
const int STEPSIZE = 1;
const unsigned long PAUSELENGTH = 150000;

const int SW = 7;
const int amountSliders = 7; // amount of sliders you want, also name them in the array below
const String sliderNames[amountSliders] = {
    "Master",
    "Music",
    "Browser",
    "Communication",
    "Games",
    "Teams",
    "Mic",
};
const int I2CAddress = 0x50;                                           // most common EEPROM module I²C address
int displayValue[amountSliders] = {100, 100, 100, 100, 100, 100, 100}; // start values for every slider

// leave following values at their default
RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::FOUR3);
bool prev_a = false;
bool prev_b = false;
unsigned long lastENCreadTime = micros();
int ENCcounter = 0;
int lastENCread = 0;
enum
{
  extEEPROM = 0,
  intEEPROM = 1
};
int previousValue[amountSliders] = {100, 100, 100, 100, 100, 100, 100}; // extra values to see if it changed compared to last cycle
int sliderNumber = 0;                                                   // variable which numbers all the sliders
unsigned long lastButtonPress = 0;
bool singleButtonPress = false;
int state = 0; // state 0 is the menu screen to select what you want to change
               // state 1 is the screen where you change the value itself
enum
{
  menuScreen = 0,
  valueScreen = 1
};
int EEPROMvalue = 0; // value to read in value from eeprom or to save them temporarly
byte reading = 0;
byte arrow[8] = { // byte for creating an arrow on the lcd screen
    B11000,
    B11100,
    B11110,
    B11111,
    B11110,
    B11100,
    B11000,
    B00000};
const int amountIndicator = 7;
byte indicator[amountIndicator][8] = {
    {B11111,
     B10001,
     B10001,
     B10001,
     B10001,
     B10001,
     B10001,
     B11111},
    {B11111,
     B10001,
     B10001,
     B10001,
     B10001,
     B10001,
     B11111,
     B11111},
    {B11111,
     B10001,
     B10001,
     B10001,
     B10001,
     B11111,
     B11111,
     B11111},
    {B11111,
     B10001,
     B10001,
     B10001,
     B11111,
     B11111,
     B11111,
     B11111},
    {B11111,
     B10001,
     B10001,
     B11111,
     B11111,
     B11111,
     B11111,
     B11111},
    {B11111,
     B10001,
     B11111,
     B11111,
     B11111,
     B11111,
     B11111,
     B11111},
    {B11111,
     B11111,
     B11111,
     B11111,
     B11111,
     B11111,
     B11111,
     B11111}};

void setup()
{
  pinMode(SW, INPUT_PULLUP);
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  prev_a = digitalRead(PIN_ENCODER_A);
  prev_b = digitalRead(PIN_ENCODER_B);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, arrow);
  for (int i = 0; i < amountIndicator; i++)
  {
    lcd.createChar(i + 1, indicator[i]);
  }
  lcd.home();
  Serial.begin(9600);
  while (!Serial)
  {
    // wait for Serial connection (or wait for deej to be runned in the background)
  }
  if (useEEPROM == extEEPROM)
  {
    Wire.begin();                       // join the I²C bus as master
    Wire.beginTransmission(I2CAddress); // start writing to module
    Wire.write(0x00);                   // first byte of word
    Wire.write(0x00);                   // second byte of word so we need byte 0x0000 of the EEPROM
    Wire.endTransmission();
    Wire.requestFrom(I2CAddress, amountSliders); // start reading bytes
    for (int i = 0; i < amountSliders; i++)
    { // reading values out of the eeprom
      while (!Wire.available())
      {
      }
      EEPROMvalue = Wire.read();
      if ((EEPROMvalue != 0) && (EEPROMvalue <= 100))
      {
        displayValue[i] = EEPROMvalue;
      }
      previousValue[i] = displayValue[i];
    }
  }
  else if (useEEPROM == intEEPROM)
  {
    for (int i = 0; i < amountSliders; i++)
    { // reading values out of the eeprom
      EEPROMvalue = EEPROM.read(i);
      if ((EEPROMvalue != 0) && (EEPROMvalue <= 100))
      {
        displayValue[i] = EEPROMvalue;
      }
      else
      {
        EEPROM.write(i, 100); // if the values is not correct write 100 to it
      }
      previousValue[i] = displayValue[i];
    }
  }
}

void loop()
{
  bool cur_a = digitalRead(PIN_ENCODER_A);
  bool cur_b = digitalRead(PIN_ENCODER_B);
  if (prev_a != cur_a || prev_b != cur_b)
  {
    encoder.tick();
    //    Serial.println("tick");
  }
  prev_a = cur_a;
  prev_b = cur_b;
  RotaryEncoder::Direction direction = encoder.getDirection(); // get direction from encoder
  if (direction != RotaryEncoder::Direction::NOROTATION)
  { // do something if there is a rotation
    // Serial.println("turning");
    if (direction == RotaryEncoder::Direction::CLOCKWISE)
    { // direction is CW
      //  Serial.println("rotateRight");
      int turnValue = calculateTurnValue(1);
      Serial.print("Turning right with value ");
      Serial.println(turnValue);
      // int turnValue = calculateTurnValue(RotaryEncoder::Direction::CLOCKWISE)
      RotateRight(turnValue);
    }
    if (direction == RotaryEncoder::Direction::COUNTERCLOCKWISE)
    { // direction is CCW
      //  Serial.println("rotateLeft");
      // int turnValue = calculateTurnValue(RotaryEncoder::Direction::COUNTERCLOCKWISE)
      int turnValue = calculateTurnValue(-1);
      Serial.print("Turning left with value ");
      Serial.println(turnValue);
      RotateLeft(turnValue);
    }
  }

  // Read the button state
  int btnState = digitalRead(SW);

  // If we detect LOW signal, button is pressed
  if (btnState == LOW)
  {
    // if 50ms have passed since last LOW pulse, it means that the
    // button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50)
    {
      ButtonPress();
      // Serial.println("buttonPress");
    }

    // Remember last button press event
    lastButtonPress = millis();
  }

  // Put in a slight delay to help debounce the reading
  delay(1);
}

void UpdateLCD()
{
  lcd.clear();
  if (state == menuScreen)
  {
    lcd.setCursor(0, 0); // put sliders names on screen
    lcd.write(0);
    lcd.print(sliderNames[sliderNumber]);
    lcd.setCursor(15, 0);
    lcd.write(sliderToIndicator(displayValue[sliderNumber]));
    lcd.setCursor(1, 1);
    if (sliderNumber >= (amountSliders - 1))
    {
      lcd.print(sliderNames[0]);
      lcd.setCursor(15, 1);
      lcd.write(sliderToIndicator(displayValue[0]));
    }
    else
    {
      lcd.print(sliderNames[sliderNumber + 1]);
      lcd.setCursor(15, 1);
      lcd.write(sliderToIndicator(displayValue[sliderNumber + 1]));
    }
  }
  else if (state == valueScreen)
  { // put slider on screen
    lcd.setCursor(0, 0);
    lcd.print(sliderNames[sliderNumber]);
    lcd.setCursor(15, 0);
    lcd.write(sliderToIndicator(displayValue[sliderNumber]));
    lcd.setCursor(0, 1);
    lcd.print(displayValue[sliderNumber]);
    lcd.print("%");
  }
}

void UpdateSliders()
{
  String builtString = String("");
  for (int index = 0; index < amountSliders; index++)
  {
    builtString += String(displayValue[index]);
    if (index < amountSliders - 1)
    {
      builtString += String("|");
    }
  }
  if (Serial)
  {
    Serial.println(builtString); // combining every slider values seperated by | and sending it through the serial console
  }
}

void UpdateEEPROM()
{
  for (int i = 0; i < amountSliders; i++)
  { // only update when going back to main menu and the value needs to be changed
    if (previousValue[i] != displayValue[i])
    {
      Wire.beginTransmission(I2CAddress); // start writing to module
      Wire.write(0x00);                   // first byte of word
      Wire.write(i);                      // second byte of word is the needed byte of the EEPROM
      Wire.endTransmission();
      Wire.requestFrom(I2CAddress, 1); // start reading bytes
      while (!Wire.available())
      {
        EEPROMvalue = Wire.read();
      }
      if (displayValue[i] != EEPROMvalue)
      {                                     // only update EEPROM value if the value has changed to save some write cycles
        Wire.beginTransmission(I2CAddress); // start writing to module
        Wire.write(0x00);                   // first byte of word
        Wire.write(i);                      // second byte of word is the needed byte of the EEPROM
        Wire.write(displayValue[i]);        // writing to EEPROM
        Wire.endTransmission();
      }
    }
    previousValue[i] = displayValue[i];
  }
}

void RotateLeft(int amount)
{
  if (state == menuScreen)
  {
    sliderNumber--;
    if (sliderNumber < 0)
    {
      sliderNumber = amountSliders - 1;
    }
  }
  else if (state == valueScreen)
  {
      displayValue[sliderNumber] -= amount;
      if (displayValue[sliderNumber] < 0)
      {
        displayValue[sliderNumber] = 0;
      }
      UpdateSliders();
    // if (displayValue[sliderNumber] >= increment[sliderNumber])
    // { // decreasing slider
    //   displayValue[sliderNumber] = displayValue[sliderNumber] - increment[sliderNumber];
    //   UpdateSliders();
    // }
  }
  UpdateLCD();
}

void RotateRight(int amount)
{
  if (state == menuScreen)
  { // scrolling between all the slides
    sliderNumber++;
    if (sliderNumber > (amountSliders - 1))
    {
      sliderNumber = 0;
    }
  }
  else if (state == valueScreen)
  {
      displayValue[sliderNumber] += amount;
      if (displayValue[sliderNumber] > 100)
      {
        displayValue[sliderNumber] = 100;
      }
      UpdateSliders();

    // if ((100 - displayValue[sliderNumber]) >= increment[sliderNumber])
    // { // increasing slider
    //   displayValue[sliderNumber] = displayValue[sliderNumber] + increment[sliderNumber];
    //   UpdateSliders();
    // }
  }
  UpdateLCD();
}

void ButtonPress()
{
  if (state == menuScreen)
  {
    state = valueScreen;
  }
  else if (state == valueScreen)
  {
    state = menuScreen;
    if (useEEPROM == extEEPROM)
    {
      UpdateEEPROM();
    }
    else if (useEEPROM == intEEPROM)
    {
      for (int i = 0; i < amountSliders; i++)
      { // only update when going back to main menu and the value needs to be changed
        if (previousValue[i] != displayValue[i])
        {
          EEPROM.update(i, displayValue[i]); // writing value to EEPROM
        }
        previousValue[i] = displayValue[i];
      }
    }
    sliderNumber = 0;
  }
  UpdateLCD();
}
int sliderToIndicator(int valueToConvert)
{
  return map(valueToConvert, 0, 100, 1, amountIndicator);
}

int calculateTurnValue(int returnVal)
{
  int changevalue = 1;
  if (returnVal == lastENCread)
  { // still turning in the same direction
    ENCcounter++;
    unsigned long difference = (micros() - lastENCreadTime);
    Serial.print("Difference is ");
    Serial.println(difference);
    Serial.println(difference < PAUSELENGTH);
    if (difference < PAUSELENGTH)
    {
      changevalue = max((ENCcounter / STEPSIZE) * MULTIPLIER, 1);
      Serial.println(changevalue);
    }
    else
    {
      ENCcounter = 0;
    }
    lastENCreadTime = micros();
    Serial.print("lastENCreadTime ");
    Serial.println(lastENCreadTime);
  }
  else
  { // turned into other direction, resetting counter
    ENCcounter = 0;
  }
  lastENCread = returnVal;
  return abs(returnVal * changevalue);
}
