/*
 * pwm.c
 * "library" to assist with interfacing with the T20's PWM kernel driver
 *
 * Copyright (C) 2019 Darell Tan
 * Licensed under the 3-clause ("modified") BSD License.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "pwm.h"

#define PWM_PATH "/dev/pwm"

#define PWM_IOCTL_CONFIG		0x001
#define PWM_IOCTL_CONFIG_DUTY	0x002
#define PWM_IOCTL_ENABLE		0x010
#define PWM_IOCTL_DISABLE		0x100

int pwm_fd = -1;

int pwm_init() {
	pwm_fd = open(PWM_PATH, O_RDONLY);
	if (pwm_fd < 0) 
		return -1;
	return 0;
}

void pwm_destroy() {
	if (pwm_fd < 0)
		close(pwm_fd);
	pwm_fd = -1;
}

static int _pwm_ioctl(unsigned long request, unsigned long param) {
	if (pwm_fd < 0)
		return -2;
	return ioctl(pwm_fd, request, param);
}

int pwm_config(struct pwm_ioctl_t *config) {
	return _pwm_ioctl(PWM_IOCTL_CONFIG, (unsigned long) config);
}

// Enables a channel.
// The channel needs to already have been configured using pwm_config
int pwm_enable(int channel) {
	return _pwm_ioctl(PWM_IOCTL_ENABLE, channel);
}

int pwm_disable(int channel) {
	return _pwm_ioctl(PWM_IOCTL_DISABLE, channel);
}

// Sets the duty cycle for the given channel
// Note that the duty cycle should not exceed the period, that's your
// responsibility to check. We cheat here, and the kernel module uses this
// value instead of checking its own value
int pwm_set_duty(int channel, int duty) {
	struct pwm_ioctl_t chn = {
		.channel = channel,
		.duty = duty,
		.period = duty,	// we cheat here coz this wont be changed
	};
	return _pwm_ioctl(PWM_IOCTL_CONFIG_DUTY, (unsigned long) &chn);
}

