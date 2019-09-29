/*
 * pwm.h
 * "library" to assist with interfacing with the T20's PWM kernel driver
 *
 * Copyright (C) 2019 Darell Tan
 * Licensed under the 3-clause ("modified") BSD License.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PWM_H__
#define __PWM_H__

struct pwm_ioctl_t {
	int channel;
	int duty;
	int period;
	int polarity;
};

int  pwm_init();
void pwm_destroy();

int pwm_config(struct pwm_ioctl_t *config);
int pwm_enable(int channel);
int pwm_disable(int channel);
int pwm_set_duty(int channel, int duty);

#endif

