# uC-OS-II
## HW1 觀察任務context switch狀況  
  - app_hooks.c -> App_TaskSwHook 觀察發生context switch時當前任務以及下個任務  
  
## PA1 排程 RM 、 FIFO(not finish)  
  - os_cores.c -> OS_IntExit()發生context switch時代表preempt發生  
  - os_cores.c -> OS_Sched()發生context switch時代表任務完成 主動釋放CPU使用權  
  - os_time.c -> OSTimeDly() 更改為tick>=0u 避免tick=0u時不進入os_sched()   
  - os_core.c -> OSTimeTick() 計數task remainTime 以及 搜尋是否有任務missdeadline   
  - main.c - RM根據periodic當作priority，在TaskCreateExt時做sort，根據periodic從小排到大，並賦予priority   
  - main.c -> task() 模擬任務占用CPU，並計算task ArriveTime、Deadline等等  
  

## PA2 排程 EDF  
  - os_core.c -> os_schedNew() 使用OS_TCB* ptcb搜尋OSTCBList，找出earliestDeadline = choosePrio並設定OSPrioHighRdy = choosePrio  
  - os_core.c -> os_schedNew() 相同deadline時根據taskID做排程  
  ### CUS scheduler implement  
  - ucos_ii.h -> 建立新structure存放aperiodic job資料  
  - app_hooks.c -> 讀取CUS server size、匯入apperiodicjob.tex內容資料  
  - App_TimeTickHook -> 處理apperiodic job到達與控制並計算新的CUS server deadline
  - App_TimeTickHook -> 時間到達resume CUS task讓CUS server執行aperiodic job工作
  - main.c -> 判斷CUS server size是否大於0
  - main.c -> cus_task function 負責處理分配到的aperiodic job工作
    
              
  
