

/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                              uC/OS-II
*                                            EXAMPLE CODE
*
* Filename : main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/
#define _CRT_SECURE_NO_WARNINGS
#include  <cpu.h>
#include  <lib_mem.h>
#include  <os.h>

#include  "app_cfg.h"
#define TASK_STACKSIZE 2048
#define MAX_PRINT_BUFFER 16

//#define R1_Priority 1
//#define R2_Priority 2




/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK  StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];
INT16U stopAlltask =0;
INT16U deadTask;

BUFFER Buffer;

OS_EVENT* R1;
OS_EVENT* R2;
INT8U R1_Ceiling;
INT8U R2_Ceiling;





/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  StartupTask(void* p_arg);


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/

/* taskset define
    TaskID / Arrivetime / Executetime / periodic */
static void task(void* p_arg);


void task(void* p_arg) {
    task_para_set* task_data = (task_para_set*)p_arg;
    INT32U current_time;
    INT32U executed_time;
    INT8U err; 
    INT8U base_prio = task_data->TaskPriority;
    INT8U current_prio_log;
    INT8U target_prio_log;
    current_time = OSTimeGet();
    while (stopAlltask == 0) {
        task_data->Ready = 1;
        while (task_data->RemainTime > 0 && stopAlltask == 0) {
            if (stopAlltask == 1) {
                break;
            }

            executed_time = task_data->TaskExecutionTime - task_data->RemainTime;
            // 檢查 R1 Lock
            if ((executed_time == task_data->R1Lock) && (task_data->R1UnLock != 0)) {
#if PROTOCOL == NPCS
                LOG_print(3, "./Output.txt", "%d\tLockResource\ttask(%2d)(%2d)\t%s\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R1");
                OSSchedLock(); // 禁止搶佔
#elif PROTOCOL == CPP
                current_prio_log = base_prio;
                if (executed_time > task_data->R2Lock && executed_time < task_data->R2UnLock) {
                    if (R2_Ceiling < current_prio_log) current_prio_log = R2_Ceiling;
                }
                target_prio_log = (R1_Ceiling < current_prio_log) ? R1_Ceiling : current_prio_log;
                LOG_print(3, "./Output.txt", "%d\tLockResource\ttask(%2d)(%2d)\t%s\t%d to %d\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R1", current_prio_log, target_prio_log);

                OSMutexPend(R1, 0, &err);
#endif
            }

            // 檢查 R2 Lock
            if ((executed_time == task_data->R2Lock) && (task_data->R2UnLock != 0)) {

#if PROTOCOL == NPCS
                LOG_print(3, "./Output.txt", "%d\tLockResource\ttask(%2d)(%2d)\t%s\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R2");
                OSSchedLock(); // 禁止搶佔 
#elif PROTOCOL == CPP
                // Lock R2 前，檢查是否持有 R1
                current_prio_log = base_prio;
                if (executed_time > task_data->R1Lock && executed_time < task_data->R1UnLock) {
                    if (R1_Ceiling < current_prio_log) current_prio_log = R1_Ceiling;
                }
                // Lock R2 後，再跟 R2 Ceiling 比
                target_prio_log = (R2_Ceiling < current_prio_log) ? R2_Ceiling : current_prio_log;
                LOG_print(3, "./Output.txt", "%d\tLockResource\ttask(%2d)(%2d)\t%s\t%d to %d\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R2", current_prio_log, target_prio_log);

                OSMutexPend(R2, 0, &err);
#endif
            }

            // 檢查 R1 Unlock
            if ((executed_time == task_data->R1UnLock) && (task_data->R1UnLock != 0)) {

#if PROTOCOL == NPCS
                LOG_print(3, "./Output.txt", "%d\tUnlockResource\ttask(%2d)(%2d)\t%s\t\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R1");
                OSSchedUnlock(); // 恢復排程 
#elif PROTOCOL == CPP
                // 檢查是否持有 R2
                current_prio_log = base_prio;
                if (R1_Ceiling < current_prio_log) current_prio_log = R1_Ceiling; // 因為正在解鎖 R1，肯定持有它
                if (executed_time > task_data->R2Lock && executed_time < task_data->R2UnLock) {
                    if (R2_Ceiling < current_prio_log) current_prio_log = R2_Ceiling;
                }
                // Unlock 後，R1 移除。檢查 R2
                target_prio_log = base_prio;
                if (executed_time > task_data->R2Lock && executed_time < task_data->R2UnLock) {
                    if (R2_Ceiling < target_prio_log) target_prio_log = R2_Ceiling;
                }
                LOG_print(3, "./Output.txt", "%d\tUnlockResource\ttask(%2d)(%2d)\t%s\t%d to %d\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R1", current_prio_log, target_prio_log);

                OSMutexPost(R1);
#endif
            }

            // 檢查 R2 Unlock
            if ((executed_time == task_data->R2UnLock) && (task_data->R2UnLock != 0)) {

#if PROTOCOL == NPCS
                LOG_print(3, "./Output.txt", "%d\tUnlockResource\ttask(%2d)(%2d)\t%s\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R2");
                OSSchedUnlock(); // 恢復排程
#elif PROTOCOL == CPP
                // 檢查 R1
                current_prio_log = base_prio;
                if (R2_Ceiling < current_prio_log) current_prio_log = R2_Ceiling; // 肯定持有 R2
                if (executed_time > task_data->R1Lock && executed_time < task_data->R1UnLock) {
                    if (R1_Ceiling < current_prio_log) current_prio_log = R1_Ceiling;
                }
                // Unlock 後，R2 移除 檢查有沒有 R1
                target_prio_log = base_prio;
                if (executed_time > task_data->R1Lock && executed_time < task_data->R1UnLock) {
                    if (R1_Ceiling < target_prio_log) target_prio_log = R1_Ceiling;
                }
                LOG_print(3, "./Output.txt", "%d\tUnlockResource\ttask(%2d)(%2d)\t%s\t%d to %d\n",
                    OSTime, task_data->TaskID, task_data->job_no, "R2", current_prio_log, target_prio_log);

                OSMutexPost(R2);
#endif
            }
            LOG_print(3, "./Output.txt", "%d\ttask(% 2d) is running\t\n", OSTime, task_data->TaskID);
            current_time = OSTimeGet();
            while (OSTimeGet() == current_time) {
                if (deadTask != 0 && stopAlltask == 0)
                {
                    stopAlltask = 1;
                    LOG_print(3, "./Output.txt", "%d\tMissdeadline\ttask(%2d)(%2d)\t-------------------\n", OSTime, TaskParameter[deadTask].TaskID, TaskParameter[deadTask].job_no);
                    break;
                }
            }
        }
        if (stopAlltask == 1) {
            break;
        }
        OS_ENTER_CRITICAL();
        current_time = OSTimeGet();
        task_data->deadline += task_data->TaskPeriodic; 
        task_data->ResponseTime = current_time - task_data->TaskArriveTime;
        task_data->TaskArriveTime += task_data->TaskPeriodic;
        if (current_time >= task_data->TaskArriveTime) {
            task_data->Delay = 0;
        }
        else {
            task_data->Delay = task_data->TaskArriveTime - current_time;
        }
        OS_EXIT_CRITICAL();
        OSTimeDly(task_data->Delay);
    }
    while (1) {}
}

int  main(void)
{
#if OS_TASK_NAME_EN > 0u
    CPU_INT08U  os_err;
#endif
    INT8U err;
    OS_TCB* ptcb;
    CPU_IntInit();

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    CPU_IntDis();                                               /* Disable all Interrupts                               */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

    OSInit();                                                   /* Initialize uC/OS-II                                  */


    OutFileInit();
    InputFile();


    Buffer.rear = 0;
    Buffer.front = 0;

    Task_STK = malloc(TASK_NUMBER * sizeof(int*));


    // 假設所有任務資料都已經讀入完畢
    // 先依據 Period 將任務排序（Period 越小越高優先權）
    for (int a = 0; a < TASK_NUMBER - 1; a++) {
        for (int b = a + 1; b < TASK_NUMBER; b++) {
            if (TaskParameter[a].TaskPeriodic > TaskParameter[b].TaskPeriodic) {
                // 交換 TaskParameter[a] 和 TaskParameter[b]
                task_para_set temp = TaskParameter[a];
                TaskParameter[a] = TaskParameter[b];
                TaskParameter[b] = temp;
            }
        }
    }
#if PROTOCOL == NPCS
    // 排序完之後重新給 priority
    for (int i = 0; i < TASK_NUMBER; i++) {
        TaskParameter[i].TaskPriority = i + 1;
    }


    for (int n = 0; n < TASK_NUMBER; n++)
    {
        Task_STK[n] = malloc(TASK_STACKSIZE * sizeof(int));
        OSTaskCreateExt(task,
            &TaskParameter[n],
            &Task_STK[n][TASK_STACKSIZE - 1],
            TaskParameter[n].TaskPriority ,
            TaskParameter[n].TaskID,
            &Task_STK[n][0],
            TASK_STACKSIZE,
            &TaskParameter[n],
            (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
    }

#elif PROTOCOL == CPP
    INT8U R1_Prio = 255; 
    INT8U R2_Prio = 255;
    for (int i = 0; i < TASK_NUMBER; i++) {
        TaskParameter[i].TaskPriority = 3*(i+1);
    }

    for (int i = 0; i < TASK_NUMBER; i++) {
        // 檢查該 Task 是否使用 R1
        if (TaskParameter[i].R1Lock < TaskParameter[i].R1UnLock) {
            if (TaskParameter[i].TaskPriority < R1_Prio) {
                R1_Prio = TaskParameter[i].TaskPriority;
            }
        }
        // 檢查該 Task 是否使用 R2
        if (TaskParameter[i].R2Lock < TaskParameter[i].R2UnLock) {
            if (TaskParameter[i].TaskPriority < R2_Prio) {
                R2_Prio = TaskParameter[i].TaskPriority;
            }
        }
    }

    R1_Ceiling = R1_Prio - 1;
    R2_Ceiling = R2_Prio - 2;

    R1 = OSMutexCreate(R1_Ceiling, &err); 
    R2 = OSMutexCreate(R2_Ceiling, &err); 

    for (int n = 0; n < TASK_NUMBER; n++) {
        Task_STK[n] = malloc(TASK_STACKSIZE * sizeof(int));
        OSTaskCreateExt(task,
            &TaskParameter[n],
            &Task_STK[n][TASK_STACKSIZE - 1],
            TaskParameter[n].TaskPriority, 
            TaskParameter[n].TaskID,
            &Task_STK[n][0],
            TASK_STACKSIZE,
            &TaskParameter[n],
            (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
    }
#endif

    stopAlltask = 0;

#if OS_TASK_NAME_EN > 0u
    OSTaskNameSet(APP_CFG_STARTUP_TASK_PRIO,
        (INT8U*)"Startup Task",
        &os_err);
#endif
    ptcb = OSTCBList;
    OSTimeSet(0);
    printf("==================TCB Linked List=================\n ");
    printf("Task\tPrev_tcb_addr\tTCB_address\tNext_tcb_addr\t\n");
    while (ptcb != 0) {
        printf("%d\t%10x\t%10x\t%10x\t\n", ptcb->OSTCBPrio, ptcb->OSTCBPrev, ptcb, ptcb->OSTCBNext);
        ptcb = ptcb->OSTCBNext;
    }
    printf("\n\n");
    OSStart();                                                  /* Start multitasking (i.e. give control to uC/OS-II)   */


    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }
}


/*
*********************************************************************************************************
*                                            STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'StartupTask()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  StartupTask(void* p_arg)
{
    (void)p_arg;

    OS_TRACE_INIT();                                            /* Initialize the uC/OS-II Trace recorder               */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    APP_TRACE_DBG(("uCOS-III is Running...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        OSTimeDlyHMSM(0u, 0u, 1u, 0u);
        APP_TRACE_DBG(("Time: %d\n\r", OSTimeGet()));
    }
}


