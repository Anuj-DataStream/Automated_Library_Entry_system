#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

// ======================================================
// WiFi - Wokwi virtual WiFi
// ======================================================

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// India Standard Time = UTC + 5:30
const char* NTP_SERVER = "pool.ntp.org";


// ======================================================
// GOOGLE APPS SCRIPT WEB APP
// ======================================================
//
// IMPORTANT:
// Replace this with your actual Google Apps Script
// Web App URL ending with /exec
//

const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbxqRelT7x1grkYgpwSV6tkO_wkGTOYMUGzjf4a1foYkrhrQC5Y-i_LNVfKvCJRs-mBk4g/exec";


// ======================================================
// RFID CONNECTIONS
// ======================================================

#define RFID_SDA   21
#define RFID_SCK   18
#define RFID_MOSI  23
#define RFID_MISO  19
#define RFID_RST   22

MFRC522 rfid(RFID_SDA, RFID_RST);


// ======================================================
// BUZZER
// ======================================================

#define BUZZER_PIN 15


// ======================================================
// LCD 16x2
// RS, E, D4, D5, D6, D7
// ======================================================

LiquidCrystal lcd(
  27,
  26,
  25,
  33,
  32,
  4
);


// ======================================================
// STUDENT DATABASE
// ======================================================

struct Student {

  String uid;
  String name;
  String rollNo;
  String branch;

  bool inside;

  String entryDate;
  String entryTime;

  String exitDate;
  String exitTime;
};


// ======================================================
// MULTIPLE STUDENTS
// ======================================================

Student students[] = {

  {
    "01 02 03 04",
    "Anuj",
    "101",
    "AR",
    false,
    "",
    "",
    "",
    ""
  },

  {
    "11 22 33 44",
    "Rahul",
    "102",
    "AR",
    false,
    "",
    "",
    "",
    ""
  },

  {
    "55 66 77 88",
    "Aman",
    "103",
    "AR",
    false,
    "",
    "",
    "",
    ""
  },

  {
    "AA BB CC DD",
    "Riya",
    "104",
    "AR",
    false,
    "",
    "",
    "",
    ""
  }
};


const int NUMBER_OF_STUDENTS =
  sizeof(students) / sizeof(students[0]);


// ======================================================
// FUNCTION DECLARATIONS
// ======================================================

String getUID();

int findStudent(String uid);

String getDate();

String getTime();

void beepEntry();

void beepExit();

void beepError();

void showReadyScreen();

void showEntry(Student &student);

void showExit(Student &student);

void connectWiFi();

void setupTime();

String urlEncode(String value);

bool sendToGoogleSheet(Student &student);


// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("======================================");
  Serial.println("   SMART LIBRARY RFID SYSTEM");
  Serial.println("======================================");


  // ----------------------------------------------------
  // Buzzer
  // ----------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);


  // ----------------------------------------------------
  // LCD
  // ----------------------------------------------------

  lcd.begin(16, 2);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Smart Library");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(1500);


  // ----------------------------------------------------
  // WiFi
  // ----------------------------------------------------

  connectWiFi();


  // ----------------------------------------------------
  // NTP TIME
  // ----------------------------------------------------

  setupTime();


  // ----------------------------------------------------
  // RFID
  // ----------------------------------------------------

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SDA
  );

  rfid.PCD_Init();

  delay(100);


  // ----------------------------------------------------
  // READY
  // ----------------------------------------------------

  Serial.println();
  Serial.println("RFID reader ready.");
  Serial.println("Waiting for card...");

  showReadyScreen();
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  // No card detected
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }


  // Card detected but cannot be read
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }


  // ----------------------------------------------------
  // Read UID
  // ----------------------------------------------------

  String uid = getUID();


  Serial.println();
  Serial.println("--------------------------------------");

  Serial.print("RFID UID: ");
  Serial.println(uid);


  // ----------------------------------------------------
  // Find student
  // ----------------------------------------------------

  int index = findStudent(uid);


  // ====================================================
  // UNKNOWN CARD
  // ====================================================

  if (index == -1) {

    Serial.println("STATUS: UNKNOWN CARD");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Unknown Card");

    lcd.setCursor(0, 1);
    lcd.print("Access Denied");

    beepError();

    delay(2000);

    showReadyScreen();
  }


  // ====================================================
  // KNOWN STUDENT
  // ====================================================

  else {

    Student &student = students[index];


    Serial.print("Name: ");
    Serial.println(student.name);

    Serial.print("Roll No: ");
    Serial.println(student.rollNo);

    Serial.print("Branch: ");
    Serial.println(student.branch);


    // ==================================================
    // ENTRY
    // ==================================================

    if (student.inside == false) {

      student.inside = true;

      student.entryDate = getDate();
      student.entryTime = getTime();

      Serial.println("ACTION: ENTRY");

      Serial.print("Date: ");
      Serial.println(student.entryDate);

      Serial.print("Time: ");
      Serial.println(student.entryTime);


      // ------------------------------------------------
      // SEND DATA TO GOOGLE SHEET
      // ------------------------------------------------

      bool sent = sendToGoogleSheet(student);

      if (sent) {

        Serial.println("Google Sheet: ENTRY recorded");

      } else {

        Serial.println("Google Sheet: FAILED");
      }


      showEntry(student);

      beepEntry();
    }


    // ==================================================
    // EXIT
    // ==================================================

    else {

      student.inside = false;

      student.exitDate = getDate();
      student.exitTime = getTime();

      Serial.println("ACTION: EXIT");

      Serial.print("Date: ");
      Serial.println(student.exitDate);

      Serial.print("Time: ");
      Serial.println(student.exitTime);


      // ------------------------------------------------
      // SEND DATA TO GOOGLE SHEET
      // ------------------------------------------------

      bool sent = sendToGoogleSheet(student);

      if (sent) {

        Serial.println("Google Sheet: EXIT recorded");

      } else {

        Serial.println("Google Sheet: FAILED");
      }


      showExit(student);

      beepExit();
    }


    delay(3000);

    showReadyScreen();
  }


  // ----------------------------------------------------
  // Stop RFID communication
  // ----------------------------------------------------

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();


  // Small delay so the same card isn't immediately
  // processed again
  delay(500);
}


// ======================================================
// SEND RFID DATA TO GOOGLE SHEET
// ======================================================
//
// IMPORTANT:
// Only these four parameters are sent:
//
// uid
// roll
// name
// branch
//
// Example:
//
// ?uid=01%2002%2003%2004&roll=101
// &name=Anuj&branch=AR
//
// Google Apps Script decides whether this is ENTRY
// or EXIT by checking the existing Sheet data.
// ======================================================

bool sendToGoogleSheet(Student &student) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("ERROR: WiFi not connected");

    return false;
  }


  // ----------------------------------------------------
  // Build URL
  // ----------------------------------------------------

  String url = String(GOOGLE_SCRIPT_URL);

  url += "?uid=";
  url += urlEncode(student.uid);

  url += "&roll=";
  url += urlEncode(student.rollNo);

  url += "&name=";
  url += urlEncode(student.name);

  url += "&branch=";
  url += urlEncode(student.branch);


  // ----------------------------------------------------
  // Print URL for debugging
  // ----------------------------------------------------

  Serial.println();
  Serial.println("Sending to Google Apps Script:");

  Serial.println(url);


  // ----------------------------------------------------
  // HTTPS connection
  // ----------------------------------------------------

  WiFiClientSecure client;

  // Google uses HTTPS.
  // For this prototype we don't validate the certificate.
  client.setInsecure();


  HTTPClient http;


  if (!http.begin(client, url)) {

    Serial.println("ERROR: HTTP begin failed");

    return false;
  }


  // Follow Google's redirects
  http.setFollowRedirects(
    HTTPC_FORCE_FOLLOW_REDIRECTS
  );


  // ----------------------------------------------------
  // GET request
  // ----------------------------------------------------

  int httpCode = http.GET();


  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);


  // ----------------------------------------------------
  // Read response
  // ----------------------------------------------------

  if (httpCode > 0) {

    String response = http.getString();

    Serial.print("Google Response: ");
    Serial.println(response);


    http.end();


    if (httpCode >= 200 && httpCode < 400) {

      return true;

    }

    return false;
  }


  Serial.print("HTTP Error: ");
  Serial.println(
    http.errorToString(httpCode)
  );


  http.end();

  return false;
}


// ======================================================
// URL ENCODING
// ======================================================
//
// Converts characters such as spaces into:
//
// space → %20
//
// This is important for:
//
// "Anuj Kumar"
// "01 02 03 04"
// ======================================================

String urlEncode(String value) {

  String encoded = "";

  char c;

  char hex[] = "0123456789ABCDEF";


  for (int i = 0; i < value.length(); i++) {

    c = value.charAt(i);


    if (
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' ||
      c == '_' ||
      c == '.' ||
      c == '~'
    ) {

      encoded += c;

    } else {

      encoded += '%';

      encoded += hex[
        (c >> 4) & 0x0F
      ];

      encoded += hex[
        c & 0x0F
      ];
    }
  }


  return encoded;
}


// ======================================================
// GET RFID UID
// ======================================================

String getUID() {

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(
      rfid.uid.uidByte[i],
      HEX
    );

    if (i < rfid.uid.size - 1) {
      uid += " ";
    }
  }

  uid.toUpperCase();

  return uid;
}


// ======================================================
// FIND STUDENT
// ======================================================

int findStudent(String uid) {

  for (int i = 0; i < NUMBER_OF_STUDENTS; i++) {

    if (students[i].uid == uid) {
      return i;
    }
  }

  return -1;
}


// ======================================================
// GET CURRENT DATE
// ======================================================

String getDate() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {

    return "NO DATE";
  }


  char dateBuffer[20];

  strftime(
    dateBuffer,
    sizeof(dateBuffer),
    "%d/%m/%Y",
    &timeinfo
  );


  return String(dateBuffer);
}


// ======================================================
// GET CURRENT TIME
// ======================================================

String getTime() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {

    return "NO TIME";
  }


  char timeBuffer[20];

  strftime(
    timeBuffer,
    sizeof(timeBuffer),
    "%I:%M:%S %p",
    &timeinfo
  );


  return String(timeBuffer);
}


// ======================================================
// CONNECT TO WOKWI WIFI
// ======================================================

void connectWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD,
    6
  );


  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }


  Serial.println();

  Serial.println("WiFi connected!");

  Serial.print("IP address: ");

  Serial.println(WiFi.localIP());
}


// ======================================================
// SETUP NTP TIME
// ======================================================

void setupTime() {

  Serial.println("Synchronizing time...");


  // India timezone
  // IST = UTC+5:30
  configTime(
    19800,
    0,
    NTP_SERVER
  );


  struct tm timeinfo;


  while (!getLocalTime(&timeinfo)) {

    Serial.println("Waiting for time...");

    delay(500);
  }


  Serial.println("Time synchronized.");

  Serial.print("Current date: ");
  Serial.println(getDate());

  Serial.print("Current time: ");
  Serial.println(getTime());
}


// ======================================================
// READY SCREEN
// ======================================================

void showReadyScreen() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Scan Student");

  lcd.setCursor(0, 1);
  lcd.print("RFID Card...");
}


// ======================================================
// ENTRY DISPLAY
// ======================================================

void showEntry(Student &student) {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ENTRY GRANTED");

  lcd.setCursor(0, 1);
  lcd.print(student.name);

  delay(2000);


  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(student.entryDate);

  lcd.setCursor(0, 1);
  lcd.print(student.entryTime);
}


// ======================================================
// EXIT DISPLAY
// ======================================================

void showExit(Student &student) {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("EXIT RECORDED");

  lcd.setCursor(0, 1);
  lcd.print(student.name);

  delay(2000);


  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(student.exitDate);

  lcd.setCursor(0, 1);
  lcd.print(student.exitTime);
}


// ======================================================
// ENTRY BEEP
// ======================================================

void beepEntry() {

  tone(BUZZER_PIN, 2000);

  delay(150);

  noTone(BUZZER_PIN);
}


// ======================================================
// EXIT BEEP
// ======================================================

void beepExit() {

  tone(BUZZER_PIN, 2000);

  delay(150);

  noTone(BUZZER_PIN);

  delay(150);

  tone(BUZZER_PIN, 2000);

  delay(150);

  noTone(BUZZER_PIN);
}


// ======================================================
// ERROR BEEP
// ======================================================

void beepError() {

  tone(BUZZER_PIN, 500);

  delay(500);

  noTone(BUZZER_PIN);
}