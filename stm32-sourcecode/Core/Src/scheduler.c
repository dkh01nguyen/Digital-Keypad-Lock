/*
 * scheduler.c
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */
#include "main.h"
#include "scheduler.h"

sTask SCH_tasks_G[SCH_MAX_TASKS];
uint8_t SCH_task_count = 0;
uint8_t Error_code_G = 0;

void SCH_Init(void)
{
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++)
        SCH_Delete_Task(i);

    SCH_task_count = 0;
    Error_code_G = 0;
}

uint8_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD)
{
    if (SCH_task_count >= SCH_MAX_TASKS)
    {
        Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
        return SCH_MAX_TASKS;
    }

    uint8_t index = 0;
    while (index < SCH_task_count && DELAY >= SCH_tasks_G[index].Delay)
    {
        DELAY -= SCH_tasks_G[index].Delay;
        index++;
    }

    for (uint8_t i = SCH_task_count; i > index; i--)
        SCH_tasks_G[i] = SCH_tasks_G[i - 1];

    SCH_tasks_G[index].pTask = pFunction;
    SCH_tasks_G[index].Delay = DELAY;
    SCH_tasks_G[index].Period = PERIOD;
    SCH_tasks_G[index].RunMe = 0;

    if (index < SCH_task_count)
        SCH_tasks_G[index + 1].Delay -= DELAY;

    SCH_task_count++;
    return index;
}

void SCH_Update(void)
{
    if (SCH_task_count == 0) return;

    if (SCH_tasks_G[0].Delay > 0)
        SCH_tasks_G[0].Delay--;

    for (uint8_t i = 0; i < SCH_task_count && SCH_tasks_G[i].Delay == 0; i++)
        SCH_tasks_G[i].RunMe++;
}

void SCH_Dispatch_Tasks(void)
{
    if (SCH_task_count == 0) return;

    if (SCH_tasks_G[0].RunMe > 0)
    {
        void (*pTask)() = SCH_tasks_G[0].pTask;

        SCH_tasks_G[0].RunMe--;
        (*pTask)();

        sTask tmp = SCH_tasks_G[0];
        SCH_Delete_Task(0);

        if (tmp.Period > 0)
            SCH_Add_Task(tmp.pTask, tmp.Period, tmp.Period);
    }
}

uint8_t SCH_Delete_Task(const uint8_t index)
{
    if (index >= SCH_task_count) return RETURN_ERROR;

    for (uint8_t i = index; i < SCH_task_count - 1; i++)
        SCH_tasks_G[i] = SCH_tasks_G[i + 1];

    SCH_tasks_G[SCH_task_count - 1].pTask = 0;
    SCH_tasks_G[SCH_task_count - 1].Delay = 0;
    SCH_tasks_G[SCH_task_count - 1].Period = 0;
    SCH_tasks_G[SCH_task_count - 1].RunMe = 0;

    SCH_task_count--;
    return RETURN_NORMAL;
}


