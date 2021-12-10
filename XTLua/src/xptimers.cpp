//
//  xptimers.cpp
//  xlua
//
//  Created by Benjamin Supnik on 4/13/16.
//
//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

// xTLua
// Modified by Mark Parker on 04/19/2020
#include <cstdio>
#include "xptimers.h"
#include <stdlib.h>
#include <stdio.h>
//#include <XPLMProcessing.h>
#include <XPLMDataAccess.h>

struct xlua_timer {
	xlua_timer *		m_next;
	xlua_timer_f 	m_func;
	void *			m_ref;
	
	double			m_next_fire_time;
	double			m_repeat_interval;	// -1 to stop after 1
	
};

static xlua_timer * x_timers;
static xlua_timer * l_timers;
xlua_timer * xlua_create_timer(xlua_timer_f func, void * ref)
{
	for(xlua_timer * t = l_timers; t; t = t->m_next)
	if(t->m_func == func && t->m_ref == ref)
	{
		printf("ERROR: timer already exists.");
		return NULL;
	}

	xlua_timer * nt = new xlua_timer;
	nt->m_next = l_timers;
	l_timers = nt;
	nt->m_next_fire_time = -1.0;
	nt->m_repeat_interval = -1.0;
	nt->m_func = func;
	nt->m_ref = ref;
	return nt;
}

void xlua_run_timer(xlua_timer * t, double delay, double repeat)
{
	t->m_repeat_interval = repeat;
	if(delay == -1.0)
		t->m_next_fire_time = delay;
	else
		t->m_next_fire_time = xlua_get_simulated_time() + delay;
}

int xlua_is_timer_scheduled(xlua_timer * t)
{
	if(t == NULL)
		return 0;
	if(t->m_next_fire_time == -1.0)
		return 0;
	return 1;	
}
xlua_timer * xtlua_create_timer(xlua_timer_f func, void * ref)
{
	for(xlua_timer * t = x_timers; t; t = t->m_next)
	if(t->m_func == func && t->m_ref == ref)
	{
		printf("ERROR: timer already exists.");
		return NULL;
	}

	xlua_timer * nt = new xlua_timer;
	nt->m_next = x_timers;
	x_timers = nt;
	nt->m_next_fire_time = -1.0;
	nt->m_repeat_interval = -1.0;
	nt->m_func = func;
	nt->m_ref = ref;
	return nt;
}

void xtlua_run_timer(xlua_timer * t, double delay, double repeat)
{
	t->m_repeat_interval = repeat;
	if(delay == -1.0)
		t->m_next_fire_time = delay;
	else
		t->m_next_fire_time = xlua_get_simulated_time() + delay;
}

int xtlua_is_timer_scheduled(xlua_timer * t)
{
	if(t == NULL)
		return 0;
	if(t->m_next_fire_time == -1.0)
		return 0;
	return 1;	
}


void xtlua_do_timers_for_time(double now,bool isPaused)
{
	for(xlua_timer * t = x_timers; t; t = t->m_next)
	if(t->m_next_fire_time != -1.0 && t->m_next_fire_time <= now)
	{
		if(!isPaused)
			t->m_func(t->m_ref);
		if(t->m_repeat_interval == -1.0)
			t->m_next_fire_time = -1.0;
		else
			t->m_next_fire_time += t->m_repeat_interval;
	}	
}
void xlua_do_timers_for_time(double now,bool isPaused)
{
	for(xlua_timer * t = l_timers; t; t = t->m_next)
	if(t->m_next_fire_time != -1.0 && t->m_next_fire_time <= now)
	{
		t->m_func(t->m_ref);
		if(t->m_repeat_interval == -1.0)
			t->m_next_fire_time = -1.0;
		else
			t->m_next_fire_time += t->m_repeat_interval;
	}	
}
void xtlua_timer_cleanup()
{
	while(l_timers)
	{
		xlua_timer * k = l_timers;
		l_timers = l_timers->m_next;
		delete k;
	}
	while(x_timers)
	{
		xlua_timer * k = x_timers;
		x_timers = x_timers->m_next;
		delete k;
	}
}


/*double xlua_get_simulated_time(void)
{
	static XPLMDataRef sim_time = XPLMFindDataRef("sim/time/total_running_time_sec");
	return XPLMGetDataf(sim_time);
}*/
