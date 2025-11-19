# uC-OS-II
HW1 觀察任務context switch狀況
  1.app_hooks.c -> App_TaskSwHook 觀察發生context switch時當前任務以及下個任務

PA1 排程 RM 、 FIFO(not finish)
  1.os_cores.c -> OS_IntExit()發生context switch時代表preempt發生
  2.os_cores.c -> OS_Sched()發生context switch時代表任務完成 主動釋放CPU使用權
  3.os_time.c -> OSTimeDly() 更改為tick>=0u 避免tick=0u時不進入os_sched() 
  4.os_core.c -> OSTimeTick() 計數task remainTime 以及 搜尋是否有任務missdeadline
  5.main.c - RM根據periodic當作priority，在TaskCreateExt時做sort，根據periodic從小排到大，並賦予priority
  6.main.c -> task() 模擬任務占用CPU，並計算task ArriveTime、Deadline等等

PA2 排程 EDF
  1.os_core.c -> os_schedNew() 使用OS_TCB* ptcb搜尋OSTCBList，找出earliestDeadline = choosePrio並設定OSPrioHighRdy = choosePrio
  2.os_core.c -> os_schedNew() 相同deadline時根據taskID做排程

  CUS scheduler implement
    1.
              
  
