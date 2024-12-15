//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 03_ESP32_RFID_Google_Spreadsheet_Attendance
//----------------------------------------Including the libraries.
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include "WiFi.h"
#include <HTTPClient.h>
//----------------------------------------

// Defines SS/SDA PIN and Reset PIN for RFID-RC522.
#define SS_PIN  5  
#define RST_PIN 4

// Defines the button PIN.
#define BTN_PIN 15

// Defines the fingerprint sensor pins.
#define FINGERPRINT_RX 3
#define FINGERPRINT_TX 1

//----------------------------------------SSID and PASSWORD of your WiFi network.
const char* ssid = "Redmi 10 5g";  //--> Your wifi name
const char* password = "1234567789"; //--> Your wifi password
//----------------------------------------

// Google script Web_App_URL.
String Web_App_URL = "https://script.google.com/macros/s/AKfycbyjbTDXkIhQBs1g1qtqks8bOukzH9CwCIb3c_jnC1aYD-pRTXb0H0qwKKYzcPcEViFh/exec";

String reg_Info = "";

String atc_Info = "";
String atc_Name = "";
String atc_Date = "";
String atc_Time_In = "";
String atc_Time_Out = "";
String atc_Status = "";

String bm_Info = "";
String bm_Name = "";
String bm_FBM = "";
String bm_Student_Number = "";
String bm_Year_Level = "";
String bm_Course = "";


int scansuccess;
char fingerprint[32] = "";
String FBM_Result = "";

// Variables for the number of columns and rows on the LCD.
int lcdColumns = 20;
int lcdRows = 4;

// Variable to read data from RFID-RC522.
int readsuccess;
char str[32] = "";
String UID_Result = "--------";

String modes = "atc";
String modes1 = "";
String modes2 = "";

// Define MasterCard UID
const String MASTER_CARD_UID = "F4C98C02"; // Replace with your MasterCard's UID

// Create LiquidCrystal_I2C object as "lcd" and set the LCD I2C address to 0x27 and set the LCD configuration to 20x4.
// In general, the address of a 20x4 I2C LCD is "0x27".
// However, if the address "0x27" doesn't work, you can find out the address with "i2c_scanner". Look here : https://playground.arduino.cc/Main/I2cScanner/
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  // (lcd_address, lcd_Columns, lcd_Rows)

// Create MFRC522 object as "mfrc522" and set SS/SDA PIN and Reset PIN.
MFRC522 mfrc522(SS_PIN, RST_PIN);  //--> Create MFRC522 instance.

// Create Adafruit_Fingerprint object as "finger" and set the fingerprint sensor pins.
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

//________________________________________________________________________________http_Req()
// Subroutine for sending HTTP requests to Google Sheets.
void http_Req(String str_modes, String str_uid, String str_fbm) {
  if (WiFi.status() == WL_CONNECTED) {
    String http_req_url = "";

    //----------------------------------------Create links to make HTTP requests to Google Sheets.
    if (str_modes == "atc") {
      http_req_url  = Web_App_URL + "?sts=atc";
      http_req_url += "&uid=" + str_uid;
    }
    if (str_modes == "reg") {
      http_req_url = Web_App_URL + "?sts=reg";
      http_req_url += "&uid=" + str_uid;
    }
    if (str_modes == "atc1") {
      http_req_url  = Web_App_URL + "?sts=bm";
      http_req_url += "&fbm=" + modes2;
    }
    if (str_modes == "reg1") {
      http_req_url = Web_App_URL + "?sts=reg";
      http_req_url += "&fbm=" + modes1;
    }
    //----------------------------------------

    //----------------------------------------Sending HTTP requests to Google Sheets.
    Serial.println();
    Serial.println("-------------");
    Serial.println("Sending request to Google Sheets...");
    Serial.print("URL : ");
    Serial.println(http_req_url);
    
    // Create an HTTPClient object as "http".
    HTTPClient http;

    // HTTP GET Request.
    http.begin(http_req_url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from google sheet.
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      Serial.println("Payload : " + payload);  
    }
    
    Serial.println("-------------");
    http.end();
    //----------------------------------------

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Example :                                                                                              //
    // Sending an http request to fill in "Time In" attendance.                                               //
    // User data :                                                                                            //
    // - Name : Adam                                                                                          //
    // - UID  : A01                                                                                           //
    // So the payload received if the http request is successful and the parameters are correct is as below : //
    // OK,Adam,29/10/2023,08:30:00 ---> Status,Name,Date,Time_In                                              //
    //                                                                                                        //
    // So, if you want to retrieve "Status", then getValue(payload, ',', 0);                                  //
    // String sts_Res = getValue(payload, ',', 0);                                                            //
    // So the value of sts_Res is "OK".                                                                       //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    String sts_Res = getValue(payload, ',', 0);

    //----------------------------------------Conditions that are executed are based on the payload response from Google Sheets (the payload response is set in Google Apps Script).
    if (sts_Res == "OK") {
      //..................
      if (str_modes == "atc") {
        atc_Info = getValue(payload, ',', 1);
        
        if (atc_Info == "TI_Successful") {
          atc_Name = getValue(payload, ',', 2);
          atc_Date = getValue(payload, ',', 3);
          atc_Time_In = getValue(payload, ',', 4);
          atc_Status = determineStatus(atc_Time_In);

          //::::::::::::::::::Create a position value for displaying "Name" on the LCD so that it is centered.
          int name_Lenght = atc_Name.length();
          int pos = 0;
          if (name_Lenght > 0 && name_Lenght <= lcdColumns) {
            pos = map(name_Lenght, 1, lcdColumns, 0, (lcdColumns / 2) - 1);
            pos = ((lcdColumns / 2) - 1) - pos;
          } else if (name_Lenght > lcdColumns) {
            atc_Name = atc_Name.substring(0, lcdColumns);
          }
          //::::::::::::::::::

          lcd.clear();
          delay(500);
          lcd.setCursor(pos,0);
          lcd.print(atc_Name);
          lcd.setCursor(0,1);
          lcd.print("Date    : ");
          lcd.print(atc_Date);
          lcd.setCursor(0,2);
          lcd.print("Time IN :   ");
          lcd.print(atc_Time_In);
          lcd.setCursor(0,3);
          lcd.print("Status:   ");
          lcd.print(atc_Status);
          delay(5000);
          lcd.clear();
          delay(500);
        }

        if (atc_Info == "TO_Successful") {
          atc_Name = getValue(payload, ',', 2);
          atc_Date = getValue(payload, ',', 3);
          atc_Time_In = getValue(payload, ',', 4);
          atc_Time_Out = getValue(payload, ',', 5);

          //::::::::::::::::::Create a position value for displaying "Name" on the LCD so that it is centered.
          int name_Lenght = atc_Name.length();
          int pos = 0;
          if (name_Lenght > 0 && name_Lenght <= lcdColumns) {
            pos = map(name_Lenght, 1, lcdColumns, 0, (lcdColumns / 2) - 1);
            pos = ((lcdColumns / 2) - 1) - pos;
          } else if (name_Lenght > lcdColumns) {
            atc_Name = atc_Name.substring(0, lcdColumns);
          }
          //::::::::::::::::::

          lcd.clear();
          delay(500);
          lcd.setCursor(pos,0);
          lcd.print(atc_Name);
          lcd.setCursor(0,1);
          lcd.print("Date    : ");
          lcd.print(atc_Date);
          lcd.setCursor(0,2);
          lcd.print("Time IN :   ");
          lcd.print(atc_Time_In);
          lcd.setCursor(0,3);
          lcd.print("Time Out:   ");
          lcd.print(atc_Time_Out);
          delay(5000);
          lcd.clear();
          delay(500);
        }

        if (atc_Info == "atcInf01") {
          lcd.clear();
          delay(500);
          lcd.setCursor(1,0);
          lcd.print("You have completed");
          lcd.setCursor(2,1);
          lcd.print("your  attendance");
          lcd.setCursor(2,2);
          lcd.print("record for today");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        if (atc_Info == "atcErr01") {
          lcd.clear();
          delay(500);
          lcd.setCursor(6,0);
          lcd.print("Error !");
          lcd.setCursor(4,1);
          lcd.print("Your card or");
          lcd.setCursor(4,2);
          lcd.print("key chain is");
          lcd.setCursor(3,3);
          lcd.print("not registered");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        atc_Info = "";
        atc_Name = "";
        atc_Date = "";
        atc_Time_In = "";
        atc_Time_Out = "";
        atc_Status = "";
      }
      //..................

      //..................
      if (str_modes == "reg") {
        reg_Info = getValue(payload, ',', 1);
        
        if (reg_Info == "R_Successful_UID") {
          lcd.clear();
          delay(500);
          lcd.setCursor(2,0);
          lcd.print("The UID of your");
          lcd.setCursor(0,1);
          lcd.print("card or keychain has");
          lcd.setCursor(1,2);
          lcd.print("been successfully");
          lcd.setCursor(6,3);
          lcd.print("uploaded");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        if (reg_Info == "regErr02") {
          lcd.clear();
          delay(500);
          lcd.setCursor(6,0);
          lcd.print("Error !");
          lcd.setCursor(0,1);
          lcd.print("The UID of your card");
          lcd.setCursor(0,2);
          lcd.print("or keychain has been");
          lcd.setCursor(5,3);
          lcd.print("registered");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        reg_Info = "";
      }
      //..................
      if (str_modes == "atc1") {
        bm_Info = getValue(payload, ',', 1);
        
        if (bm_Info == "BM_Successful") {
          bm_Name = getValue(payload, ',', 2);
          bm_Student_Number = getValue(payload, ',', 3);
          bm_Year_Level = getValue(payload, ',', 4);
          bm_Course = getValue(payload, ',', 5);

          //::::::::::::::::::Create a position value for displaying "Name" on the LCD so that it is centered.
          int name_Lenght = bm_Name.length();
          int pos = 0;
          if (name_Lenght > 0 && name_Lenght <= lcdColumns) {
            pos = map(name_Lenght, 1, lcdColumns, 0, (lcdColumns / 2) - 1);
            pos = ((lcdColumns / 2) - 1) - pos;
          } else if (name_Lenght > lcdColumns) {
            bm_Name = bm_Name.substring(0, lcdColumns);
          }
          //::::::::::::::::::

          lcd.clear();
          delay(500);
          lcd.setCursor(pos,0);
          lcd.print(bm_Name);
          lcd.setCursor(0,1);
          lcd.print("Student Number :");
          lcd.print(bm_Student_Number);
          lcd.setCursor(0,2);
          lcd.print("Year Level :");
          lcd.print(bm_Year_Level);
          lcd.setCursor(0,3);
          lcd.print("Course :");
          lcd.print(bm_Course);
          delay(5000);
          lcd.clear();
          delay(500);

          if (bm_Info == "bmInf01") {
          lcd.clear();
          delay(500);
          lcd.setCursor(1,0);
          lcd.print("You have completed");
          lcd.setCursor(2,1);
          lcd.print("your  Boimetrics");
          lcd.setCursor(2,2);
          lcd.print("record for today");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        if (bm_Info == "bmErr03") {
          lcd.clear();
          delay(500);
          lcd.setCursor(6,0);
          lcd.print("Error !");
          lcd.setCursor(4,1);
          lcd.print("Your Fingerprint");
          lcd.setCursor(6,2);
          lcd.print("key is");
          lcd.setCursor(3,3);
          lcd.print("not registered");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        bm_Info = "";
        bm_Name = "";
        bm_Student_Number = "";
        bm_Year_Level = "";
        bm_Course = "";
      }
      //..................

      //..................
      if (str_modes == "reg1") {
        reg_Info = getValue(payload, ',', 1);
        
        if (reg_Info == "R_Successful_FBM") {
          lcd.clear();
          delay(500);
          lcd.setCursor(2,0);
          lcd.print("Your Fingerprint");
          lcd.setCursor(6,1);
          lcd.print("key has");
          lcd.setCursor(1,2);
          lcd.print("been successfully");
          lcd.setCursor(6,3);
          lcd.print("uploaded");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        if (reg_Info == "regErr03") {
          lcd.clear();
          delay(500);
          lcd.setCursor(6,0);
          lcd.print("Error !");
          lcd.setCursor(2,1);
          lcd.print("Your Fingerprint");
          lcd.setCursor(3,2);
          lcd.print("key has been");
          lcd.setCursor(5,3);
          lcd.print("registered");
          delay(5000);
          lcd.clear();
          delay(500);
        }

        reg_Info = "";
      }
     }
    }
  } else {
    lcd.clear();
    delay(500);
    lcd.setCursor(6,0);
    lcd.print("Error !");
    lcd.setCursor(1,1);
    lcd.print("WiFi disconnected");
    delay(3000);
    lcd.clear();
    delay(500);
  }
}
//________________________________________________________________________________

String determineStatus(String timeIn) {
  // Assuming the format of timeIn is "HH:MM:SS"
  int hour = timeIn.substring(0, 2).toInt();
  int minute = timeIn.substring(3, 5).toInt();

  // Define the cutoff times for "On time," "Late," and "Absent"
  int onTimeHour = 7;
  int onTimeMinute = 45;
  int lateHour = 7;
  int lateMinute = 55;

  if (hour < onTimeHour || (hour == onTimeHour && minute <= onTimeMinute)) {
    return "On time";
  } else if (hour < lateHour || (hour == lateHour && minute <= lateMinute)) {
    return "Late";
  } else {
    return "Absent";
  }
}

// Function to verify MasterCard
bool isMasterCard(String uid) {
    return uid == MASTER_CARD_UID;
}

unsigned long sessionStartTime = 0;
const unsigned long SESSION_DURATION = 3 * 60 * 60 * 1000; // 3 hours in milliseconds
bool sessionActive = false; // Flag to track if session is active

// Function to check if session is active
bool isSessionActive() {
    if (sessionActive && (millis() - sessionStartTime) <= SESSION_DURATION) {
        return true;
    } else {
        sessionActive = false; // Session expired
        return false;
    }
}

//________________________________________________________________________________getValue()
// String function to process the data (Split String).
// I got this from : https://www.electroniclinic.com/reyax-lora-based-multiple-sensors-monitoring-using-arduino/
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//________________________________________________________________________________ 

//________________________________________________________________________________getUID()
// Subroutine to obtain UID/ID when RFID card or RFID keychain is tapped to RFID-RC522 module.
int getUID() {
  // Check for new RFID card
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }

  // Convert RFID UID to string
  byteArray_to_string(mfrc522.uid.uidByte, mfrc522.uid.size, str);
  UID_Result = str;

  // Halt RFID card and stop encryption
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return 1;
}
int getFBM() {
    // Check for fingerprint
  if (finger.getImage() != FINGERPRINT_OK) {
    return 0;
  }

  // Convert fingerprint image to template
  if (finger.image2Tz() != FINGERPRINT_OK) {
    return 0;
  }

  // Search for fingerprint in the database
  if (finger.fingerFastSearch() != FINGERPRINT_OK) {
    return 0;
  }

  // Get fingerprint ID and confidence
  int fingerprintID = finger.fingerID;
  int fingerprintConfidence = finger.confidence;

  // Display fingerprint results
  Serial.print("Fingerprint ID: ");
  Serial.println(fingerprintID);
  Serial.print("Confidence: ");
  Serial.println(fingerprintConfidence);

  FBM_Result = fingerprintID;

  return 1;
}

//________________________________________________________________________________byteArray_to_string()
void byteArray_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len*2] = '\0';
}
//________________________________________________________________________________

//________________________________________________________________________________VOID SETUP()
void setup(){
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial1.begin(57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX); // Initialize fingerprint sensor serial
  Serial.println();
  delay(1000);

  pinMode(BTN_PIN, INPUT_PULLUP);
  
  // Initialize LCD.
  lcd.init();
  // turn on LCD backlight.
  lcd.backlight();
  
  lcd.clear();

  delay(500);

  // Init SPI bus.
  SPI.begin();      
  // Init MFRC522.
  mfrc522.PCD_Init(); 

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(10);
    }
  }

  delay(500);

  lcd.setCursor(1,0);
  lcd.print("Biometrics Config");
  lcd.setCursor(4,1);
  lcd.print("with NFC ID");
  lcd.setCursor(1,2);
  lcd.print("Attendance  System");
  lcd.setCursor(3,3);
  lcd.print("By  E.C.A.D.S.");
  delay(3000);
  lcd.clear();

  //----------------------------------------Set Wifi to STA mode
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  WiFi.mode(WIFI_STA);
  Serial.println("-------------");
  //---------------------------------------- 

  //----------------------------------------Connect to Wi-Fi (STA).
  Serial.println();
  Serial.println("------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  //:::::::::::::::::: The process of connecting ESP32 with WiFi Hotspot / WiFi Router.
  // The process timeout of connecting ESP32 with WiFi Hotspot / WiFi Router is 20 seconds.
  // If within 20 seconds the ESP32 has not been successfully connected to WiFi, the ESP32 will restart.
  // I made this condition because on my ESP32, there are times when it seems like it can't connect to WiFi, so it needs to be restarted to be able to connect to WiFi.

  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");

    lcd.setCursor(0,0);
    lcd.print("Connecting to SSID");
    delay(250);

    lcd.clear();
    delay(250);
    
    if (connecting_process_timed_out > 0) connecting_process_timed_out--;
    if (connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("------------");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi connected");
  delay(2000);
  //::::::::::::::::::
  //----------------------------------------

  lcd.clear();
  delay(500);
}
//________________________________________________________________________________

//________________________________________________________________________________VOID LOOP()
void loop(){
  // put your main code here, to run repeatedly:

  //----------------------------------------Switches modes when the button is pressed.
  // modes = "reg" means the mode for new user registration.
  // modes = "atc" means the mode for filling in attendance (Time In and Time Out).

  int BTN_State = digitalRead(BTN_PIN);

  if (BTN_State == LOW) {
    lcd.clear();
    
    if (modes == "atc") {
      modes = "reg";
    } else if (modes == "reg") {
      modes = "atc1";
    } else if (modes == "atc1") {
      modes = "reg1";
    } else if (modes == "reg1") {
      modes = "atc";
    }
    
    
    delay(500);
  }
  //----------------------------------------

  // Detect if reading the UID from the card or keychain was successful.

  //----------------------------------------Conditions that are executed if modes == "atc".
  // Execute only if in "atc" mode
    if (!isSessionActive()) {
        lcd.setCursor(0, 0);
        lcd.print("Tap MasterCard");
        lcd.setCursor(0, 1);
        lcd.print("to start session");

        // Detect if reading the UID from the card or keychain was successful
        readsuccess = getUID();
        if (readsuccess && isMasterCard(UID_Result)) {
            sessionStartTime = millis(); // Start the session timer
            sessionActive = true;

            lcd.clear();
            lcd.setCursor(4, 0);
            lcd.print("Session Started");
            lcd.setCursor(2, 1);
            lcd.print("Valid for 3 hours");
            delay(3000);
        }
    } else if (modes == "atc") {
        // Session is active, allow attendance recording
        lcd.setCursor(3, 0);
        lcd.print("ATTENDANCE UID");
        lcd.setCursor(2, 1);
        lcd.print("Tap Student Card");
        lcd.setCursor(5, 3);
        lcd.print("to record");

        readsuccess = getUID();
        if (readsuccess) {
            lcd.clear();
            delay(500);
            lcd.setCursor(4, 0);
            lcd.print("Getting UID");
            lcd.setCursor(4, 1);
            lcd.print("Successfully");
            delay(1000);
            http_Req(modes, UID_Result, "");
                }
            }  
        
    
  //----------------------------------------

  //----------------------------------------Conditions that are executed if modes == "reg".

  if (modes == "reg") {
    lcd.setCursor(2,0);
    lcd.print("REGISTRATION UID");
    lcd.setCursor(0,1);
    lcd.print("");
    lcd.setCursor(0,2);
    lcd.print("Please tap your card");
    lcd.setCursor(4,3);
    lcd.print("or key chain");
    delay(1000);
    readsuccess = getUID();

     if (readsuccess){
      lcd.clear();
      delay(500);
      lcd.setCursor(0,0);
      lcd.print("Getting  UID");
      lcd.setCursor(0,1);
      lcd.print("Successfully");
      lcd.setCursor(0,2);
      lcd.print("UID : ");
      lcd.print(UID_Result);
      lcd.setCursor(0,3);
      lcd.print("Please wait...");  
      delay(1000);
      http_Req(modes, UID_Result, "");
    }

  }

  scansuccess = getFBM();

   if (modes == "atc1") {
    lcd.setCursor(3,0);
    lcd.print("BIOMETRICS FBM");
    lcd.setCursor(0,1);
    lcd.print("");
    lcd.setCursor(2,2);
    lcd.print("Please scan your");
    lcd.setCursor(4,3);
    lcd.print("fingerprint");
    delay(1000);
     scansuccess = getFBM();

    if (scansuccess) {
      lcd.clear();
      delay(500);
      lcd.setCursor(0,0);
      lcd.print("Fingerprint");
      lcd.setCursor(0,1);
      lcd.print("Successfully");
      lcd.setCursor(0,2);
      lcd.print("Enrolled");
      lcd.setCursor(0,3);
      lcd.print("Please wait...");
      delay(1000);
      http_Req(modes, FBM_Result, modes2);
    }
  }

  if (modes == "reg1") {
    lcd.setCursor(2,0);
    lcd.print("REGISTRATION FBM");
    lcd.setCursor(0,1);
    lcd.print("");
    lcd.setCursor(2,2);
    lcd.print("Please scan your");
    lcd.setCursor(4,3);
    lcd.print("fingerprint");
    delay(1000);

    if (fingerprintEnroll()) {
      lcd.clear();
      delay(500);
      lcd.setCursor(0,0);
      lcd.print("Fingerprint");
      lcd.setCursor(0,1);
      lcd.print("Successfully");
      lcd.setCursor(0,2);
      lcd.print("Enrolled");
      lcd.setCursor(0,3);
      lcd.print("Please wait...");
      delay(1000);
      http_Req(modes, FBM_Result, modes1);
    }
  }
  delay(10);
}

// Function to enroll fingerprint
bool fingerprintEnroll() {
  int p = -1;
  Serial.println("Waiting for valid finger to enroll as ID #1");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return false;
      default:
        Serial.println("Unknown error");
        return false;
    }
  }

  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(1);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return false;
      default:
        Serial.println("Unknown error");
        return false;
    }
  }

  // OK success!
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(1);
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }

  Serial.print("ID ");
  Serial.println(1);
  p = finger.storeModel(1);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    modes1 = String(1); // Store the fingerprint ID as the student number
    bm_FBM = modes1; // Assign the student number to atc_student_number
    return true;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return false;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }
}

//________________________________________________________________________________
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<