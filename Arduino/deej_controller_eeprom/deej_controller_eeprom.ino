#include <Wire.h>              //include the I²C library including it before the lcd library since it might happen that a normal lcd gets used (non I²C)
#include <LiquidCrystal_I2C.h> //library for an i2C display, change this to an regular lcd library to use an normal lcd
#include <EEPROM.h>            //library to use the eeprom

LiquidCrystal_I2C lcd(0x27, 16, 2); // don't forget to change this line when using an regular lcd
// if the I2C lcd is not working, check you connection and the address above (0x27);

// you can tweak following values for you needs

// Please read the README
const int useEEPROM = 0; // 0: use external eeprom
                         // 1: use internal eeprom
const int CLK = 8;
const int DT = 9;
const int SW = 7;
const int amountSliders = 9; // amount of sliders you want, also name them in the array below
const String sliderNames[amountSliders] = {
    "Headset",
    "Speakers",
    "Games",
    "Browser",
    "Music",
    "Communication",
    "Team",
    "System Sounds",
    "Mic",
};
const int increment[amountSliders] = {5, 5, 2, 1, 1, 5, 2, 5, 5};                // choose you're increment for each slider 1,2,4,5,10,20,25,50,100
const int I2CAddress = 0x50;                                                     // most common EEPROM module I²C address
int displayValue[amountSliders] = {100, 100, 100, 100, 100, 100, 100, 100, 100}; // start values for every slider

// leave following values at their default
enum
{
  extEEPROM = 0,
  intEEPROM = 1
};
int currentStateCLK;
int previousValue[amountSliders] = {100, 100, 100, 100, 100, 100, 100, 100, 100}; // extra values to see if it changed compared to last cycle
int lastStateCLK;
int sliderNumber = 0; // variable which numbers all the sliders
unsigned long lastButtonPress = 0;
bool singleButtonPress = false;
int state = 0; // state 0 is the menu screen to select what you want to change
               // state 1 is the screen where you change the value itself
enum
{
  menuScreen = 0,
  valueScreen = 1
};
// int EEPROMvalue = 0; //value to read in value from eeprom or to save them temporarly
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
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
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
   if(useEEPROM==extEEPROM){
     Wire.begin();//join the I²C bus as master
     Wire.beginTransmission(I2CAddress);//start writing to module
     Wire.write(0x00);//first byte of word
     Wire.write(0x00);//second byte of word so we need byte 0x0000 of the EEPROM
     Wire.endTransmission();
     Wire.requestFrom(I2CAddress, amountSliders); //start reading bytes
     for (int i = 0; i < amountSliders; i++) { //reading values out of the eeprom
       while (!Wire.available()) {}
       EEPROMvalue = Wire.read();
       if ((EEPROMvalue != 0) && (EEPROMvalue <= 100)) {
         displayValue[i] = EEPROMvalue;
       }
       previousValue[i] = displayValue[i];
     }
   }
   else if(useEEPROM==intEEPROM){
     for (int i = 0; i < amountSliders; i++) { //reading values out of the eeprom
       EEPROMvalue = EEPROM.read(i);
       if ((EEPROMvalue != 0) && (EEPROMvalue <= 100)) {
         displayValue[i] = EEPROMvalue;
       }
       else {
         EEPROM.write(i, 100); //if the values is not correct write 100 to it
       }
       previousValue[i] = displayValue[i];
     }
   }
}

void loop()
{
  // Read the current state of CLK
  currentStateCLK = digitalRead(CLK);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1)
  {

    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT) != currentStateCLK)
    {
      RotateRight();
      // Serial.println("rotateRight");
    }
    else
    {
      // Encoder is rotating CW so increment
      RotateLeft();
      // Serial.println("rotateLeft");
    }
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

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

void UpdateEEPROM(){
  for (int i = 0; i < amountSliders; i++) { //only update when going back to main menu and the value needs to be changed
    if (previousValue[i] != displayValue[i]) {
      Wire.beginTransmission(I2CAddress);//start writing to module
      Wire.write(0x00);//first byte of word
      Wire.write(i);//second byte of word is the needed byte of the EEPROM
      Wire.endTransmission();
      Wire.requestFrom(I2CAddress, 1); //start reading bytes
      while (!Wire.available()) {
        EEPROMvalue = Wire.read();
      }
      if (displayValue[i] != EEPROMvalue) { //only update EEPROM value if the value has changed to save some write cycles
        Wire.beginTransmission(I2CAddress);//start writing to module
        Wire.write(0x00);//first byte of word
        Wire.write(i);//second byte of word is the needed byte of the EEPROM
        Wire.write(displayValue[i]);//writing to EEPROM
        Wire.endTransmission();
      }
    }
    previousValue[i] = displayValue[i];
  }
}

void RotateLeft()
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
    if (displayValue[sliderNumber] >= increment[sliderNumber])
    { // decreasing slider
      displayValue[sliderNumber] = displayValue[sliderNumber] - increment[sliderNumber];
      UpdateSliders();
    }
  }
  UpdateLCD();
}

void RotateRight()
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
    if ((100 - displayValue[sliderNumber]) >= increment[sliderNumber])
    { // increasing slider
      displayValue[sliderNumber] = displayValue[sliderNumber] + increment[sliderNumber];
      UpdateSliders();
    }
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
       if(useEEPROM==extEEPROM){
         UpdateEEPROM();
       }
       else if(useEEPROM==intEEPROM){
         for (int i = 0; i < amountSliders; i++) { //only update when going back to main menu and the value needs to be changed
           if (previousValue[i] != displayValue[i]) {
             EEPROM.update(i, displayValue[i]);//writing value to EEPROM
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
