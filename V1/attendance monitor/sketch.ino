#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

// ================= RFID =================
#define SS_PIN   21
#define RST_PIN  22

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= BUZZER =================
#define BUZZER_PIN 15

// ================= LCD =================
// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(27, 26, 25, 33, 32, 4);


// =====================================================
// STUDENT DATA
// Replace these UID values with your actual RFID UID
// =====================================================

struct Student {
  String uid;
  String name;
  String rollNo;
  String branch;
};

// Example students
Student students[] = {
  {"A1 B2 C3 D4", "Anuj", "101", "AR"},
  {"11 22 33 44", "Rahul", "102", "AR"},
  {"55 66 77 88", "Aman", "103", "AR"}
};

int numberOfStudents = sizeof(students) / sizeof(students[0]);


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // LCD
  lcd.begin(16, 2);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID Attendance");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);

  // RFID
  SPI.begin();

  rfid.PCD_Init();

  delay(100);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID Card");
  lcd.setCursor(0, 1);
  lcd.print("Ready...");

  Serial.println("================================");
  Serial.println("RFID Attendance System");
  Serial.println("Scan your RFID card...");
  Serial.println("================================");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Check if a new card is present
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read the card
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Get UID
  String uid = getUID();

  Serial.print("Card UID: ");
  Serial.println(uid);

  // Check whether student exists
  int studentIndex = findStudent(uid);

  lcd.clear();

  if (studentIndex != -1) {

    // ==========================
    // STUDENT FOUND
    // ==========================

    Serial.println("Student Found!");
    Serial.print("Name: ");
    Serial.println(students[studentIndex].name);

    Serial.print("Roll No: ");
    Serial.println(students[studentIndex].rollNo);

    Serial.print("Branch: ");
    Serial.println(students[studentIndex].branch);

    // Buzzer confirmation
    beep(1);

    lcd.setCursor(0, 0);
    lcd.print("Access Granted");

    lcd.setCursor(0, 1);
    lcd.print(students[studentIndex].name);

    delay(2000);

    // Show roll number
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Roll: ");
    lcd.print(students[studentIndex].rollNo);

    lcd.setCursor(0, 1);
    lcd.print(students[studentIndex].branch);

    delay(2000);

  } else {

    // ==========================
    // UNKNOWN CARD
    // ==========================

    Serial.println("Unknown RFID Card!");

    // Error buzzer
    beep(2);

    lcd.setCursor(0, 0);
    lcd.print("Unknown Card");

    lcd.setCursor(0, 1);
    lcd.print("Access Denied");

    delay(2500);
  }

  // Stop communication with card
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Return to waiting screen
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID Card");

  lcd.setCursor(0, 1);
  lcd.print("Ready...");
}


// =====================================================
// GET RFID UID
// =====================================================

String getUID() {

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      uid += " ";
    }
  }

  uid.toUpperCase();

  return uid;
}


// =====================================================
// FIND STUDENT
// =====================================================

int findStudent(String uid) {

  for (int i = 0; i < numberOfStudents; i++) {

    if (students[i].uid == uid) {
      return i;
    }
  }

  return -1;
}


// =====================================================
// BUZZER
// =====================================================

void beep(int times) {

  for (int i = 0; i < times; i++) {

    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);

    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
  }
}