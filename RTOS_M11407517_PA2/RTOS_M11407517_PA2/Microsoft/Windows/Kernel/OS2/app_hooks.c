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
*                                          Application Hooks
*
* Filename : app_hooks.c
* Version  : V2.92.13
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>


/*
*********************************************************************************************************
*                                      EXTERN  GLOBAL VARIABLES
*********************************************************************************************************
*/
BUFFER Buffer;
int count=0;
int next_aper_job_idx = 0;
extern OS_EVENT* AperiodicQ;
extern start;
extern preemptflag;
extern float CUS_SERVER_SIZE;
extern ServerDeadline;
extern CurrentJobIndex;
extern OS_TCB* CusTCB;

#define MAX_PRINT_BUFFER 16

#define LOG_CONSOLE 1
#define LOG_FILE    2
#define LOG_BOTH    3

void LOG_print(INT8U LOG_type, char* outfile,
    _In_z_ _Printf_format_string_ char const* _Format,
    ...) {
    va_list _Arglist;
    va_start(_Arglist, _Format);

    if (LOG_type & LOG_CONSOLE)
        vprintf(_Format, _Arglist);

    if (LOG_type & LOG_FILE) {
        errno_t Output_err;
        va_end(_Arglist);              // 結束第一次
        va_start(_Arglist, _Format);   // 重新開始一次

        if ((Output_err = fopen_s(&Output_fp, OUTPUT_FILE_NAME, "a")) == 0) {
            vfprintf(Output_fp, _Format, _Arglist);
            fclose(Output_fp);
        }
    }

    va_end(_Arglist);
}

void OutFileInit() {
    if ((Output_err = fopen_s(&Output_fp, OUTPUT_FILE_NAME, "w") == 0))
        fclose(Output_fp);
    else
        printf("Error to clear output file");
}

void InputFile() {
    errno_t err;

    if ((err = fopen_s(&fp, INPUT_FILE_NAME, "r")) == 0)
        printf("The file %s was opened\n", INPUT_FILE_NAME);
    else 
        printf("The file %s was not opened\n", INPUT_FILE_NAME);

    char str[MAX];
    char* ptr;
    char* pTmp = NULL;
    int TaskInfo[INFO], i, j = 0;
    TASK_NUMBER = 0;

    while (!feof(fp)) {
        i = 0;
        memset(str, 0, sizeof(str));
        fgets(str, sizeof(str) - 1, fp);
        ptr = strtok_s(str, " ", &pTmp);

        while (ptr != NULL) {
            TaskInfo[i] = atoi(ptr);
            ptr = strtok_s(NULL, " ", &pTmp);
            if (i == 0) {
                TASK_NUMBER++;
                TaskParameter[j].TaskID = TASK_NUMBER;
            }
            else if (i == 1) {
                TaskParameter[j].TaskArriveTime = TaskInfo[i];

                if (ptr == NULL) {
                    CUS_SERVER_SIZE = (float)TaskInfo[i] / 100.0;
                    TaskParameter[j].TaskArriveTime = 0;
                    TaskParameter[j].TaskPriority = 0;
                    TaskParameter[j].deadline = 65535;

                }
            }
            else if (i == 2) {
                TaskParameter[j].TaskExecutionTime = TaskInfo[i];
                TaskParameter[j].RemainTime = TaskInfo[i];
            }
            else if (i == 3) {
                TaskParameter[j].TaskPeriodic = TaskInfo[i]; 
            }
            TaskParameter[j].deadline = TaskParameter[j].TaskArriveTime + TaskParameter[j].TaskPeriodic;
            i++;
        }

        TaskParameter[j].TaskPriority = j;

        j++;
    }
    printf("%f", CUS_SERVER_SIZE);
    fclose(fp);


}

void InputAperiodicjobsFile() {
    errno_t err;

    if ((err = fopen_s(&fp, APERIODIC_FILE_NAME, "r")) == 0)
        printf("The file %s was opened\n", APERIODIC_FILE_NAME);
    else {
        printf("The file %s was not opened\n", APERIODIC_FILE_NAME);
        return;
    }

    char str[MAX];
    char* ptr;
    char* pTmp = NULL;
    int TaskInfo[INFO], i;
    AperTaskNumber = 0;
    int j = 0;
    while (fgets(str, sizeof(str) - 1, fp) != NULL) { // 使用 fgets 當條件比較安全
        i = 0;
        ptr = strtok_s(str, " ", &pTmp);

        while (ptr != NULL) {
            TaskInfo[i] = atoi(ptr);
            ptr = strtok_s(NULL, " ", &pTmp);

            if (i == 0) {
                // Job ID
                AperTaskParameter[j].JobId = TaskInfo[0]; // 修正邏輯
                AperTaskParameter[j].IsArrived = 0;
                AperTaskParameter[j].IsFinished = 0;
            }
            else if (i == 1) {
                AperTaskParameter[j].ArrivalTime = TaskInfo[i];
            }
            else if (i == 2) {
                AperTaskParameter[j].ExecutionTime = TaskInfo[i];
                AperTaskParameter[j].RemainTime = TaskInfo[i];
            }
            else if (i == 3) {
                AperTaskParameter[j].AbsoluteDeadline = TaskInfo[i];
            }
            i++;
        }
        if (i > 0) {
            AperTaskNumber++; // 更新全域變數
            j++;
        }
    }
    fclose(fp);
    printf("Total Aperiodic Jobs Loaded: %d\n", AperTaskNumber);
}



/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*********************************************************************************************************
**                                         GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
**                                        uC/OS-II APP HOOKS
*********************************************************************************************************
*********************************************************************************************************
*/

#if (OS_APP_HOOKS_EN > 0)

/*
*********************************************************************************************************
*                                  TASK CREATION HOOK (APPLICATION)
*
* Description : This function is called when a task is created.
*
* Argument(s) : ptcb   is a pointer to the task control block of the task being created.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  App_TaskCreateHook (OS_TCB *ptcb)
{
#if (APP_CFG_PROBE_OS_PLUGIN_EN == DEF_ENABLED) && (OS_PROBE_HOOKS_EN > 0)
    OSProbe_TaskCreateHook(ptcb);
#endif


}


/*
*********************************************************************************************************
*                                  TASK DELETION HOOK (APPLICATION)
*
* Description : This function is called when a task is deleted.
*
* Argument(s) : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  App_TaskDelHook (OS_TCB *ptcb)
{
    (void)ptcb;
}


/*
*********************************************************************************************************
*                                    IDLE TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
*               has been added to allow you to do such things as STOP the CPU to conserve power.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts are enabled during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 251
void  App_TaskIdleHook (void)
{
}
#endif


/*
*********************************************************************************************************
*                                  STATISTIC TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
*               statistics task.  This allows your application to add functionality to the statistics task.
*
* Argument(s) : none.
*********************************************************************************************************
*/

void  App_TaskStatHook (void)
{
}


/*
*********************************************************************************************************
*                                   TASK RETURN HOOK (APPLICATION)
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : ptcb      is a pointer to the task control block of the task that is returning.
*
* Note(s)    : none
*********************************************************************************************************
*/


#if OS_VERSION >= 289
void  App_TaskReturnHook (OS_TCB  *ptcb)
{
    (void)ptcb;
}
#endif


/*
*********************************************************************************************************
*                                   TASK SWITCH HOOK (APPLICATION)
*
* Description : This function is called when a task switch is performed.  This allows you to perform other
*               operations during a context switch.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*
*               (2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                   will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                  task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

#if OS_TASK_SW_HOOK_EN > 0
void  App_TaskSwHook(void)
{
#if (APP_CFG_PROBE_OS_PLUGIN_EN > 0) && (OS_PROBE_HOOKS_ENs > 0)
    OSProbe_TaskSwHook();
#endif
    task_para_set* cur = OSTCBCur->OSTCBExtPtr;
    task_para_set* next = OSTCBHighRdy->OSTCBExtPtr;    


}





#endif  


/*
*********************************************************************************************************
*                                   OS_TCBInit() HOOK (APPLICATION)
*
* Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
*               up most of the TCB.
*
* Argument(s) : ptcb    is a pointer to the TCB of the task being created.
*
* Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 204
void  App_TCBInitHook (OS_TCB *ptcb)
{
    (void)ptcb;

}
#endif


/*
*********************************************************************************************************
*                                       TICK HOOK (APPLICATION)
*
* Description : This function is called every tick.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_TIME_TICK_HOOK_EN > 0
void  App_TimeTickHook (void)
{
#if (APP_CFG_PROBE_OS_PLUGIN_EN == DEF_ENABLED) && (OS_PROBE_HOOKS_EN > 0)
    OSProbe_TickHook();
#endif
    if (CUS_SERVER_SIZE > 0) {
        INT32U current_time = OSTimeGet() + 1;
        int i;
        for (i = 0; i < AperTaskNumber; i++) {
            // 檢查預判的時間是否等於到達時間
            if (AperTaskParameter[i].ArrivalTime == current_time) {
                AperTaskParameter[i].IsArrived = 1;

                //  Server 忙碌中或 Deadline 未到 -> 不做事
                if (current_time < ServerDeadline && CurrentJobIndex != -1) {
                    LOG_print(3, "./Output.txt","%d\tAperiodic job (%d) arrives. Do nothing.\n",
                        current_time, AperTaskParameter[i].JobId);
                }
                else if (CurrentJobIndex == -1) {
                    INT32U new_deadline = current_time + (INT32U)(AperTaskParameter[i].ExecutionTime / CUS_SERVER_SIZE);

                    if (AperTaskParameter[i].AbsoluteDeadline >= new_deadline) {
                        LOG_print(3, "./Output.txt","%d\tAperiodic job (%d) arrives and sets CUS's deadline as %d.\n",
                            current_time, AperTaskParameter[i].JobId, new_deadline);

                        OS_ENTER_CRITICAL();
                        ServerDeadline = new_deadline;
                        CurrentJobIndex = i;

                        if (CusTCB != NULL) {
                            task_para_set* cus_ext = (task_para_set*)CusTCB->OSTCBExtPtr;
                            if (cus_ext != NULL) {
                                cus_ext->deadline = ServerDeadline;
                            }
                            OSTaskResume(CusTCB->OSTCBPrio);
                        }
                        OS_EXIT_CRITICAL();
                    }
                    else {
                        LOG_print(3, "./Output.txt","%d\tAperiodic job (%d) rejects scheduling.\n",
                            current_time, AperTaskParameter[i].JobId);
                        AperTaskParameter[i].IsFinished = 1;
                    }
                }
            }
        }

        if (current_time == ServerDeadline) {
            int next_job_idx = -1;
            for (i = 0; i < AperTaskNumber; i++) {
                if (AperTaskParameter[i].IsArrived && !AperTaskParameter[i].IsFinished && i != CurrentJobIndex) {
                    next_job_idx = i;
                    break;
                }
            }

            if (next_job_idx != -1) {
                INT32U new_deadline = current_time + (INT32U)(AperTaskParameter[next_job_idx].ExecutionTime / CUS_SERVER_SIZE);

                if (AperTaskParameter[next_job_idx].AbsoluteDeadline >= new_deadline) {
                    LOG_print(3, "./Output.txt","%d\tAperiodic job (%d) sets CUS's deadline as %d.\n",
                        current_time, AperTaskParameter[next_job_idx].JobId, new_deadline);

                    OS_ENTER_CRITICAL();
                    ServerDeadline = new_deadline;
                    CurrentJobIndex = next_job_idx;

                    if (CusTCB != NULL) {
                        task_para_set* cus_ext = (task_para_set*)CusTCB->OSTCBExtPtr;
                        if (cus_ext != NULL) {
                            cus_ext->deadline = ServerDeadline;
                        }
                        OSTaskResume(CusTCB->OSTCBPrio);
                    }
                    OS_EXIT_CRITICAL();
                }
                else {
                    LOG_print(3, "./Output.txt","%d\tAperiodic job (%d) rejects scheduling.\n",
                        current_time, AperTaskParameter[next_job_idx].JobId);
                    AperTaskParameter[next_job_idx].IsFinished = 1;
                }
            }
        }
    }
}
#endif
#endif
