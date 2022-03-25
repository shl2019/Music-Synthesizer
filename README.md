# **Embedded Systems CW2 Music Synthesizer**
## Go Inbed Group



---   

### Music Synthesizer

<img src="https://github.com/shl2019/EmbeddedCW2/blob/main/Music%20Synthesizer.png" width="550" height="500">
<br/>

---   

### User Interface   

##### Joy stick
The joy stick is configured to changing the note frequency lightly.   
##### First Knob
The first knob on the left is a press knob that will change the keyboard mode between transmit/receive.   
##### Second Knob
The second knob is for octave selection.   
##### Third Knob
The third knob is for timbre selection.   
##### Fourth Knob
The fourth knob is for volume selection.   
##### Keyboard Mode
When the keyboard is set to TX(transmitting) mode, it will not generated any sound, but auto-configured as an extension of RX keyboard and play corresponding octave.   
<br/>

---   

### Video Demo   

Please find the Demo via this link: https://web.microsoftstream.com/video/0a6d23d4-fab1-4962-8104-bb528b7e5a7b
<br/>

---   

### Identification of All Tasks   

There are four main tasks implemented in our system, ScanKeysTask, CAN_TX_Task, decodeTask, and displayUpdateTask.   

#### scanKeyTask   
The ScanKeyTask is the thread that initialled with delay, it scans the key array to detect if keys or knobs have been pressed, also the joystick. It then performs the associated configuration, for example, the octave, timbre, mode and timbre selection. The information will also be sent to another keyboard by the thread-safe queue. Once everything is configured, the Sample_ISR which is interrupt triggered will write the output voltage to the speaker.

#### displayUpdateTask   
The displayUpdateTask is a thread that has no dependency with the other task, it is set with an initiation interval of 100ms, it will update the key pressed, volume, joystick, etc on the display. This task is the only task of the four total tasks that don’t have an interrupt.   

#### decodeTask   
The decode_Task is a thread also triggered by the message queue, to minimise the execution time for this task, we didn’t do anything else but receive the Message_RX. The CAN_RX function is in an ISR called CAN_RX_ISR, so the interrupt is called whenever received a CAN message.   

#### CAN_TX_Task   
The CAN_TX_Task is the thread that is triggered by the message queue and semaphore, it will only execute when there are messages in the In Mailbox and take the semaphore, the semaphore will be given in CAN_TX_ISR so every time the mailbox becomes available the semaphore will be given.   
<br/>

---   

### Characterisation of Tasks with Initiation Interval and Execution Time and CPU Time Utilisation   

The table below demonstrates the minimum initiation interval τi set and maximum execution time Ti measured by us. The Ti is obtained from averaging total execution time with 32 iterations. The CPU time utilisation is calculated by the formula Maximum Execution Time/ Minimum   
<br/>
Initiation Interval * 100, i.e., &emsp; ![1](http://latex.codecogs.com/svg.latex?Utilisation=\frac{T_i}{\tau_i}*100)   
<br/>
|        Task       | Total Execution Time /μs | Number of Iterations | Maximum Execution Time Ti  /μs | Minimum Initiation Interval τi /μs | CPU Time Utilisation % |
|:-----------------:|:------------------------:|:--------------------:|:------------------------------:|:----------------------------------:|:----------------------:|
|    scanKeyTask    |           17087          |          32          |            533.96875           |                20000               |       2.66984375       |
| displayUpdateTask |          544924          |          32          |            17028.875           |               100000               |        17.028875       |
|     decodeTask    |            696           |          32          |              21.75             |                25200               |       0.086309524      |
|    CAN_TX_Task    |           27913          |          32          |            872.28125           |                1670                |       52.23241018      |
|     sampleISR     |            534           |          32          |             16.6875            |              45.45454              |       36.71250441      |
<br/>

---   

### Critical Instant Analysis of Rate Monotonic Scheduler   

By conducting a critical instant analysis of the rate monotonic scheduler, we are checking whether all deadlines are met under worst-case conditions. As concluded from the table above, the deadline of the lowest-priority task is τn = 100000 μs. By referring to the formula of latency, the task execution counts and per-task execution times are calculated as follows. Those times are summed together and compared with the initiation interval of the longest task, which is displayUpdateTask in this case.<br/>  
Latency is calculated by &emsp; ![2](http://latex.codecogs.com/svg.latex?Ln=\sum_{i}\frac{\tau_n}{\tau_i}T_i)  
<br/>
The latency result shows that all tasks are executed within the initiation interval of the longest task, which means all deadlines are met under worst-case conditions.   
<br/>
|        Task       | Minimum Initiation Interval τi   /μs | Maximum Execution Time Ti  /μs |   τn/τi  | Latency Ln /μs | Minimum Initiation Interval of   Longest Task τn /μs |
|:-----------------:|:------------------------------------:|:------------------------------:|:--------:|:--------------:|:----------------------------------------------------:|
|    scanKeyTask    |                 20000                |            533.9688            |     5    |    2669.844    |                                                      |
| displayUpdateTask |                100000                |            17028.88            |     1    |    17028.88    |                                                      |
|     decodeTask    |                 25200                |              21.75             | 3.968254 |    86.30952    |                                                      |
|    CAN_TX_Task    |                 1670                 |            872.2813            | 59.88024 |    52232.41    |                                                      |
|   Total Latency   |                                      |                                |          |    72017.44    |                        <100000                       |
<br/>

---   

### Dependency Flowchart   
   
![image](https://github.com/shl2019/EmbeddedCW2/blob/main/Flow%20Chart.png)   
<br/>

---   

### Identification of Shared Data Structure and Methods for Safe Access and Synchronisation   

The first data struct we have used is a mutex, A Mutex is a Mutually exclusive flag. It acts as a gatekeeper to a section of code allowing one thread in and blocking access to all others. This ensures that the code being controlled will only be hit by a single thread at a time.   

```
SemaphoreHandle_t keyArrayMutex;   
```

It is defined to keyarray to avoid multi-accessing, it is lockedby   

```
xSemaphoreTake(keyArrayMutex, portMAX_DELAY);   
```

in scanKey tread when read the keyboard input and unlocked by   

```
xSemaphoreGive(keyArrayMutex);   
```

when reading ends.   

The second data struct we used is semaphore, when sharing/sending the data across the thread, we don’t want the CPU cycle to be used by polling that doesn’t do anything. For example, we define a semaphore to our CAN_TX function,   

```
SemaphoreHandle_t CAN_TX_Semaphore;
```

We use Semaphore to indicate if the message can be accepted, the Semaphore will be given by CAN_TX_ISR when the is space in msgOutQ mailbox,   

```
void CAN_TX_ISR (void) {
    xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}
```

and taken by  CAN_TX_Task before CAN_TX.   

```
xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
```
<br/>

---   

### Inter-task Blocking Dependencies Analysis and Deadlock Possibility   

Blocking dependencies are generally used at the start of the thread to avoid the tread stack in the infinite loop, for example, in the scanKey task,   
```
vTaskDelayUntil( &xLastWakeTime, xFrequency );
```
The two input arguments are the initiation interval of the task and the current time, it will place the thread in the block until the next xFrequency, when it is in the block, the CPU will run the other thread until it is unblocked.   

The other dependency is the thread-safe queues,we have defined those for both CAN_TX_ISR and CAN_RX_ISR.    
```
QueueHandle_t msgInQ;
QueueHandle_t msgOutQ;
```
It eliminated the deadlock by CAN_TX function because when the message sent by CAN_TX is not received CAN_RX, it will block and the RTOS will go into a deadlock, now with the message queue, we are sending Message_TX by queue, and using CAN_TX in the CAN_TX_task that is blocked by,   
```
xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
```
which means the thread will not execute unless receive a message sent by a queue.    

In a similar application with the decode_task, the CAN_RX_ISR is triggered by an interrupt, it then put Message_RX in the queue, the decode_task is blocked until received the message.   
```
xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
```
It then decodes the RX message. To avoid the potential problem and optimise the execution time of every task, we didn’t write the code to play the note here, but we write it with an if loop in scanKey_task. The decode task could potentially be removed, but we keep it here for completion of the task.   
<br/>

---
### Advanced Feature 1: Frequency Changing with Knob and Joystick

In this stage, we implement a frequency shifting method to increase or decrease octaves. We change select 5 different scales, to have a different octave.   

```
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
```

We first read the information from joystick. Add the joyX and joyY together, to have a number between 0 to 2000. If the stick is in default level, then LocShift will be about 1000. If we move the joystick, this number will change. When it becomes larger, the frequency should be higher, when it is lower, the frequency will decrease.
We then change this basic frequency scaling variable by changing the knob1. We set 3 to be our base scale, so A3 = 440Hz. A2 = 220Hz, and so on. Thus, scale 2 will divide the frequency by 2, and scale 4 will multiply the frequency by 2. After selecting the scale, we will normalize this value to be centred around 1.0. So we divide it by joyStickOffset, which is the default value of joyX+joyY read in setup().

```
    xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
    shifts = doubleShift;
```
Now we pass it to shifts, which can be read by sampleISR. Since the double variable cannot be loaded in an atomic way. We will pass it inside a mutex lock, to avoid any asynchronies error.   
<br/>

---   

### Advanced Feature 2: Sine and Square Waves Generation   

We add two more wave types, sine and square respectively.   

```
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
```

Since we want to minimize the calculation inside sampleISR, we need to calculate the stepSize for sine, and each period for each note in the setup. The static array step[] will return the current step for each note. When the step reaches one period, it will stop, then go back to 0 and count again.   

```
for(int j = 0;j<12;j++){
  for(int i =0;i<100;i++){
        sineStep[j][i] = (int)128*sin(2*(NoteFrequencyTable[j]/22000)*PI*i);
  }
  Period[j] = 22000/NoteFrequencyTable[j];
}
```

The sineStep is a 2D array, calculated what the step will be for different 12 notes. And Period[j] stores the number of ticks needed to complete one period. For example, for a 440Hz frequency, the ticks needed will be 22000/440 = 50.   

```
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

```

Now, we have a lookup table here, so we can add the step on Vout by simply checking what the key pressed, and what the step is right now. Note, we average the Vout when there are multiple keys pressed since otherwise, the Vout will go beyond 255. But we also add currentKeyNumber*0.9 here, since we detect that when multiple keys are pressed, the total Vout after average, will be so small. So the more the key is pressed, the smaller the sound is. By multiplying 0.9, we can keep the same volume with multiple keys. The volume can be controlled by Kob3 by shifting the Vout to   

```
(8 - knob3.getVolume()/2);
```

Also, we can change the frequency of sine by the joystick or octave knob.   

```
  for(int i= 0;i<12;i++){
    if(step[i]>=ShiftedPeriod[i]){
      step[i] = 0;
  }
}
```

The for loop above will reset the step by a shifted period, which is defined below.   

```
    for(int i= 0; i<12;i++){
        ShiftedPeriod[i] = (Period[i]/(LocShifts/(double)joyStickOffset));
    }
```

Thus, when the period changes, the frequency will change accordingly. So we can hear a twisted sin wave when rotating the stick offset.   

Square wave:   

```
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
```

The Square has a structure similar to sin, except that it doesn’t have a lookup table. Just simply set the Vout to 128 when the current step is lower than ShiftedPeriod/2, and -128, when higher than ShiftedPeriod/2.   
<br/>

---   

### Advanced Feature 3: Chords with Multiple Keyboards   

We want two keyboards to generate sound with different frequencies at the same time. The first thing we need is a knob to select the current mode. 0 and 1, for receiver and sender.   

```
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
```   
The code above uses nob0s, to control the mode. When pressed and release, the mode will go to the next state. Since nob0s is a square wave function, when we press and release, the output will be like 00001111110000. Firstly, we need to create a Nob0sdelta, which is a delta function of nob0s. We store the previous state of nob0s, and compare it with the current state if it changes from 1 to 0, which means it has been pressed, so Nob0delta will be 1, otherwise 0. Then, we atomic load knob0s to localKnob0s to avoid synchronisation bug, flip the state of knod0s when the Nob0delta is 1. When this is finished, the previousNobs will be updated.   

Now, we focus on mode0, the receiver part. RX_Message[1] stores the number of keys been pressed on the sender keyboard. We pass it to a global variable called SecondKeysnumber (inside the Mutex lock). Then we check how many keys have been pressed in the receiver keyboard. If the number is smaller than 12, we will have space to attach the key pressed from the sender keyboard to the current keyboard. The “currentKey” is simply the key number been pressed. For example, if C and D have been pressed, the currentKey will look like [1,3,0,0,0….]. Thus, after putting all the keys from another keyboard, we can inherit the its information. Then modify or change its frequency later on.

```
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
```
For mode 1, the sender will send key number, “P””R” information, and the currentKey array to the receiver. If we want to connect more than two keyboards, we don’t need to modify this part. Since the currentkey inherits information from the previous keyboard.   

```
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
```
```
    updateStepList();
```

 Then, we will use this modified currentKey array, wo generate a list f steps by using updateStepList().   
 
```
void updateStepList(){
      for(int i=0;i<12;i++){
        currentStepSizeList[i] = stepSizes[currentKey[i]];
      }
}
```

Which put the steps in order according to currentKey[].   

Now, we have the information of how many keys have been pressed from sender keyboards(SecondKeynumber) and the step list (currentStepSizeList). So we could increase an octave. By shifting the keys from the sender keyboard to shifts*2. Thus, a chord can be generated from two keyboards at the same time in one speaker. Since we inherit the second keyboard to the first keyboard, the joystick, octave knob, and sin, square wave all are functional in the receiver keyboard.   

```
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
```
<br/>

---   
## END
