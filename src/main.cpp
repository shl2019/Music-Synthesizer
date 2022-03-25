#define _USE_MATH_DEFINES
#include <Arduino.h>
#include <U8g2lib.h>
#include <cmath>
#include <math.h>
#include <list>
#include <STM32FreeRTOS.h>
#include <string>
#include <knobClass.hpp>
#include <ES_CAN.h>
using namespace std;

//Constants
  const uint32_t interval = 100; //Display update interval

//Pin definitions
  //Row select and enable
  const int RA0_PIN = D3;
  const int RA1_PIN = D6;
  const int RA2_PIN = D12;
  const int REN_PIN = A5;

  //Matrix input and output
  const int C0_PIN = A2;
  const int C1_PIN = D9;
  const int C2_PIN = A6;
  const int C3_PIN = D1;
  const int OUT_PIN = D11;

  //Audio analogue out
  const int OUTL_PIN = A4;
  const int OUTR_PIN = A3;

  //Joystick analogue in
  const int JOYY_PIN = A0;
  const int JOYX_PIN = A1;

  //Output multiplexer bits
  const int DEN_BIT = 3;
  const int DRST_BIT = 4;
  const int HKOW_BIT = 5;
  const int HKOE_BIT = 6;

// se the stepsize of each tone
  const uint32_t stepSizes[] = {0,(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),9)),(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),8)),
                              (int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),7)),(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),6)),
                              (int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),5)),(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),4)),
                              (int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),3)),(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),2)),
                              (int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),1)),(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),0)),
                              (int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),-1)),(int)((pow(2,32)*440.0/22000.0)/pow(pow(2,1/12.0),-2))
  };
  volatile int32_t currentStepSizeList[12];
  volatile int8_t currentKeysNumber;
  volatile uint8_t keyArray[7];
  int32_t sineStep[12][100];
  float Period[12];
  float ShiftedPeriod[12];
  float NoteFrequencyTable[12] = {261.63,277.18,293.66,311.13,329.63,349.23,369.99,392.0,415.30,440,466.16,493.88};
  int8_t currentKey[12];
  knob knob3;
  knob knob2;
  knob knob1;
  knob knob0; 
  int8_t knob0s;
  int8_t knobs = 1;
  double joyX;
  double joyY;
  double shifts; //the tune will change according to this value
  double shifts2;
  volatile int8_t currentVolume = 0;
  int32_t joyStickOffset;
  SemaphoreHandle_t keyArrayMutex;
  int8_t SecondKeysnumber;
  int tmp = 0;
  

  //set TX and RX
  uint8_t TX_Message[16] = {0};
  uint8_t RX_Message[16] = {0};

  // initalise queue handlers
  QueueHandle_t msgInQ;
  QueueHandle_t msgOutQ;

  //set the semaphore
  SemaphoreHandle_t CAN_TX_Semaphore;

  //Display driver object
  U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

  //Function to set outputs using key matrix
  void setOutMuxBit(const uint8_t bitIdx, const bool value) {
        digitalWrite(REN_PIN,LOW);
        digitalWrite(RA0_PIN, bitIdx & 0x01);
        digitalWrite(RA1_PIN, bitIdx & 0x02);
        digitalWrite(RA2_PIN, bitIdx & 0x04);
        digitalWrite(OUT_PIN,value);
        digitalWrite(REN_PIN,HIGH);
        delayMicroseconds(2);
        digitalWrite(REN_PIN,LOW);
    }
  int show,show2,show3;
  int32_t generateSawtoothWave(int32_t *phaseAcc){
    int32_t Vout=0;
      for(int i = 0; i<(currentKeysNumber-SecondKeysnumber);i++){
            phaseAcc[i] += (int)((currentStepSizeList[i]*shifts));
            Vout += phaseAcc[i]>>25;
        }
      for(int i = (currentKeysNumber-SecondKeysnumber);i<currentKeysNumber;i++){
        phaseAcc[i] += (int)(currentStepSizeList[i]*shifts2);
        Vout += phaseAcc[i]>>25;
      }

        Vout = Vout >> (8 - knob3.getVolume()/2);
        return Vout;
    }

int32_t generateSineWave(){
  static int8_t step[12];
  for(int i= 0;i<12;i++){
    if(step[i]>=ShiftedPeriod[i]){
      step[i] = 0;
  }

  }
  int32_t Vout=0;
  for(int i = 0; i<currentKeysNumber;i++){
        Vout += sineStep[currentKey[i]-1][step[currentKey[i]-1]];
        step[currentKey[i]-1]+=1;   
    }
    if(currentKeysNumber!=0&&currentKeysNumber!=1){
    Vout = (int)(Vout/(currentKeysNumber*0.9));}

    Vout = Vout >> (8 - knob3.getVolume()/2);
    return Vout;

}
int32_t generateSquareWave(){
  static int8_t step[12];
  int32_t Vout=0;
  
  for(int i= 0;i<12;i++){
      if(step[i]>ShiftedPeriod[i]){
      step[i] = 0;
    }
  }
  for(int i = 0; i<currentKeysNumber;i++){
      if(step[currentKey[i]-1]>ShiftedPeriod[i]/2){
        Vout += 128;
      }else{
        Vout += -128;
      }
        step[currentKey[i]-1]+=1;   
    }
    
    Vout = Vout >> (8 - knob3.getVolume()/2);
    return Vout;
    
}

void sampleISR(){

    static int32_t phaseAcc[12];
    int32_t Vout=0;
    for(int i = currentKeysNumber;i<12;i++){
        phaseAcc[i] = 0;
    }
    if(knob2.getVolume()==0){                     //choose SawtoothWave
        Vout = generateSawtoothWave(phaseAcc);
    }else if(knob2.getVolume()==1){
        Vout = generateSineWave();
        
    }else if(knob2.getVolume()==2){
        Vout = generateSquareWave();
    }

    if(Vout>127){
      Vout = 127;
    }else if(Vout<-128){
      Vout = -128;
    }
    if(knob0s==0){
    analogWrite(OUTR_PIN, Vout + 128);
    }
    return;

}


void CAN_RX_ISR (void) {
    uint8_t RX_Message_ISR[16];
    uint32_t ID;
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
    // Serial.print("xd");
    // Serial.println(RX_Message_ISR[15]);
}

void CAN_TX_ISR (void) {
    xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}

void scanKeysTask(void * pvParameters);
void displayUpdateTask(void * pvParameters);
void decodeTask(void * pvParameters);
void CAN_TX_Task(void * pvParameters);

void setup() {

joyX = analogRead(JOYX_PIN);
joyY = analogRead(JOYY_PIN);
joyStickOffset = (int)(joyX+joyY);

for(int j = 0;j<12;j++){
  for(int i =0;i<100;i++){
        sineStep[j][i] = (int)128*sin(2*(NoteFrequencyTable[j]/22000)*PI*i);
  }
  Period[j] = 22000/NoteFrequencyTable[j];
}

  knob2.setScale(2);  //set scale to 4
  knob2.setVolume(0); //set initial timbre 
  knob1.setScale(5);   // provide 6 scale for note frequecny
  knob1.setVolume(3);   //set initial octave
  knob3.setVolume(13); //set initial valume to 16
  //knob1.setVolume(3);
  // put your setup code here, to run once:

  //Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  //Initialise CAN
  CAN_Init(false);
  setCANFilter(0x123,0x7ff);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  CAN_RegisterTX_ISR(CAN_TX_ISR);
  CAN_Start();

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  //Initialise UART
  Serial.begin(9600);
  //Serial.println("Hello World");
  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);
  sampleTimer->setOverflow(22000, HERTZ_FORMAT);
  sampleTimer->attachInterrupt(sampleISR);
  sampleTimer->resume();
  TaskHandle_t scanKeysHandle = NULL;
  TaskHandle_t displayUpdateHandle = NULL;
  keyArrayMutex = xSemaphoreCreateMutex();

  //Initialise the semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);

  xTaskCreate(
    
    scanKeysTask,/* Function that implements the task */
    "scanKeys",/* Text name for the task */
    512,      /* Stack size in words, not bytes*/
    NULL,/* Parameter passed into the task */
    2,/* Task priority*/
    &scanKeysHandle);  /* Pointer to store the task handle*/

    //Initialise Queue handlers
     msgInQ = xQueueCreate(64,16);
     msgOutQ = xQueueCreate(64,16);
  

  xTaskCreate(
    decodeTask,/* Function that implements the task */
    "decodeTask",/* Text name for the task */
    64,      /* Stack size in words, not bytes*/
    NULL,/* Parameter passed into the task */
    1,/* Task priority*/
    &scanKeysHandle);  /* Pointer to store the task handle*/

  xTaskCreate(
    CAN_TX_Task ,/* Function that implements the task */
    "CAN_TX_Task",/* Text name for the task */
    258,      /* Stack size in words, not bytes*/
    NULL,/* Parameter passed into the task */
    3,/* Task priority*/
    &scanKeysHandle);  /* Pointer to store the task handle*/

  xTaskCreate(
    displayUpdateTask,/* Function that implements the task */
    "displayUpdate",/* Text name for the task */
    512,      /* Stack size in words, not bytes*/
    NULL,/* Parameter passed into the task */
    1,/* Task priority*/
    &scanKeysHandle);  /* Pointer to store the task handle*/

 
    
    vTaskStartScheduler();
}



uint8_t readCols(){
    int c0 = digitalRead(C0_PIN);
    int c1 = digitalRead(C1_PIN);
    int c2 = digitalRead(C2_PIN);
    int c3 = digitalRead(C3_PIN);
    uint8_t u8 = c0+c1*2+c2*4+c3*8;
    return u8;
}

void setRow(uint8_t rowIdx){
    digitalWrite(REN_PIN, LOW);
    if(rowIdx == 0){
      digitalWrite(RA0_PIN, LOW);
      digitalWrite(RA1_PIN, LOW);
      digitalWrite(RA2_PIN, LOW);
    }
      else if(rowIdx == 1){
      digitalWrite(RA0_PIN, HIGH);
      digitalWrite(RA1_PIN, LOW);
      digitalWrite(RA2_PIN, LOW);

    }
      else if(rowIdx == 2){
      digitalWrite(RA0_PIN, LOW);
      digitalWrite(RA1_PIN, HIGH);
      digitalWrite(RA2_PIN, LOW);

    }
      else if(rowIdx == 3){
      digitalWrite(RA0_PIN, HIGH);
      digitalWrite(RA1_PIN, HIGH);
      digitalWrite(RA2_PIN, LOW);

    }
      else if(rowIdx == 4){
      digitalWrite(RA0_PIN, LOW);
      digitalWrite(RA1_PIN, LOW);
      digitalWrite(RA2_PIN, HIGH);

    }
      else if(rowIdx == 5){
      digitalWrite(RA0_PIN, HIGH);
      digitalWrite(RA1_PIN, LOW);
      digitalWrite(RA2_PIN, HIGH);

    }
      else if(rowIdx == 6){
      digitalWrite(RA0_PIN, LOW);
      digitalWrite(RA1_PIN, HIGH);
      digitalWrite(RA2_PIN, HIGH);

    }
    digitalWrite(REN_PIN, HIGH);
}

int find_rotation_variable(int previous_A, int previouos_B, int current_A, int current_B,int currentVolume){
    if(previous_A!=current_A && previouos_B!=current_B){
      return currentVolume*2;
    }else if(previous_A == 0 && previouos_B == 0){
      return -current_A+current_B;
    }else if(previous_A == 0 && previouos_B == 1){
      return current_A-1+current_B;
    }else if(previous_A == 1 && previouos_B == 0){
      return -current_A-current_B+1;
    }else if(previous_A == 1 && previouos_B == 1){
      return current_A-current_B;
    }else{
      return 0;
    }

}
String getNote(int i){
  if(i==1){
    return "C";
  }else
  if(i==2){
    return "C#";
  }else
  if(i==3){
    return "D";
  }
  if(i==4){
    return "D#";
  }
  if(i==5){
    return "E";
  }
  if(i==6){
    return "F";
  }
  if(i==7){
    return "F#";
  }
  if(i==8){
    return "G";
  }
  if(i==9){
    return "G#";
  }
  if(i==10){
    return "A";
  }
  if(i==11){
    return "A#";
  }
  if(i==12){
    return "B";
  }
  else{return "Nan";}
}
void u8to2DKeyArray(int8_t **col){
  for(int i=0;i<7;i++){
    if(keyArray[i] == 0){
      col[i][3] = 1;col[i][2]=1;col[i][1]=1;col[i][0]=1;
    }else
    if(keyArray[i] == 1){
      col[i][3] = 1;col[i][2]=1;col[i][1]=1;col[i][0]=0;
    }else
    if(keyArray[i] == 2){
      col[i][3] = 1;col[i][2]=1;col[i][1]=0;col[i][0]=1;

    }else
    if(keyArray[i] == 3){
      col[i][3] = 1;col[i][2]=1;col[i][1]=0;col[i][0]=0;
    }else
    if(keyArray[i] == 4){
      col[i][3] = 1;col[i][2]=0;col[i][1]=1;col[i][0]=1;
    }else
    if(keyArray[i] == 5){
      col[i][3] = 1;col[i][2]=0;col[i][1]=1;col[i][0]=0;
    }else
    if(keyArray[i] == 6){
      col[i][3] = 1;col[i][2]=0;col[i][1]=0;col[i][0]=1;
    }else
    if(keyArray[i] == 7){
      col[i][3] = 1;col[i][2]=0;col[i][1]=0;col[i][0]=0;
    }else
    if(keyArray[i] == 8){
      col[i][3] = 0;col[i][2]=1;col[i][1]=1;col[i][0]=1;
    }else
    if(keyArray[i] == 9){
      col[i][3] = 0;col[i][2]=1;col[i][1]=1;col[i][0]=0;
    }else
    if(keyArray[i] == 10){
      col[i][3] = 0;col[i][2]=1;col[i][1]=0;col[i][0]=1;
    }else
    if(keyArray[i] == 11){
      col[i][3] = 0;col[i][2]=1;col[i][1]=0;col[i][0]=0;
    }else
    if(keyArray[i] == 12){
      col[i][3] = 0;col[i][2]=0;col[i][1]=1;col[i][0]=1;
    }else
    if(keyArray[i] == 13){
      col[i][3] = 0;col[i][2]=0;col[i][1]=1;col[i][0]=0;
    }else
    if(keyArray[i] == 14){
      col[i][3] = 0;col[i][2]=0;col[i][1]=0;col[i][0]=1;
    }else{
      col[i][3] = 0;col[i][2]=0;col[i][1]=0;col[i][0]=0;
      
    }
  }

}
int8_t updateCurrentKey(int8_t **keyarray2D){
  int8_t c =0;
  for(int i = 0;i<12;i++){
    currentKey[i]=0;
  }
  for(int i= 0;i<3;i++){
    for(int j = 0;j<4;j++){
    if(keyarray2D[i][j]!=0){
      currentKey[c] = i*4+j+1;
      c++;
      }
    }
  }
  return c;
}
void updateStepList(){
      for(int i=0;i<12;i++){
        currentStepSizeList[i] = stepSizes[currentKey[i]];
      }
}

void decodeTask(void * pvParameters) {
  uint8_t octave;
  uint8_t key;
  uint8_t press;
  while(1){
    //Serial.println("Decode"); 
    xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
    press = RX_Message[0];
    octave = RX_Message[1];
    key = RX_Message[2];
    if(press==80){
  
    //Serial.println(octave); 
    //Serial.println(key); 
    }

    
    sampleISR();
  }
}

void CAN_TX_Task (void * pvParameters) {
      uint8_t msgOut[16];
      
        while (1) {
       
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
       
        xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
  
        
        CAN_TX(0x123, msgOut);
        }
}

void scanKeysTask(void * pvParameters) {

  int8_t keyarray2D[7][4];
  int8_t *pkeyarray2D[7];
  int8_t currentNob0s = 0; 
  int8_t Nob0sdelta = 0;
  int8_t previousNobs = 0;
  int8_t localKnob0s = 0;
  double doubleShift = 0;

  for (int i = 0; i < 7; i++){
        pkeyarray2D[i] = keyarray2D[i];}

  int currentVolume = 0;
  int previous_A = 0;
  int previous_B = 0;
  const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime= xTaskGetTickCount();
  while (1) {
    //Serial.println("scankey");
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
    for(int i=0;i<=6;i++){
      setRow(i);
      delayMicroseconds(2);//change deley to 2 make sure it have enough time to setRow
      keyArray[i] = readCols();
    }
    joyX = analogRead(JOYX_PIN);
    joyY = analogRead(JOYY_PIN);
    xSemaphoreGive(keyArrayMutex);
    u8to2DKeyArray(pkeyarray2D);

    int8_t keysnumber = updateCurrentKey(pkeyarray2D);

    knob3.updateKnob(keyarray2D[3][0],keyarray2D[3][1]);
    knob2.updateKnob(keyarray2D[3][2],keyarray2D[3][3]);
    knob1.updateKnob(keyarray2D[4][0],keyarray2D[4][1]);
    int32_t LocShifts = (int)(joyX+joyY);
    int scale = knob1.getVolume();
    if(scale ==0){
      LocShifts = LocShifts/8;
    }else if(scale == 1){
      LocShifts = LocShifts/4;
    }else if(scale == 2){
      LocShifts = LocShifts/2;      
    }
    else if(scale == 4){
      LocShifts = LocShifts*2;
    }else if(scale == 5){
      LocShifts = LocShifts*4;
    }
    doubleShift = (double)(LocShifts/(double)joyStickOffset);
    currentNob0s = keyarray2D[6][0];
    if(previousNobs == 1 && currentNob0s == 0){
      Nob0sdelta = 1;
    }else{
      Nob0sdelta = 0;
    }
    if(Nob0sdelta == 1){
        localKnob0s = __atomic_load_n(&knob0s,__ATOMIC_RELAXED); 
        if(localKnob0s == 0){
        __atomic_store_n(&knob0s,1,__ATOMIC_RELAXED); 
        }else{
        __atomic_store_n(&knob0s,0,__ATOMIC_RELAXED); 
        }
    }
    localKnob0s = __atomic_load_n(&knob0s,__ATOMIC_RELAXED); 
    previousNobs = keyarray2D[6][0];
    xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
    shifts = doubleShift;
    shifts2 = doubleShift*2;
     //Serial.print("Mutex");
    for(int i= 0; i<12;i++){
        ShiftedPeriod[i] = (Period[i]/(LocShifts/(double)joyStickOffset));
        
    }
    int c = 0;

    if(localKnob0s == 0 && RX_Message[0]== 80){
      SecondKeysnumber = RX_Message[1];
      if(keysnumber<12){
        int j = 0;
        for(int i =keysnumber;i<12;i++){
          if(RX_Message[2+j]!=0){
              currentKey[keysnumber]=RX_Message[2+j];
              j++;
              keysnumber++;
            }
          }
        }
      }else{
        SecondKeysnumber = 0;
      }

    xSemaphoreGive(keyArrayMutex);

    __atomic_store_n(&currentKeysNumber,keysnumber,__ATOMIC_RELAXED);

    updateStepList();
   

  

     //checking the current key pressing status
    if(localKnob0s==1){
        if(currentKey[0]!=0){
          TX_Message[1]=keysnumber;
          TX_Message[0] = 'P';
         
          //TX_Message[1] = knob1.getVolume();
          for(int i = 0; i<12;i++){
          TX_Message[i+2] = currentKey[i];
          }
        }else{
          TX_Message[1] = 0;
          TX_Message[0] = 'R';
          
          
        }
        
       
    
    }
    //int tmp = 0;
   //determine if it is in send/receive mode
  if(localKnob0s==1&&TX_Message[0]=='P'){    
    tmp = 1;
    xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);
    Serial.println("hi");
    
    }
  if(localKnob0s==1&&tmp==1&&TX_Message[0]=='R'){
    xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);
    //Serial.println("endddddddddddddddd");
    tmp=0;
    }
    Serial.print(tmp);
    sampleISR();
    
     
  }
  
    

}
void displayUpdateTask(void * pvParameters){

  while(1) {
    //Update display
    //Serial.println("Display"); 
    u8g2.clearBuffer();         // clear the internal memory
    if(knob0s==0){
    u8g2.setFont(u8g2_font_squeezed_r6_tr); // choose a suitable font
    u8g2.setCursor(2,9); // set up the text display
    u8g2.print("Music Synthesizer");
    u8g2.setFont(u8g2_font_open_iconic_play_1x_t); // set up the key icon
    u8g2.drawGlyph(2,21,77);
    u8g2.setFont(u8g2_font_squeezed_r6_tr); // display the keypressed
    u8g2.setCursor(13,20);
    int keynumber = 0;
    for(int i=0;i<12;i++){
      if(currentKey[i]!=0){
        u8g2.print(getNote(currentKey[i]));
        keynumber++;
      }else{
        break;
      }

    }
    u8g2.drawFrame(11,12,50,10);
     //u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
   
    //u8g2.setCursor(2,20);
    //u8g2.print("volume: ");
    u8g2.setFont(u8g2_font_open_iconic_play_1x_t);// set up the volume icon
    u8g2.drawGlyph(94,21,79);
    u8g2.setFont(u8g2_font_squeezed_r6_tr); // display the volume
    u8g2.setCursor(103,20);
    u8g2.print(knob3.getVolume(),DEC);
    u8g2.setFont(u8g2_font_open_iconic_play_1x_t); // set up the Timbre icon
    u8g2.drawGlyph(2,33,64);
    //u8g2.setCursor(2,30);
    u8g2.setFont(u8g2_font_squeezed_r6_tr); // display the timbre
    u8g2.setCursor(11,32);
    u8g2.print("Timbre: ");
    if(knob2.getVolume()==1){
      u8g2.print("Sine");
    }else if(knob2.getVolume()==2){
      u8g2.print("Square");
    }else if(knob2.getVolume()==0){
      u8g2.print("Saw");
    }

    u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
    u8g2.drawGlyph(70,33,87);
    u8g2.setFont(u8g2_font_squeezed_r6_tr);
    u8g2.setCursor(80,32);
    u8g2.print("Octave: ");
    u8g2.print(knob1.getVolume(),DEC);
    u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
    u8g2.drawGlyph(70,10,81);
    u8g2.setFont(u8g2_font_squeezed_r6_tr);
    u8g2.setCursor(80,9);
    u8g2.print("Mode: ");
    u8g2.print("RX");
  
    u8g2.setFont(u8g2_font_open_iconic_thing_1x_t); // joystick icon
    u8g2.drawGlyph(70,21,81);
    int8_t x = analogRead(JOYX_PIN);
    int8_t y = analogRead(JOYY_PIN);
    int32_t tmp = analogRead(JOYX_PIN)+analogRead(JOYY_PIN);

    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    
    if(tmp < 400){
      u8g2.drawGlyph(80,21,259);

    }else if(tmp > 1300){
      u8g2.drawGlyph(80,21,218);

    }else{
      u8g2.drawGlyph(80,21,212);
    }

    }else if(knob0s == 1){
         u8g2.setFont(u8g2_font_squeezed_r6_tr); // choose a suitable font
         u8g2.setCursor(2,9); // set up the text display
         u8g2.print("Music Synthesizer");
         u8g2.setFont(u8g2_font_open_iconic_play_1x_t); // set up the key icon
         u8g2.drawGlyph(2,21,77);
         u8g2.setFont(u8g2_font_squeezed_r6_tr); // display the keypressed
         u8g2.setCursor(13,20);
         int keynumber = 0;
          for(int i=0;i<12;i++){
            if(currentKey[i]!=0){
              u8g2.print(getNote(currentKey[i]));
              keynumber++;
            }else{
              break;
            }

          }
          u8g2.drawFrame(11,12,50,10);
          //u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
            u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
            u8g2.drawGlyph(70,10,81);
            u8g2.setFont(u8g2_font_squeezed_r6_tr);
            u8g2.setCursor(80,9);
            u8g2.print("Mode: ");
            u8g2.print("TX");
            u8g2.setFont(u8g2_font_open_iconic_thing_1x_t); // joystick icon
            u8g2.drawGlyph(70,21,81);
            u8g2.setFont(u8g2_font_open_iconic_check_1x_t);
            u8g2.drawGlyph(80,21,68);
            u8g2.drawGlyph(104,21,68);
            u8g2.setFont(u8g2_font_open_iconic_play_1x_t);// set up the volume icon
            u8g2.drawGlyph(94,21,79);
            u8g2.setFont(u8g2_font_squeezed_r6_tr); // choose a suitable font
            u8g2.setCursor(2,31); // set up the text display
            u8g2.print("Controlled by keyboard on the left!!");




    }
    u8g2.sendBuffer(); 
    //Toggle LED
    digitalToggle(LED_BUILTIN);
  }
}

void loop() {

  // put your main code here, to run repeatedly:

}
