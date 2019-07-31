#include <NewPing.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

// ultrasonic
#define ULTRASONIC_TRIG A0
#define ULTRASONIC_ECHO A1
#define ULTRASONIC_DELAY 2000
#define ULTRASONIC_SERVER_DELAY 600000 // 10 minutes
#define ULTRASONIC_FULL 10
#define ULTRASONIC_EMPTY 40

//NewPing WaterTracker(ULTRASONIC_TRIG, ULTRASONIC_ECHO, 400);
int waterLevel, waterLevelPreviousTime, waterLevelPreviousTimeServer;
long duration;
int distance;

// pump
#define PUMP_PIN 8

int isPumpOn = 0;

// rfid
#define SS_PIN 10
#define RST_PIN 9
#define ANIMAL_SERVER_DELAY 2000

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

String lastId = "";
int animalServerPreviousTime;
int tagCount1 = 0, tagCount2 = 0;
String cow1Id = "39 38 19 10", cow2Id = "8B A4 A8 1B";
// gsm

// We Created software serial object to communicate with SIM800L
SoftwareSerial mySerial(3, 2); //SIM800L Tx & Rx is connected to Arduino #3 & #2

void setup()
{
  Serial.begin(9600);
  // gsm
  mySerial.begin(9600);
  // ultrasonic
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  waterLevelPreviousTime = millis();
  waterLevelPreviousTimeServer = millis();
  // pump
  pinMode(PUMP_PIN, OUTPUT);
  // rfid
  animalServerPreviousTime = millis();
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  delay(4);
}

void loop()
{
  // ultrasonic
  updateWaterLevel();
  processTroughFullOrEmpty();
  // pump
  updatePumpState();
  // rfid
  checkForAnimal();
}

// ultrasonic
void updateWaterLevel()
{
  if (millis() - waterLevelPreviousTime > ULTRASONIC_DELAY)
  {
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    //sets the tri pin
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    //reads echo pin, returns the sound wave travel time
    duration = pulseIn(ULTRASONIC_ECHO, HIGH);
    distance = duration * 0.034 / 2;
    return distance;
    if (distance != 0)
      waterLevel = distance;
    waterLevelPreviousTime = millis();
  }
  if (millis() - waterLevelPreviousTimeServer > ULTRASONIC_SERVER_DELAY)
  {
    // send water level to server every ULTRASONIC_SERVER_DELAY milliseconds
    sendWaterLevel(waterLevel);
    waterLevelPreviousTimeServer = millis();
  }
}

void isTroughFull()
{
  return waterLevel <= ULTRASONIC_FULL;
}

void isTroughEmpty()
{
  return waterLevel >= ULTRASONIC_EMPTY;
}

void processTroughFullOrEmpty()
{
  if (isTroughFull)
  {
    isPumpOn = 0;
  }
  else if (isTroughEmpty)
  {
    isPumpOn = 1;
  }
}

// pump
void updatePumpState()
{
  if (isPumpOn)
    digitalWrite(PUMP_PIN, HIGH);
  else
    digitalWrite(PUMP_PIN, LOW);
}

// rfid
void checkForAnimal()
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  if (millis() - animalServerPreviousTime > ANIMAL_SERVER_DELAY)
  {
    animalServerPreviousTime = millis();
    // sendAnimalId(content.substring(1));
    if (content.substring(1) == cow1Id)
      tagCount1++;
    else if (content.substring(1) == cow2Id)
      tagCount2++;
    sendAnimalId(tagCount1);
    sendAnimalId(tagCount2);
  }
}

// gsm
void sendWaterLevel(int level)
{
  tosend(44);
  waterLevelField(level);
  endconnection();
}

void sendAnimalId(int id)
{
  tosend(12);
  animalIdField(id);
  endconnection();
}

void tosend(int charSize)
{

  mySerial.println("AT"); //Once the handshake test is successful, i t will back to OK
  delay(200);

  mySerial.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  delay(200);

  mySerial.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  delay(200);

  mySerial.println("AT+CPIN?");
  delay(200);

  mySerial.println("AT+CBC");
  delay(200);

  mySerial.println("AT+CREG?");
  delay(200);

  mySerial.println("AT+CGATT?");
  delay(200);

  mySerial.println("AT+CIPSHUT");
  delay(200);

  mySerial.println("AT+CIPSTATUS");
  delay(1000);

  mySerial.println("AT+CIPMUX=0 ");
  delay(1000);
  updateSerial();

  mySerial.println("AT+CSTT=\"MTN-UGANDA\",\"MTN-INTERNET\","
                   "");
  delay(200);
  updateSerial();

  mySerial.println("AT+CIICR"); //start wireless connection cellular network
  delay(2000);
  updateSerial();

  mySerial.println("AT+CIFSR"); //enquire regarding the IP address allocated
  delay(2000);
  updateSerial();

  mySerial.println("AT+CIPSTART=\"TCP\",\"184.106.153.149\",\"80\""); //connect to the ThingSpeak update URL (https://api.thingspeak.com)
  updateSerial();

  /*Serial.println("WRITE CIPSTART: ");
String command = Serial.readString();
while(Serial.available())
{
  
}
updateSerial();*/

  mySerial.println();
  String cmd = "AT+CIPSEND=" + String(charSize);
  mySerial.println(cmd); //declare the number of bytes (characters) I want to send
  updateSerial();
}
void waterLevelField(int fieldValue)
{
  String str = "GET update?api_key=KBFP2I9KYT77MC97&field1=" + String(fieldValue); // use API key for water level channel
  mySerial.println(str);                                                           //this is a constant beginning for the GET command and is as provided by ThingSpeak
  delay(4000);
  updateSerial();
  mySerial.println((char)26);
  delay(4000);
  updateSerial();
  mySerial.println();
}

void animalIdField(int fieldValue)
{
  String str = "GET update?api_key=3U8EWAGSE1G3J9TY&field1=" + String(fieldValue); // use API key for animal id channel
  mySerial.println(str);                                                           //this is a constant beginning for the GET command and is as provided by ThingSpeak
  delay(4000);
  updateSerial();
  mySerial.println((char)26);
  delay(4000);
  updateSerial();
  mySerial.println();
}

void endconnection()
{
  mySerial.println("AT+CIPSHUT");
  delay(200);
  updateSerial();
}
void updateSerial()
{
  //delay(2500); // commented by Terry
  while (Serial.available())
  {
    mySerial.write(Serial.read()); //Forward what Serial received to Software Serial Port
  }
  while (mySerial.available())
  {
    Serial.write(mySerial.read()); //Forward what Software Serial received to Serial Port
  }
}
