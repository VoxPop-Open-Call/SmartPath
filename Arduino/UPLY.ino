/************************************************
 * 
 * Author: Francisco Dias & Bernardo Galego
 * UPLY.ino
 * 
 * Fully Functional 9/2/2023
 * 
 * 
************************************************/

#include <SoftwareSerial.h>
#include <ctype.h>
#include "HX711.h"

// Defining pins
#define CLK1 2              // Clock load cell 1
#define DOUT1 3             // Data out load cell 1
#define Reed 4              // Reed Switch
#define RPWM 5              // PWM Signal for controlling motor speed – Straight rotation
#define LPWM 6              // PWM Signal for controlling motor speed – Inverse rotation
#define boot 7              // ON/OFF GPS button (connected to GPS pin 12)
#define REN 8               // Output Signal for controlling motor direction – Straight rotation
#define LEN 9               // Output Signal for controlling motor direction – Inverse rotation
#define DOUT2 10            // Data out load cell 2
#define CLK2 11             // Clock load cell 2
#define RX 12               // Serial receiver connected to GPS TX
#define TX 13               // Serial transmitter connected to GPS RX
#define Relay1 22           // Relay 1 - ON Door close
#define Relay2 23           // Relay 2 - ON Door open
#define Close_R 30          // Bumper Close Right side
#define Close_L 32          // Bumper Close Left side
#define Stop_R 40           // Bumper Stop Right side
#define Stop_L 42           // Bumper Stop Left side

// Defining variables
#define WEIGHTTHRESHOLD 1               // Kilograms
#define REGULARPERIOD 30                // Seconds
#define FASTPERIOD 1                    // Seconds
#define MESSAGEWAIT 0.6                 // Seconds
#define TIMEOUT 0.1                     // Seconds
#define WEIGHTCORRECTIONFACTOR1 4328.2  //! Load cell curve values. These may change with installation
#define WEIGHTCORRECTIONFACTOR2 28234
#define DEBUG false                     // If developer wants debug messages
#define INFO true                       // If developer wants info messages
#define BAUDRATE 57600                  // By trial and error, this was the best
    // To change baudrate, first connect to the module in this baud rate, then send "AT+IPREX=<baudrate>"
    // Accepted values are [300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 3000000, 3200000, 3686400]


// Switch States
int ReedState = 0;            // Reed Status
int PrevReed = 0;             // Reed Auxiliar
int StopRState = 0;           // Bumper Status - stop
int StopLState = 0;           // Bumper Status - stop
int CloseRState = 0;          // Bumper Status - close
int CloseLState = 0;          // Bumper Status - close

// Flags
int door = 0;                 // Door State: 0-closed, 1-opened
int open_uply = 0;
int stop_uply = 0;
int close_uply = 0;
bool doorFullyOpen = 0;

// Time variables
int open_time = 1500;         // max open time
int close_time = 1850;        // max close time
unsigned long start_op = 0;   // opening time starts counting
unsigned long stop_op = 0;    // opening time stops counting
unsigned long start_cl = 0;   // closing time starts counting
long int lastTimeOfWriting = 0;   // For always knowing UTCTime
char lastKnownUTCTime[12];

// GPS Serial
SoftwareSerial mySerial(RX, TX);

// Load cells
HX711 scale1, scale2;
float currentMaxWeight = 0;

// To get current time
long int t1;

// Period of GPS status update
int period;

// GPS values
char latitude[13], longitude[14], altitude[6], velocity[4], date[7], UTCTime[9];
char latitudeSign, longitudeSign;
bool latitudeDone = false, latitudeSignDone = false, longitudeSignDone = false;
bool longitudeDone = false, altitudeDone = false, velocityDone = false;
bool dateDone = false, UTCTimeDone = false;

// Serial number
String serialNumber;

// Functions
String getAnswer() {
  // To get current time
  long int t1Local, t2Local;

  // For strtok
  char* token;
  char temp[190];     // Hoping no message to parse is greater than this

  // Hold serial answers
  String answerString = "";
  int answer;

  while(!mySerial.available());
  while(true) {
    while (mySerial.available()) {
      answer = mySerial.read();
      answerString += char(answer);
    }

    // Wait to give time for serial answer to be completely written
    t1Local = millis();
    t2Local = millis();
    while (t2Local - t1Local < MESSAGEWAIT * 1000)
      t2Local = millis();

    // If there still isn't any new information, continue
    if(!mySerial.available())
      break;
  }
  if (DEBUG)
    Serial.println(answerString);
  
  // Remove original question from beggining (serial echoes back)
  answerString.toCharArray(temp, answerString.length() + 1);
  token = strtok(temp, "\n"); // This removes original question (most of the time)
  token = strtok(NULL, "\r"); // This ensures that the token contains the answer

  return token;
}

void clearOutput()  {
  while (mySerial.available())
    mySerial.read();
  while (Serial.available())
    Serial.read();
}

void sendATCommand(char* outgoingString, char* expectedAnswer, char* backup)  {
  // These are to get current time
  long int t1Local, t2Local;

  // These are for command "AT+HTTPACTION=0\r\n"
  char* moduleAnswer;
  char* tokens[5];  // This should only fill 3 elements, added two more out of caution
  char* serverCode;

  // For strtok
  char* token;
  char temp[50];  

  // Hold serial answers
  String answerString, finalString;
  int answer;
    
  while (true) {
    if(DEBUG) {
      Serial.print("Sending ");
      Serial.print(outgoingString);
    }
    clearOutput();
    for (unsigned int iter = 0; iter < strlen(outgoingString); iter++) 
      mySerial.write(outgoingString[iter]);

  /*
    if (outgoingString == "AT+HTTPACTION=0\r\n") {
      // Although this command may be well executed by the module,
      // still need to check if server executed it properly
      while(!mySerial.available());
      answerString = "";
      while(true) {
        while (mySerial.available()) {
          answer = mySerial.read();
          answerString += char(answer);
        }
    
        // Wait to give time for serial answer to be completely written
        t1Local = millis();
        t2Local = millis();
        while (t2Local - t1Local < MESSAGEWAIT * 1000)
          t2Local = millis();
    
        // If there still isn't any new information, continue
        if(!mySerial.available())
          break;
      }
      if (DEBUG)
        Serial.println(answerString);
      // Remove original question from beggining (serial echoes back)
      answerString.toCharArray(temp, answerString.length() + 1);
      token = strtok(temp, "\n"); // This removes original question (most of the time)
      moduleAnswer = strtok(NULL, "\r"); // This ensures that the token contains the answer
      token = strtok(NULL, " ");
      int iter = 0;
      while (token != NULL)  {      // Start collecting all the tokens to see if
        token = strtok(NULL, ",");  // there's the right number of them
        tokens[iter] = token;
        iter++;
      }
      // Should only be 3 elements in the array
      if (iter == 3) {
        strcpy(serverCode, tokens[1]);
        // 200 = Request success
        if (strcmp(serverCode, "200") && strcmp(moduleAnswer, expectedAnswer))  {
          clearOutput();
          return;
        }
      }
      if(DEBUG) {
        Serial.print("ignored answer, number of elements was ");
        Serial.println(iter);
      }
    }
    else  {*/
    finalString = getAnswer();

    if (finalString == expectedAnswer || expectedAnswer == "")  {
      clearOutput();
      return;
    }
    else if (DEBUG) {
      Serial.print("Unexpected answer: ");
      Serial.println(finalString);
    }

    // If answer wasn't acceptable, try backup command if it exists,
    // then try again original command
    if (backup != "")  {
      clearOutput();
      for (int iter = 0; iter < strlen(backup); iter++)
        mySerial.write(backup[iter]);

      getAnswer();  // Wait for backup's response
    }

    // Wait a bit
    t1Local = millis();
    t2Local = millis();
    while (t2Local - t1Local < TIMEOUT * 1000)
      t2Local = millis();
  }
}

String getGPSFix()  {
  // Command to send
  char* command = "AT+CGPSINFO\r\n";

  // Hold serial answers
  String finalString;

  clearOutput();
  for (int iter = 0; iter < strlen(command); iter++)
    mySerial.write(command[iter]);

  // Correct output is (Lat, N/S, Long, E/W, date, UTC time, altitude, velocity, course)
  return getAnswer();
}

void parseGPSMessage(String message) {
  // For strtok
  char* token;
  char temp[100];     // Hoping no message to parse is greater than this
  char* tokens[14];   // This should only fill 10 elements, added 4 out of caution
  
  // For HTTP requests
  char URL[190];
  strcpy(URL, "AT+HTTPPARA=\"URL\",\"http://uply.detutech.pt/UPLYGPS.php?Latitude=");

  // For parsing
  char tempArray[13];
  char serialNumberArray[20];
  char tempDate[7];
  
  
  if (INFO)  {
    Serial.print("Got fix: ");
    Serial.println(message);
  }
  message.toCharArray(temp, message.length());

  // Start dissecting message
  token = strtok(temp, " ");    // Ignore header
  int tokenNumber = 0;
  while (token != NULL)  {      // Start collecting all the tokens to see if
    token = strtok(NULL, ",");  // there's the right number of them
    tokens[tokenNumber] = token;
    tokenNumber++;
  }

  // Only proceed if there are only 10 elements, else ignore message
  // Only check values that are not yet confirmed
  if (tokenNumber == 10) {
    if (latitudeDone == false)  {
      strcpy(latitude, tokens[0]);
      // Check if the value is OK
      for (int iter = 0; iter < sizeof(latitude); iter++) {
        if(latitude[iter] == '\0')  { // Good value
          latitudeDone = true;
          if (INFO)  {
            Serial.print("Found latitude: ");
            Serial.println(tokens[0]);
            break;
          }
        }
        else if (!(isDigit(latitude[iter]) || latitude[iter] == '.'))
            break;
        }
      }

    if (latitudeSignDone == false)  {
      latitudeSign = *tokens[1];
      // Check if the value is OK
      if (latitudeSign == 'N' || latitudeSign == 'S')  {
        latitudeSignDone = true;
        if (INFO)  {
          Serial.print("Found latitude signal: ");
          Serial.println(tokens[1]);
        }
      }
    }

    if (longitudeDone == false)  {
      strcpy(longitude, tokens[2]);
      // Check if the value is OK
      for (int iter = 0; iter < sizeof(longitude); iter++)  {
        if(longitude[iter] == '\0')  { // Good value
          longitudeDone = true;
          if (INFO)  {
            Serial.print("Found longitude: ");
            Serial.println(tokens[2]);
            break;
          }
        }
        else if (!(isDigit(longitude[iter]) || longitude[iter] == '.')) 
          break;
      }
    }

    if (longitudeSignDone == false)  {
      longitudeSign = *tokens[3];
      // Check if the value is OK
      if (longitudeSign == 'E' || longitudeSign == 'W')  {
        longitudeSignDone = true;
        if (INFO)  {
          Serial.print("Found longitude signal: ");
          Serial.println(tokens[3]);
        }
      }
    }

    if (dateDone == false)  {
      strcpy(date, tokens[4]);
      // Check if the value is OK
      for (int iter = 0; iter < sizeof(date); iter++) {
        if(date[iter] == '\0')  { // Good value
          dateDone = true;

          // Change date format, because PHP is expecting YYMMDD and module gives DDMMYY
          strcpy(tempDate, date);
          date[0] = tempDate[4];
          date[1] = tempDate[5];
          date[4] = tempDate[0];
          date[5] = tempDate[1];

          if (INFO)  {
            Serial.print("Found date: ");
            Serial.println(tokens[4]);
          }
        }
        else if (!(isDigit(date[iter]))) 
          break;
      }
    }

    if (UTCTimeDone == false)  {
      strcpy(UTCTime, tokens[5]);
      // Check if the value is OK
      for (int iter = 0; iter < sizeof(UTCTime); iter++) {
        if(UTCTime[iter] == '\0')  { // Good value
          UTCTimeDone = true;

          // Save last known UTCTime and time of writing
          lastKnownUTCTime = UTCTime
          lastTimeOfWriting = millis()

          if (INFO)  {
            Serial.print("Found UTCTime: ");
            Serial.println(tokens[5]);
          }
        }
        else if (!(isDigit(UTCTime[iter]) || UTCTime[iter] == '.')) 
          break;
      }
    }

    if (altitudeDone == false)  {
      strcpy(altitude, tokens[6]);
      // Check if the value is OK
      for (int iter = 0; iter < sizeof(altitude); iter++) {
        if(altitude[iter] == '\0')  { // Good value
          altitudeDone = true;
          if (INFO)  {
            Serial.print("Found altitude: ");
            Serial.println(tokens[6]);
            break;
          }
        }
        else if (!(isDigit(altitude[iter]) || altitude[iter] == '.' || altitude[iter] == '-')) 
          break;
      }
    }

    if (velocityDone == false)  {
      strcpy(velocity, tokens[7]);
      // Check if the value is OK
      for (int iter = 0; iter < sizeof(velocity); iter++) {
        if(velocity[iter] == '\0')  { // Good value
          velocityDone = true;
          if (INFO)  {
            Serial.print("Found velocity: ");
            Serial.println(tokens[7]);
          }
        }
        else if (!(isDigit(velocity[iter]) || velocity[iter] == '.')) 
          break;
      }
    }

    // Check to see if every value is accounted for
    if (latitudeDone == true && latitudeSignDone == true &&
        longitudeSignDone == true && longitudeDone == true &&
        altitudeDone == true && velocityDone == true && dateDone == true
        && UTCTimeDone == true) {
      // Check for sign of latitude and longitude
      if (latitudeSign == 'S')  {
        strcpy(tempArray, latitude);
        strcpy(latitude, "-");
        strcpy(latitude + 1, tempArray);
      }

      if (longitudeSign == 'W') {
        strcpy(tempArray, longitude);
        strcpy(longitude, "-");
        strcpy(longitude + 1, tempArray);
      }
        
      if (INFO)  {
        Serial.println("Found a complete message, sending: ");
        Serial.print("Latitude: ");
        Serial.println(latitude);
        Serial.print("Longitude: ");
        Serial.println(longitude);
        Serial.print("Date: ");
        Serial.println(date);
        Serial.print("UTCTime: ");
        Serial.println(UTCTime);
        Serial.print("Altitude: ");
        Serial.println(altitude);
        Serial.print("Velocity: ");
        Serial.println(velocity);
        Serial.print("UPLYID: ");
        Serial.println(serialNumber);
      }

      // All good, revert timeInterval, set every bool to false and send message
      latitudeDone = false;
      latitudeSignDone = false;
      longitudeSignDone = false;
      longitudeDone = false;
      altitudeDone = false;
      velocityDone = false;
      dateDone = false;
      UTCTimeDone = false;

      period = REGULARPERIOD;

      // Send HTTP request with found data
      // Set URL with GET parameters
      strcat(URL, latitude);
      strcat(URL, "&Longitude=");
      strcat(URL, longitude);
      strcat(URL, "&Altitude=");
      strcat(URL, altitude);
      strcat(URL, "&Velocity=");
      strcat(URL, velocity);
      strcat(URL, "&UTCTime=");
      strcat(URL, UTCTime);
      strcat(URL, "&Date=");
      strcat(URL, date);
      strcat(URL, "&UPLYID=");
      serialNumber.toCharArray(serialNumberArray, sizeof(serialNumberArray));
      strcat(URL, serialNumberArray);
      strcat(URL, "\"\r\n"); 
      sendATCommand(URL, "", ""); //! IGNORING MODULE ANSWER. This is bad but rn only way that works

      if (DEBUG) {
        Serial.print("Sending to server ");
        Serial.println(URL);
      }

      // Send message
      sendATCommand("AT+HTTPACTION=0\r\n", "OK", ""); //! IGNORING SERVER ANSWER.
      if (INFO)
        Serial.println("Sent message to server");
    }
    else  // Some parameters are missing, reduce timeInterval
      period = FASTPERIOD;
  }
  else  {  // Message was ignored, reduce timeInterval
    period = FASTPERIOD;
    if (INFO)  {
      Serial.print("Ignoring message, number of elements is ");
      Serial.println(tokenNumber);
    }
  }
}

String getSerialNumber(char* outgoingString)  {
  // These are to get current time
  long int t1Local, t2Local;

  // For serial number verification
  bool gotOne = false;
  String tempSerial;

  // Hold serial answers
  String finalString;

  while (true) {
    clearOutput();
    for (int iter = 0; iter < strlen(outgoingString); iter++)
      mySerial.write(outgoingString[iter]);

    finalString = getAnswer();

    // Check to see if every value received is a digit
    for (int iter = 0; iter < finalString.length(); iter++) {
      if (isDigit(finalString.charAt(iter)))  {
        if (iter == finalString.length() - 1) { // Made it to the end, good value
          // To make sure the received value is legit, get it twice
          if (gotOne == false) {
            gotOne = true;
            tempSerial = finalString;
          }
          else  {
            // Compare the two. If they're not the same, keep most recent one and try again
            if (tempSerial == finalString)  {
                if(INFO)  {
                  Serial.print("Found serial number: ");
                  Serial.println(finalString);
                }
                return finalString;
            }
            else
              tempSerial = finalString;
          }
        }
      }
      else
        break;
    }
    
    // Wait a bit
    t1Local = millis();
    t2Local = millis();
    while (t2Local - t1Local < TIMEOUT * 1000)
      t2Local = millis();
  }
}

//TODO
char[] getOpeningUTCTime(unsigned long start_op) {
  char OpeningUTCTime[12];
  int diffSeconds, beginSeconds, diff, minuteCount = 0, iter = 0;

  diffSeconds = int((start_op - lastTimeOfWriting) / 1000);
  OpeningUTCTime = lastKnownUTCTime;  //! Probably doesnt work

  // Sum seconds
  // Find end of string
  while true {
    if(OpeningUTCTime[iter] == '\0')  
      break;
    iter += 1;
  }

  beginSeconds = int(OpeningUTCTime[iter - 1] + OpeningUTCTime[iter - 2]);
  while(beginSeconds > 60) {  // Subtract minutes
    diff = beginSeconds - 60;
    minuteCount += 1;
    beginSeconds = diff;
  }
  if(minuteCount == 0)  { // No minutes added, just sum seconds
    finalSeconds = beginSeconds + diffSeconds
    OpeningUTCTime[iter - 2] = '\0'
    strcat(OpeningsUTCTime, char(finalSeconds)) //! Also wont work
  }
  else  {
    //TODO
  }
}

// Motor movement
void motor_close(){
  analogWrite(RPWM,150);
  analogWrite(LPWM,0);
}

void motor_open(){
  analogWrite(RPWM,0);
  analogWrite(LPWM,150);
}

void motor_stop(){
  analogWrite(RPWM,0);
  analogWrite(LPWM,0);
}

void setup() {
  // Open serial communications
  // This serial interface is for debug only
  Serial.begin(BAUDRATE);

  // Wait for serial port to connect. Needed for native USB port only
  //while (!Serial);

  // Set the data rate for the SoftwareSerial port
  mySerial.begin(BAUDRATE);

  // Start load cells and tare them
  if (INFO)
    Serial.println("Starting load cells");
  scale1.begin(DOUT1, CLK1);
  scale2.begin(DOUT2, CLK2);
  scale1.set_scale();
  scale2.set_scale();
  scale1.tare(10);           // Argument is number of readings
  scale2.tare(10);

  // Power up GPS module
  if (INFO)
    Serial.println("Powering up GPS module");
  pinMode(boot, OUTPUT);
  digitalWrite(boot, HIGH);
  delay(2500);              // Needs to be HIGH for at least 2 seconds
  digitalWrite(boot, LOW);
  delay(3000);              // Wait for boot

  // Check if connection is OK
  if (INFO)
    Serial.println("Checking GPS module connection");
  sendATCommand("AT\r\n", "OK", "");

  // Get serial number
  if (INFO)
    Serial.println("Getting GPS serial number");
  serialNumber = getSerialNumber("AT+CGSN\r\n");

  // Start GPS module
  if (INFO)
    Serial.println("Initialising GPS service");
  sendATCommand("AT+CGPS=1\r\n", "OK", "AT+CGPS=0\r\n");

  // Initializing HTTP service
  if (INFO)
    Serial.println("Initialising HTTP service");
  sendATCommand("AT+HTTPINIT\r\n", "OK", "AT+HTTPTERM\r\n");

  // Get current time
  t1 = millis();

  // Set GPS acquisition period
  period = REGULARPERIOD;

  // Switches
  pinMode(Reed, INPUT_PULLUP);
  pinMode(Stop_R, INPUT_PULLUP);
  pinMode(Stop_L, INPUT_PULLUP);
  pinMode(Close_R, INPUT_PULLUP);
  pinMode(Close_L, INPUT_PULLUP);

  // Motor
  pinMode(RPWM,OUTPUT);
  pinMode(LPWM,OUTPUT);
  pinMode(LEN,OUTPUT);
  pinMode(REN,OUTPUT);
  digitalWrite(REN,HIGH);
  digitalWrite(LEN,HIGH);   

  // Solenoids
  pinMode(Relay1,OUTPUT); 
  pinMode(Relay2,OUTPUT); 
  digitalWrite(Relay1,HIGH);  // RELAY OFF
  digitalWrite(Relay2,HIGH);  // RELAY OFF

  if(INFO)
    Serial.println("Moving into main loop");
}

void loop() {
  // This is to get current time
  long int t2;

  // To read values from amplifiers
  long int reading1, reading2;
  float weight;

  // Hold GPS fix information
  String GPSFix;


  // If UPLY is closed, reset currentMaxWeight
  if(door == 0 && currentMaxWeight != 0)
    currentMaxWeight = 0;

  // If UPLY is open, read values from amplifiers if they're above a certain threshold
  if(door == 1)  {
    reading1 = scale1.get_units(10);  // Argument is number of readings
    reading2 = scale2.get_units(10);
    
    weight = (reading1 + reading2 - WEIGHTCORRECTIONFACTOR1) / WEIGHTCORRECTIONFACTOR2;
    if(weight > WEIGHTTHRESHOLD && weight > currentMaxWeight && weight > 0) {
      currentMaxWeight = weight;
      if(INFO) {
        Serial.print("Got loadcell reading. Weight is ");
        Serial.print(weight);
        Serial.println("kg");
      }
    }
  }  

  // Get GPS updates every period seconds
  t2 = millis();
  if (t2 - t1 > period * 1000) {
    t1 = t2;
    if (INFO)
      Serial.println("Trying to get GPS fix...");
    GPSFix = getGPSFix();

    // If it's a correct NMEA sentence, check if its values are good
    // If not, rapidly ask for another update and try to fill in the gaps
    if (GPSFix.length() > 30)   // This seems enough to prevent false positives
      parseGPSMessage(GPSFix);
  }
  
  ReedState = digitalRead(Reed);

  if (PrevReed != ReedState){
    PrevReed = ReedState;
    open_uply = 1;    
  }

  // Door goes from close to open - UPLY opens
  if ((open_uply == HIGH) && (ReedState == HIGH) && (door == 0)){
    start_op = millis();                // start opening time
    
    //Get OpeningUTCTime
    getOpeningUTCTime(start_op);

    // Opening movement
    motor_open();
    while(((millis() - start_op) < open_time) && stop_uply == LOW){
      // Read stop bumpers
      StopRState = digitalRead(Stop_R);
      StopLState = digitalRead(Stop_L);
      if((StopRState == 0) || (StopLState == 0))
        stop_uply = 1; 
    }

    motor_stop();
    
    door = 1;
    stop_op = millis();                 // stop opening time 
    open_time = stop_op - start_op;     // opening time
    open_uply = 0;                      // reset flag

    if(stop_uply == 1){
      start_cl = millis(); 
      motor_close();
      while(((millis() - start_cl) < (open_time + 300)));
      motor_stop();
      door = 0;
      open_time = 1500;
      close_uply = 0;  
    }
    stop_uply = 0;
  }

  // Read close bumpers
  CloseRState = digitalRead(Close_R);
  CloseLState = digitalRead(Close_L);
  if(CloseRState == 0 && CloseLState == 0){
    doorFullyOpen = 1;
    Serial.println("Door is fully open");
  }
  else if(doorFullyOpen == 1 && CloseRState == 1 && CloseLState == 1){  // Bumpers should be pulled up
    Serial.println("Closing");
    doorFullyOpen = 0;
    close_uply = 1;
  }

  // Door goes from open to close - UPLY closes
  if ((close_uply == HIGH) && (door == 1)){
    start_cl = millis();

    // Closing movement
    motor_close();
    while(((millis() - start_cl) < close_time));
    motor_stop();
    door = 0;
    open_time = 1500;
    close_uply = 0;
  }
}