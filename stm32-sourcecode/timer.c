/*
 * timer.c
 *
 *  Created on: Oct 16, 2025
 *      Author: nguye
 */
#include "timer.h"


void setTimer(int task_id, int duration)
{
	if (task_id >= 0 && task_id < NUM_TASKS)
	{
		timer_counter[task_id] = duration/TIMER_CYCLE;
		timer_flag[task_id] = 0;
	}
}


void timerRun()
{
	for (int i = 0; i < NUM_TASKS; i++)
	{
		if (timer_counter[i] > 0)
		{
			timer_counter[i]--;
			if (timer_counter[i] <= 0)
			{
				timer_flag[i] = 1;
			}
		}
	}
}



