#include <Keypad.h>  // Keypad library for keypad input handling
#include <Servo.h>   // Servo library for controlling the servo motor
// Pin assignments for various components
#define Motor 10      // Pin connected to the motor
#define servo 11      // Pin connected to the servo motor
#define PirSensor 12  // Pin connected to the PIR motion sensor
#define led 13        // Pin connected to the status LED
#define Red_RGB A0    // Pin connected to the red component of the RGB LED
#define Green_RGB A1  // Pin connected to the green component of the RGB LED
#define Blue_RGB A2   // Pin connected to the blue component of the RGB LED
#define Buzzer A3     // Pin connected to the buzzer for error signaling
#define LDR A4        // Pin connected to the Light Dependent Resistor (LDR)
#define NTC A5        // Pin connected to the NTC thermistor for temperature measurement
// Keypad configuration
const byte ROWS = 4;  // Number of rows in the keypad
const byte COLS = 4;  // Number of columns in the keypad
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 5, 4, 3, 2 };  // Row pins connected to the keypad
byte colPins[COLS] = { 9, 8, 7, 6 };  // Column pins connected to the keypad
// Keypad object for handling keypad interactions
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo myservo;                                         // Servo object for door lock mechanism
char StorePass[] = "AB1234";                           // Pre-defined access password
int i = 0, count = 0, number = sizeof(StorePass) - 1;  // Password entry tracking variables
// NTC thermistor configuration
int seriesResistor = 10000;     // Series resistor value for thermistor
int nominalResistance = 10000;  // Thermistor resistance at 25°C
int nominalTemperature = 25;    // Nominal temperature in Celsius
int bCoefficient = 3950;        // Beta coefficient of the thermistor
int adcMax = 1023;              // Maximum ADC value (10-bit resolution)
// Array for storing entered password
char Pass[sizeof(StorePass) - 1];
float value = 0;        // Analog value from LDR
unsigned long res = 0;  // Calculated resistance from LDR reading

int lastMessage = 90; // a variable to hold the last message sent (initialized with garbage value of 90)
bool on = false;
void setup() {
  myservo.attach(servo);  // Attach servo motor to the specified pin
  myservo.write(0);       // Initialize servo to closed door position
  Serial.begin(9600);     // Initialize serial communication for debugging
  // Set pin modes for various components
  pinMode(Motor, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(Red_RGB, OUTPUT);
  pinMode(Green_RGB, OUTPUT);
  pinMode(Blue_RGB, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(PirSensor, INPUT);
}
void loop() {
  // If there is serial data sent 
  if (Serial.available() > 0) {
    int receivedMessage = Serial.read();
    if (receivedMessage == 48) CLOSED_DOOR(); // 0 means camera is disconnected so the door is shut
    else if (receivedMessage == 49 || receivedMessage == 50) { // 1 or 2 means open the door then start the system
      if (lastMessage != receivedMessage) {
        OPEN_DOOR();
        lastMessage = receivedMessage;
      }
      CORRECT();
    } else if (receivedMessage == 51 || receivedMessage == 52) { // 3 or 4 means don't open the door and give option to use the keypad
      KEYPAD();
      lastMessage = receivedMessage;
      delay(100);
    } else {
      CLOSED_DOOR();
    }
  }
}
void KEYPAD() {
  while (true) {
    if (on == true){ // If the system is already on, keep running in addition to giving the option to open the door for the new person
      // Read and process LDR (light sensor) value
    value = analogRead(LDR);             // Get analog reading from LDR
    value = (value / 1023) * 5;          // Convert reading to voltage
    res = (1000 * value) / (5 - value);  // Calculate LDR resistance
    Serial.print("Resistance: ");
    Serial.println(res);
    // Control LED based on light level
    if (res >= 100000 || res == 0) {
      digitalWrite(led, HIGH);  // Turn on LED if light level is low
    } else {
      digitalWrite(led, LOW);  // Turn off LED if light level is sufficient
    }
    // Read and process NTC thermistor value
    int adcValue = analogRead(NTC);                                 // Get analog reading from thermistor
    float resistance = seriesResistor / ((adcMax / adcValue) - 1);  // Calculate thermistor resistance
    float Temp;
    Temp = resistance / nominalResistance;        // Normalize resistance
    Temp = log(Temp);                             // Take natural logarithm
    Temp /= bCoefficient;                         // Apply Beta coefficient
    Temp += 1.0 / (nominalTemperature + 273.15);  // Convert to Kelvin
    Temp = 1.0 / Temp;                            // Invert
    Temp -= 273;                                  // Convert back to Celsius
    // Output temperature and control RGB LED and motor
    if (Temp > 30) {
      // High temperature: Red light and full motor speed
      digitalWrite(Red_RGB, HIGH);
      digitalWrite(Green_RGB, LOW);
      digitalWrite(Blue_RGB, LOW);
      digitalWrite(Motor, HIGH);
    } else if (Temp >= 20 && Temp <= 30) {
      // Medium temperature: Green light and half motor speed
      digitalWrite(Red_RGB, LOW);
      digitalWrite(Green_RGB, HIGH);
      digitalWrite(Blue_RGB, LOW);
      digitalWrite(Motor, HIGH);
      delay(10);
      digitalWrite(Motor, LOW);
      delay(10);
    } else if (Temp < 20) {
      // Low temperature: Blue light and motor off
      digitalWrite(Red_RGB, LOW);
      digitalWrite(Green_RGB, LOW);
      digitalWrite(Blue_RGB, HIGH);
      digitalWrite(Motor, LOW);
    }
    }
    char key = keypad.getKey();  // Read the pressed key from the keypad
    // Process key input, excluding 'D' used for submission
    if (key > '0' && key != 'D') {
      Serial.println("*");    // Output key to serial monitor
      Pass[i] = key;          // Store key in password array
      i++;                    // Move to the next position in the array
    } else if (key == 'D') {  // Handle password submission
      if (i == number) {      // Check if password length is correct
        for (int j = 0; j < number; j++) {
          if (Pass[j] == StorePass[j]) count++;  // Compare entered password with stored password
        }
        if (count == number) {  // If passwords match
          OPEN_DOOR();          // Unlock the door
          CORRECT();            // Execute post-authentication tasks
        } else {
          CLOSED_DOOR();  // Lock the door on incorrect password
          WRONG();        // Indicate wrong password entry
        }
      } else {
        CLOSED_DOOR();  // Lock the door on incorrect password length
        WRONG();        // Indicate wrong password entry
      }
    }
    if (Serial.available() > 0) { // Break from the loop if there is a known user or if the camera is disconnected
      int receivedMessage = Serial.read();
      if (receivedMessage == 48) CLOSED_DOOR();
      else if (receivedMessage == 49 || receivedMessage == 50) {
        if (lastMessage != receivedMessage) {
          OPEN_DOOR();
          lastMessage = receivedMessage;
        }
        CORRECT();
      } else {
        CLOSED_DOOR();
      }
    }
  }
}
void CORRECT() {
  on = true; // Set the flag to true to know that the system is opened
  while (true) {
    // Read and process LDR (light sensor) value
    value = analogRead(LDR);             // Get analog reading from LDR
    value = (value / 1023) * 5;          // Convert reading to voltage
    res = (1000 * value) / (5 - value);  // Calculate LDR resistance
    Serial.print("Resistance: ");
    Serial.println(res);
    // Control LED based on light level
    if (res >= 100000 || res == 0) {
      digitalWrite(led, HIGH);  // Turn on LED if light level is low
    } else {
      digitalWrite(led, LOW);  // Turn off LED if light level is sufficient
    }
    // Read and process NTC thermistor value
    int adcValue = analogRead(NTC);                                 // Get analog reading from thermistor
    float resistance = seriesResistor / ((adcMax / adcValue) - 1);  // Calculate thermistor resistance
    float Temp;
    Temp = resistance / nominalResistance;        // Normalize resistance
    Temp = log(Temp);                             // Take natural logarithm
    Temp /= bCoefficient;                         // Apply Beta coefficient
    Temp += 1.0 / (nominalTemperature + 273.15);  // Convert to Kelvin
    Temp = 1.0 / Temp;                            // Invert
    Temp -= 273;                                  // Convert back to Celsius
    // Output temperature and control RGB LED and motor
    if (Temp > 30) {
      // High temperature: Red light and full motor speed
      digitalWrite(Red_RGB, HIGH);
      digitalWrite(Green_RGB, LOW);
      digitalWrite(Blue_RGB, LOW);
      digitalWrite(Motor, HIGH);
    } else if (Temp >= 20 && Temp <= 30) {
      // Medium temperature: Green light and half motor speed
      digitalWrite(Red_RGB, LOW);
      digitalWrite(Green_RGB, HIGH);
      digitalWrite(Blue_RGB, LOW);
      digitalWrite(Motor, HIGH);
      delay(10);
      digitalWrite(Motor, LOW);
      delay(10);
    } else if (Temp < 20) {
      // Low temperature: Blue light and motor off
      digitalWrite(Red_RGB, LOW);
      digitalWrite(Green_RGB, LOW);
      digitalWrite(Blue_RGB, HIGH);
      digitalWrite(Motor, LOW);
    }
    if (Serial.available() > 0) { // Break from the loop if there is a known user or if the camera is disconnected
      int receivedMessage = Serial.read();
      if (receivedMessage == 48) CLOSED_DOOR();
      else if (receivedMessage == 49 || receivedMessage == 50) {
        if (lastMessage != receivedMessage) {
          OPEN_DOOR();
          lastMessage = receivedMessage;
        }
        CORRECT();
      }else if (receivedMessage == 51 || receivedMessage == 52) {
          lastMessage = receivedMessage;
          KEYPAD();
          delay(100);
        }
        else {
          CLOSED_DOOR();
        }
      }
    }
    i = 0;      // Reset password index
    count = 0;  // Reset password match count
  }
  void WRONG() {
    digitalWrite(Red_RGB, LOW);
    digitalWrite(Green_RGB, LOW);
    digitalWrite(Blue_RGB, LOW);
    digitalWrite(Buzzer, HIGH);  // Sound the buzzer for 500ms to indicate wrong password
    delay(500);
    digitalWrite(Buzzer, LOW);         // Turn off the buzzer
    Serial.println("WRONG password");  // Output wrong password message
    // Check PIR sensor for movement detection
    if (digitalRead(PirSensor)) {
      Serial.println("There is someone inside the house");  // Movement detected
    } else {
      Serial.println("There is no one inside the house");  // No movement detected
    }
    i = 0;      // Reset password index
    count = 0;  // Reset password match count
  }
  void OPEN_DOOR() {
    myservo.write(90);  // Unlock the door by rotating servo to 90 degrees
    Serial.println("The door is open");
    delay(5000);       // Keep the door open for 5 seconds
    myservo.write(0);  // Lock the door by rotating servo back to 0 degrees
  }
  void CLOSED_DOOR() {
    Serial.println("The door is closed");
    digitalWrite(Red_RGB, LOW);
    digitalWrite(Green_RGB, LOW);
    digitalWrite(Blue_RGB, LOW);
    digitalWrite(Motor, LOW);
    myservo.write(0);  // Ensure door is in locked position
  }
