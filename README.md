# **Embedded Systems CW2 Music Synthesizer**
## Go Inbed Group

---
### Characterisation of Tasks and CPU Time Utilisation

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
### Critical Instant Analysis
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

---
### Flowchart
![image]()

