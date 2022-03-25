# **Embedded Systems CW2 Music Synthesizer**
## Go Inbed Group


---   

### Identification of All our Tasks   

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

### Flowchart   
   
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
<br/>

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
