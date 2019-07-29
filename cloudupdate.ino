#define MAX_WATER_LEVEL 5
#define MIN_WATER_LEVEL 100

#include <SPI.h>
#include <NewPing.h>
#include <MFRC522.h>
#include <Sim800L.h>
#include <GSMSim.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include <gprs.h>

#define SS_PIN 10
#define RST_PIN 9
#define valve_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
SoftwareSerial mySerial(3, 2); //SIM800L Tx & Rx is connected to Arduino #3 & #2

int trig = 6;
int echo = 7;
//int GREEN_LED = 11, RED_LED = 12;
NewPing Sonar(trig, echo, 400);
int isWaterSupplyOn = 0;
int waterSupplyWasStarted = 0, waterSupplyWasStopped = 0;
float waterLevel;
String cow1Id = "39 38 19 10", cow2Id = "8B A4 A8 1B";
String value_one;
float value_two;
int net_status;
int firsttime = 0;

void setup()
{
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);       
   
  pinMode(valve_PIN, OUTPUT);
  //pinMode(GREEN_LED, OUTPUT);
  //pinMode(RED_LED, OUTPUT);
  SPI.begin();        // Initiate  SPI bus
  mfrc522.PCD_Init(); // Initiate MFRC522
  Serial.println();
  waterLevel = getWaterLevel();
}

void loop()
{ // put your main code here, to run repeatedly:
  updateWaterLevel();
  updateWaterSupply();
  checkForNewAnimal();
  delaying();
}

// Esther
float getWaterLevel()
{
  return Sonar.ping_cm();
  ;
}

// Esther
// start or stop water supply depending on water level
void updateWaterLevel()
{
  waterLevel = getWaterLevel();
  //logFloat(waterLevel);
  if (waterLevel < MAX_WATER_LEVEL && waterLevel > 0)
  {
    stopWaterSupply();
  }
  else if (waterLevel > MIN_WATER_LEVEL && waterLevel > 0)
  {
    startWaterSupply();
  }
}

void startWaterSupply()
{
  if (!waterSupplyWasStarted)
  {
    //digitalWrite(GREEN_LED, HIGH);
    //digitalWrite(RED_LED, LOW);
    isWaterSupplyOn = 1;
    logString("started water supply");
    waterSupplyWasStarted = 1;
    waterSupplyWasStopped = 0;
  }
}

void stopWaterSupply()
{
  if (!waterSupplyWasStopped)
  {
    //digitalWrite(GREEN_LED, LOW);
    //digitalWrite(RED_LED, HIGH);
    isWaterSupplyOn = 0;
    logString("stopped water supply");
    waterSupplyWasStopped = 1;
    waterSupplyWasStarted = 0;
  }
}

// Tony
void updateWaterSupply()
{
  if (isWaterSupplyOn)
  {
    //keep the water coming
    openvalve();
  }
  else
  {
    closevalve();
  }
}

// Terry
void checkForNewAnimal()
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
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == cow1Id)
  {
    Serial.println("Cow one detected");
    value_one = cow1Id;//assign cow 1 id to value_one
    Serial.println();
    delay(500);
  }

  else if (content.substring(1) == cow2Id)
  {
    Serial.println("Cow two detected");
    value_one = cow2Id;//assign cow 2 id to value_one
    Serial.println();
    delay(500);
  }
  //recordAnimal(content.substring(1));
}

void logString(char *message)
{
  Serial.println(message);
  Serial.println();
}

void logFloat(float number)
{
  Serial.println(number);
  value_two = number;//getting the water_level
  Serial.println();
}
void openvalve() {
  digitalWrite(valve_PIN, HIGH);
}
void closevalve() {
  digitalWrite(valve_PIN, LOW);
}

// Arnold
void delaying(){
  if( firsttime == 0 ){
    cloudupdate();
    firsttime++;//5 minute delay starts
  }
  else{
    delay(300000);
    cloudupdate();
  }
}

void cloudupdate() {
  //value_one = animalId;
  Serial.println(" ");
  Serial.println(" ");
  Serial.println("Initializing... To make cloud backup ");    //Display in Serial Monitor
  delay(1000);
  mySerial.println("AT");
  updateSerial();
  mySerial.println("AT+CFUN=0");  //Turn it off
  updateSerial();
  mySerial.println("AT+CFUN=1");  //Turn it on
  updateSerial();
  mySerial.println("AT+CIPSHUT"); //Shutdown any connections
  updateSerial();
  mySerial.println("AT+CIPMUX=0"); //Enable wireless communication
  updateSerial();
  mySerial.println("AT+CGATT=1"); //Activate GPRS connection
  updateSerial();
  mySerial.println("AT+CSTT=\"INTERNET\",\"\",\"\""); //Access point connection //Successful steps from this point causes successful cloud update
  updateSerial();
  mySerial.println("AT+CIICR"); //Get IP address //This also changes the blinking of the module
  updateSerial();
  mySerial.println("AT+CIFSR"); //Activate IP address
  updateSerial();
    
    //Accessing the cloud resource
    mySerial.println("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80"); //Thingspeak.com Server connection
    updateSerial();
    mySerial.println("AT+CIPSEND=45"); //Bytes to be sent to the cloud 44-two, 45-three, 43-single
    updateSerial();

    mySerial.println("GET /update?key=KBFP2I9KYT77MC97&field1="+value_one+"&field2="+value_two);
                
    //mySerial.print("GET /update?api_key=KBFP2I9KYT77MC97&field1=");

    //mySerial.print("api.thingspeak.com/update?api_key=xxxxxxxxxxxxxxxx&field1="); 
    //---------------------------------------------------------------//
    //mySerial.print("50");
    //mySerial.println("\r\n");

    //  mySerial.println("GET https://api.thingspeak.com/update?api_key=UUFDD9H3Z3DZC4VL&field1=22\r\n\r\n");

    //mySerial.print("api.thingspeak.com/update?api_key=KBFP2I9KYT77MC97&field1="); 
    //mySerial.print("GET /update?api_key=KBFP2I9KYT77MC97&field1=");
    //---------------------------------------------------------------//
    //mySerial.print(value_one); //>>>>>>  variable 1 (RFID_tag)
    //mySerial.print("&field2=");
    //mySerial.print(value_two); //>>>>>> variable 2 (water_level)

    updateSerial();
  }

  void updateSerial()
  {
    delay(2000);   //Adjust this value to a smaller value if neccesary e.g 500
    while (Serial.available())
    {
      mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
    }
    while (mySerial.available())
    {
      Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
    }
  }
