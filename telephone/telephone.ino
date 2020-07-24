#include <SoftwareSerial.h>


#include "sound.h"


////////////////////////////////////////////////
//PINS

//ENCODER PINS
#define CS                2
#define ENC               3

//MOSFET AUDIO CHANNEL
#define PIN1_SIM900       A0
#define PIN2_SIM900       A1
#define PIN1_ARDUINO      A3
#define PIN2_ARDUINO      A2

//SWITCH PIN
#define SWB               4
#define PSW               5

//RING PINS(MOSFET H-BRIDGE)
#define RG1               8
#define RG2               9

//SoftwareSerial

#define RX_PIN            7
#define TX_PIN            6

/////////////////////////////////////////////





// Configure software serial port
SoftwareSerial phone(RX_PIN, TX_PIN); 



#define RINGING_TIMEOUT       10000   //10000 ms





enum stc {

NO_CALL = 0,
CALLING,
RECEIVE_CALL,
  
};




uint8_t stateCall = NO_CALL;

int8_t n = -1;
bool csFlag = false;
bool encFlag = false;
char number[20];
uint8_t AP_NUMBER = 0;

bool swFlag = false;

uint32_t timerBase;

char cmdBuffer[50]; //set Big buffer
uint8_t stateCmd = 0;

uint8_t messageCounter = 0;//cmdBuffer pointer




bool setRing = false;
uint32_t timerRing;
uint32_t timerTone;
uint16_t counterTone = 0;
uint8_t counterRing;
bool flagMsg = false;


uint32_t ringTimeOut;
uint32_t debugTime;

//sound part

uint16_t sndCounter = 0;






//prototypes
char encoder_read(void);
void ringing(uint8_t pin1, uint8_t pin2);
void makeCall(char *pNumber);
void closeCall(void);







void setup() {

Serial.begin(9600);

Serial.println("Cell Phone base System v1");

phone.begin(9600);

pinMode(CS,2);
pinMode(ENC,2);
pinMode(SWB,2);
pinMode(RG1,2);
pinMode(RG2,2);

pinMode(PSW,1);

//enable channels
pinMode(PIN1_SIM900, 1);
pinMode(PIN2_SIM900, 1);
pinMode(PIN1_ARDUINO, 1);
pinMode(PIN2_ARDUINO, 1);

digitalWrite(PIN1_SIM900,0);
digitalWrite(PIN2_SIM900,0);
digitalWrite(PIN1_ARDUINO,1);
digitalWrite(PIN2_ARDUINO,1);

//Set fast pwm on D11 by using  Timer/Counter 2(8bit) registers
//See Atmega328P datasheet for more info (p.141)

//Set fast pwm mode // Didn't set Timer Interrupt, Don't need it
TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20); 
TCCR2B = _BV(CS20); //no prescaler

pinMode(11,1); //Set D11 as output


counterRing = 42;

//turn on SIM900
digitalWrite(PSW, 1);
delay(2000);
digitalWrite(PSW, 0);
delay(5000);


for(uint8_t i = 0; i < 20; i++){
  number[i] = 0;    //clear buffer
}
delay(10000);
phone.println("AT+CLIP=1\r"); // turn on caller ID notification
OCR2A = 128; //Set middle level
//add error control
Serial.println("init done");

}

void loop() {


if(millis() - timerBase >= 10){//base time to 10ms

  while(phone.available() > 0){ //if we received a message from SIM900
  
  cmdBuffer[messageCounter] = phone.read();
  
  messageCounter++;
  if(messageCounter >= 50){//to big
    messageCounter = 0; //let's back to 0
  }
  flagMsg = true; //send a flag to read
  delay(10);//set base 10ms delay to avoid read buffer error !
  }

  if(flagMsg){

    

stateCmd = 0;

for(uint8_t i = 0; i < 20; i++){
  Serial.println((char)cmdBuffer[i]);

switch(stateCmd){

case 0:
if(cmdBuffer[i] == 'R'){
  stateCmd = 1;
}

if(cmdBuffer[i] == 'O'){
  stateCmd = 5;
}

if(cmdBuffer[i] == 'N'){
  stateCmd = 10;
}
break;

case 1:
if(cmdBuffer[i] == 'I'){
  stateCmd = 2;
}
break;

case 2:
if(cmdBuffer[i] == 'N'){
  stateCmd = 3;
}
break;

case 3:
if(cmdBuffer[i] == 'G'){
  stateCmd = 4;
}
break;

case 5:
if(cmdBuffer[i] == 'K'){
  stateCmd = 6;
}
break;

case 7:
if(cmdBuffer[i] == 'O'){
  stateCmd = 8;
}
break;

case 8:
if(cmdBuffer[i] == ' '){
  stateCmd = 9;
}
break;

case 9:
if(cmdBuffer[i] == 'C'){
  stateCmd = 10;
}
break;

case 10:
if(cmdBuffer[i] == 'A'){
  stateCmd = 11;
}
break;

case 11:
if(cmdBuffer[i] == 'R'){
  stateCmd = 12;
}
break;

case 12:
if(cmdBuffer[i] == 'R'){
  stateCmd = 13;
}
break;

case 13:
if(cmdBuffer[i] == 'I'){
  stateCmd = 14;
}
break;

case 14:
if(cmdBuffer[i] == 'E'){
  stateCmd = 15;
}
break;

case 15:
if(cmdBuffer[i] == 'R'){
  stateCmd = 16;
}
break;




  
}
  
}


//if RING\r
  //if(cmdBuffer[0] == 'R' && cmdBuffer[1] == 'I' && cmdBuffer[2] == 'N' && cmdBuffer[3] == 'G'){ //call reiceived
   
  if(stateCmd == 4){ 
   Serial.println("ring");
   ringTimeOut = millis();
   if(!setRing){
   setRing = true;
   }
   if(stateCall == NO_CALL){
   stateCall = RECEIVE_CALL;
   }
  }

  //if OK\r
 // if(cmdBuffer[0] == 'O' && cmdBuffer[1] == 'K'){ //call ok
  if(stateCmd == 6){
   Serial.println("ok");
  }

  //if NO CARRIER\r
 // if(cmdBuffer[0] == 'N' && cmdBuffer[1] == 'O' && cmdBuffer[2] == ' ' && cmdBuffer[3] == 'C' && cmdBuffer[4] == 'A' && cmdBuffer[5] == 'R' && cmdBuffer[6] == 'R' && cmdBuffer[7] == 'I' && cmdBuffer[8] == 'E' && cmdBuffer[9] == 'R'){ //call closed by other phone
  if(stateCmd == 16){ 
   Serial.println("ok");
   stateCall = NO_CALL;
   closeCall();
   digitalWrite(PIN1_SIM900,0);
   digitalWrite(PIN2_SIM900,0);
   digitalWrite(PIN1_ARDUINO,1);
   digitalWrite(PIN2_ARDUINO,1);
  }

stateCmd = 0; //back to 0 to get next cmd
  
messageCounter = 0;
  for(uint8_t i = 0; i < 50; i++){ //void cmd buffer
    cmdBuffer[i] = 0;
  }

//Serial.println();
flagMsg = false;
  }

  
  if(stateCall == NO_CALL){//ensure we don't send numbers during call

if(!digitalRead(CS)){ //if latch
  csFlag = true; //set read encoder enabled
  //n = 0;
  }


if(csFlag == true){//if read enabled
  if(!digitalRead(ENC) && encFlag == false){//if detected pulse
    n++; //increment counter
    //Serial.println(n);
    encFlag = true; //set flag
  }
  if(digitalRead(ENC)){//if pulse go high
    encFlag = false;//reset flag
  }
}

 if(digitalRead(CS) && csFlag == true){//if not latch

//read encoder
number[AP_NUMBER] = encoder_read();
AP_NUMBER++; 
}

  }

if(!digitalRead(SWB) && !swFlag){ //prepare to do something when switch pulled out
  swFlag = true;
}

if(digitalRead(SWB) && swFlag){//do something
  swFlag = false;

switch(stateCall){

case NO_CALL:

//go in calling state and make a voice call
makeCall(number);
stateCall = CALLING;
//clear number buffer for next call
for(uint8_t i = 0; i < 20; i++){
  number[i] = 0;
}
AP_NUMBER = 0;
Serial.println("CALLING");
digitalWrite(PIN1_SIM900,1);
digitalWrite(PIN2_SIM900,1);
digitalWrite(PIN1_ARDUINO,0);
digitalWrite(PIN2_ARDUINO,0);
setRing = false;
break;

case CALLING:

//go in no call state and close call
closeCall();
stateCall = NO_CALL;
Serial.println("NO_CALL");
digitalWrite(PIN1_SIM900,0);
digitalWrite(PIN2_SIM900,0);
digitalWrite(PIN1_ARDUINO,1);
digitalWrite(PIN2_ARDUINO,1);
break;

case RECEIVE_CALL:

  // answer the phone
       phone.print("ATA\r");
       stateCall = CALLING;
       counterRing = 42;
      delay(1000);
      digitalWrite(PIN1_SIM900,1);
      digitalWrite(PIN2_SIM900,1);
      digitalWrite(PIN1_ARDUINO,0);
      digitalWrite(PIN2_ARDUINO,0);

      
break;

  
}
  
}


if(stateCall == RECEIVE_CALL){
  if(setRing){
    //clear number buffer for next call
for(uint8_t i = 0; i < 20; i++){
  number[i] = 0;
}
    counterRing = 0;
    setRing = false;
    
  }
}


if(millis() - debugTime >= 2000){

  switch(stateCall){

  case NO_CALL:
  Serial.println("NO_CALL");
  break;
  
  case CALLING:
  Serial.println("CALLING");
  break;

  case RECEIVE_CALL:
  Serial.println("RECEIVE_CALL");
  break;

    
  }

  debugTime = millis();
}

if(millis() - ringTimeOut >= RINGING_TIMEOUT && stateCall == RECEIVE_CALL){ //if timeout, that mean user failed to get phone at time and it passed about 10 sec since the last ring
  stateCall = NO_CALL;
  Serial.println("Go in no call");
}

/*Serial.print(digitalRead(ENC));
Serial.print("\t");
Serial.println(digitalRead(CS));*/

timerBase = millis();
}

//ringing function
ringing(RG1, RG2);

if(stateCall == NO_CALL){
playPhoneTone(65535);
}

}


















void closeCall(void){

Serial.println("call closed");
phone.println("ATH"); // hang up
delay(1000);
}


void makeCall(char *pNumber){
 
  char pBuffer[30];//char buffer max 30 characters

for(uint8_t i = 0; i < 255; i++){
  playPhoneTone(65535);
}

  for(uint8_t i = 0; i < 20; i++){

   if(pNumber[i] >= 48 &&  pNumber[i] <= 57){
    playDTMF(pNumber[i]-48);
    delay(100);
   }
    
  }
  
  sprintf(pBuffer,"ATD + +%s;",pNumber); //combine it  with ATD cmd(to make call)
  
  phone.println(pBuffer);
  delay(100);
  phone.println();
  Serial.print("\n");
  Serial.println(pBuffer); //print what phone number we want to call
}




char encoder_read(void){

char CHR_NUMBER;
 
    if(n >= 10){ //if counter get 10 or more(more == bug set to zero automatically and increment again)
      n = 0; //user sent 0 number
    }
    //Serial.print("\n number : ");
  //Serial.print(n);
  playDTMF(n);
  delay(100);
  playNumber(n);
  CHR_NUMBER = 48 + n; //48 in decimal == 0 in character
  n = 0; //set to -1 to avoid start pulse of our encoder(if there not a start pulse, set it to 0)
  csFlag = false; //set read encoder disabled
  delay(100);

  Serial.print(CHR_NUMBER);
return CHR_NUMBER; // return char number
  
  
}





void ringing(uint8_t pin1, uint8_t pin2){



if(counterRing < 42){
if(millis() - timerRing < 32){
digitalWrite(pin1,1);
digitalWrite(pin2,0);
}
if(millis() - timerRing >= 32){
digitalWrite(pin1,0);
digitalWrite(pin2,1);
}
if(millis() - timerRing >= 64){
 counterRing++;
  timerRing = millis();
}
}



}



void playNumber(uint8_t inputValue){



switch(inputValue){


case 0:
for(sndCounter = 0; sndCounter < sizeof(zero); sndCounter++){
  OCR2A = pgm_read_byte(&zero[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 1:
for(sndCounter = 0; sndCounter < sizeof(one); sndCounter++){
  OCR2A = pgm_read_byte(&one[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 2:
for(sndCounter = 0; sndCounter < sizeof(two); sndCounter++){
  OCR2A = pgm_read_byte(&two[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 3:
for(sndCounter = 0; sndCounter < sizeof(three); sndCounter++){
  OCR2A = pgm_read_byte(&three[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 4:
for(sndCounter = 0; sndCounter < sizeof(four); sndCounter++){
  OCR2A = pgm_read_byte(&four[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 5:
for(sndCounter = 0; sndCounter < sizeof(five); sndCounter++){
  OCR2A = pgm_read_byte(&five[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 6:
for(sndCounter = 0; sndCounter < sizeof(six); sndCounter++){
  OCR2A = pgm_read_byte(&six[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 7:
for(sndCounter = 0; sndCounter < sizeof(seven); sndCounter++){
  OCR2A = pgm_read_byte(&seven[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 8:
for(sndCounter = 0; sndCounter < sizeof(eight); sndCounter++){
  OCR2A = pgm_read_byte(&eight[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;

case 9:
for(sndCounter = 0; sndCounter < sizeof(nine); sndCounter++){
  OCR2A = pgm_read_byte(&nine[sndCounter]); //send array value to Output compare register
  delayMicroseconds(200); // 1/5000 Hz ---> sampling rate of sounds
}
break;


  
}

  
}


void playPhoneTone(uint16_t duration){

//Period of counter process in count*188us !
//example 5000 count => 940000us ===> 940000/2 = 470000us tone => 470000us notone !

if(duration > 0){

 
if(micros() - timerTone >= 188){

if(counterTone <= duration/2){
  OCR2A = pgm_read_byte(&hz440[sndCounter]); //send array value to Output compare register
}

if(counterTone >= duration){
  counterTone = 0;
}
 
  sndCounter++;
  if(sndCounter >= sizeof(hz440)){
    sndCounter = 0;
  }

  if(duration < 65535){
  counterTone++;
  }
  
  timerTone = micros();
}


}
  
}



void playDTMF(uint8_t inputValue){



switch(inputValue){


case 0:
for(sndCounter = 0; sndCounter < sizeof(dtmf0); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf0[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 1:
for(sndCounter = 0; sndCounter < sizeof(dtmf1); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf1[sndCounter]); //send array value to Output compare register
  delayMicroseconds(400); // 1/2500 Hz ---> sampling rate of sounds
}
break;

case 2:
for(sndCounter = 0; sndCounter < sizeof(dtmf2); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf2[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 3:
for(sndCounter = 0; sndCounter < sizeof(dtmf3); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf3[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 4:
for(sndCounter = 0; sndCounter < sizeof(dtmf4); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf4[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 5:
for(sndCounter = 0; sndCounter < sizeof(dtmf5); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf5[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 6:
for(sndCounter = 0; sndCounter < sizeof(dtmf6); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf6[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 7:
for(sndCounter = 0; sndCounter < sizeof(dtmf7); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf7[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 8:
for(sndCounter = 0; sndCounter < sizeof(dtmf8); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf8[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;

case 9:
for(sndCounter = 0; sndCounter < sizeof(dtmf9); sndCounter++){
  OCR2A = pgm_read_byte(&dtmf9[sndCounter]); //send array value to Output compare register
  delayMicroseconds(333); // 1/3000 Hz ---> sampling rate of sounds
}
break;
  
}


  
}

