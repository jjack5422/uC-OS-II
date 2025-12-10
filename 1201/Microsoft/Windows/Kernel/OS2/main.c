

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
    current_time = OSTimeGet();
    if (task_data->TaskArriveTime > current_time) {
        OSTimeDly(task_data->TaskArriveTime - current_time);
    }
    while (stopAlltask==0) {
        while(task_data->RemainTime>0 && stopAlltask==0) {
            if (stopAlltask == 1) {
                break;
            }
            LOG_print(3,"./Output.txt","%d\ttask(% 2d) is running\t\n", OSTime, task_data->TaskID);
            current_time = OSTimeGet();
            while (OSTimeGet() == current_time) {
                if (deadTask != 0 && stopAlltask == 0)
                {
                    stopAlltask = 1;
                    LOG_print(3, "./Output.txt","%d\tMissdeadline\ttask(%2d)(%2d)\t-------------------\n", OSTime, deadTask,TaskParameter[deadTask].job_no);
                    break;
                }
            }
        }
        if (stopAlltask == 1) {
            break;
        }
        OS_ENTER_CRITICAL();
        current_time = OSTimeGet();
        task_data->deadline += task_data->TaskPeriodic; // T2: next_period = 10
        task_data->ResponseTime = current_time - task_data->TaskArriveTime;
        task_data->TaskArriveTime += task_data->TaskPeriodic;
        if (current_time>= task_data->TaskArriveTime) {
            task_data->Delay = 0;
        }
        else {
            task_data->Delay = task_data->TaskArriveTime - current_time;
        }
        task_data->RemainTime = task_data->TaskExecutionTime;
        OS_EXIT_CRITICAL();
        // (這會觸發 OS_Sched() 和 OSCtxSw())
        OSTimeDly(task_data->Delay);
        task_para_set* cur = OSTCBCur->OSTCBExtPtr;
        task_para_set* next = OSTCBHighRdy->OSTCBExtPtr;
        if (OSTCBCur->CompletedFlag == 1 && cur->Delay==0) {
            if (OSTCBHighRdy == OSTCBCur) {
                LOG_print(3, "./Output.txt","%d\tCompletion\ttask(%2d)(%2d)\ttask(%2d)(%2d)\t%d\t%d\t%d\t\n", OSTimeGet(),
                    cur->TaskID, cur->job_no++, next->TaskID, next->job_no,cur->ResponseTime, 
                    cur->ResponseTime-cur->TaskExecutionTime,cur->Delay);
                OSTCBCur->CompletedFlag = 0;
            }
        }

    }
    while (1) {}
}


int  main(void)
{
#if OS_TASK_NAME_EN > 0u
    CPU_INT08U  os_err;
#endif

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


