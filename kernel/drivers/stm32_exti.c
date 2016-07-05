/*
 *      This file is part of frosted.
 *
 *      frosted is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License version 2, as
 *      published by the Free Software Foundation.
 *
 *
 *      frosted is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with frosted.  If not, see <http://www.gnu.org/licenses/>.
 *
 *      Authors: danielinux, DarkVegetableMatter
 *
 */
 
#include "frosted.h"
#include "device.h"
#include <stdint.h>
#include "ioctl.h"

#include <unicore-mx/stm32/gpio.h>
#include <unicore-mx/stm32/exti.h>
#include <unicore-mx/stm32/rcc.h>
#include <unicore-mx/cm3/nvic.h>

#include "gpio.h"
#include "stm32_exti.h"

struct dev_exti {
    int exti;                               /* Exti index in the dev_exti[] array */
    uint32_t base;                          /* GPIO base */
    uint16_t pin;                           /* GPIO pin */
    uint8_t trigger;                        /* EXTI trigger type */
    void (* exti_isr)(void);                /* ISR */
};

#define MAX_EXTIS   24

static struct dev_exti *DEV_EXTI[MAX_EXTIS];

void exti_isr(uint32_t exti_base, uint32_t exti_idx)
{
    struct dev_exti *dev = DEV_EXTI[exti_idx];
    exti_reset_request(exti_base);
    if (dev && dev->exti_isr) {
        dev->exti_isr();
    }
}

void exti0_isr(void)
{
    exti_isr(EXTI0, 0);
}

void exti1_isr(void)
{
    exti_isr(EXTI1, 1);
}

void exti2_isr(void)
{
    exti_isr(EXTI2, 2);
}

void exti3_isr(void)
{
    exti_isr(EXTI3, 3);
}

void exti4_isr(void)
{
    exti_isr(EXTI4, 4);
}

void exti9_5_isr(void)
{
    if(exti_get_flag_status(EXTI5))
        exti_isr(EXTI5, 5);
    if(exti_get_flag_status(EXTI6))
        exti_isr(EXTI6, 6);
    if(exti_get_flag_status(EXTI7))
        exti_isr(EXTI7, 7);
    if(exti_get_flag_status(EXTI8))
        exti_isr(EXTI8, 8);
    if(exti_get_flag_status(EXTI9))
        exti_isr(EXTI9, 9);
}

void exti15_10_isr(void)
{
    if(exti_get_flag_status(EXTI10))
        exti_isr(EXTI10, 10);
    if(exti_get_flag_status(EXTI11))
        exti_isr(EXTI11, 11);
    if(exti_get_flag_status(EXTI12))
        exti_isr(EXTI12, 12);
    if(exti_get_flag_status(EXTI13))
        exti_isr(EXTI13, 13);
    if(exti_get_flag_status(EXTI14))
        exti_isr(EXTI14, 14);
    if(exti_get_flag_status(EXTI15))
        exti_isr(EXTI15, 15);
}

static uint32_t exti_base(int idx)
{
    switch(idx)
    {
        case 0:
            return EXTI0;
        case 1:
            return EXTI1;
        case 2:
            return EXTI2;
        case 3:
            return EXTI3;
        case 4:
            return EXTI4;
        case 5:
            return EXTI5;
        case 6:
            return EXTI6;
        case 7:
            return EXTI7;
        case 8:
            return EXTI8;
        case 9:
            return EXTI9;
        case 10:
            return EXTI10;
        case 11:
            return EXTI11;
        case 12:
            return EXTI12;
        case 13:
            return EXTI13;
        case 14:
            return EXTI14;
        case 15:
            return EXTI15;
    }
    return (uint32_t)(-1);
}


static uint32_t gpio_base(int idx)
{
    switch(idx)
    {
        case 0:
            return GPIO0;
        case 1:
            return GPIO1;
        case 2:
            return GPIO2;
        case 3:
            return GPIO3;
        case 4:
            return GPIO4;
        case 5:
            return GPIO5;
        case 6:
            return GPIO6;
        case 7:
            return GPIO7;
        case 8:
            return GPIO8;
        case 9:
            return GPIO9;
        case 10:
            return GPIO10;
        case 11:
            return GPIO11;
        case 12:
            return GPIO12;
        case 13:
            return GPIO13;
        case 14:
            return GPIO14;
        case 15:
            return GPIO15;
    }
    return (uint32_t)(-1);
}

int exti_register(uint32_t base, uint16_t pin, uint8_t trigger, void (*isr)(void))
{

    uint32_t exti = exti_base(pin);
    uint32_t gpio = gpio_base(pin);
    struct dev_exti *e;

    if (DEV_EXTI[pin] != NULL)
        return -EEXIST;

    e = kalloc(sizeof(struct dev_exti));
    if (!e)
        return -ENOMEM;

    e->base = base;
    e->pin = pin;
    e->trigger = trigger;
    e->exti_isr = isr;
    exti_select_source(exti, base);
    exti_set_trigger(exti, trigger);
    DEV_EXTI[pin] = e;
    return pin;
}

void exti_unregister(int pin)
{
    if(DEV_EXTI[pin]) {
        kfree(DEV_EXTI[pin]);
        DEV_EXTI[pin] = NULL;
    }
}

int exti_enable(int idx, int enable)
{

    if (DEV_EXTI[idx]) {
        if(enable)
            exti_enable_request(exti_base(idx));
        else
            exti_disable_request(exti_base(idx));
        return 0;
    }
}


void exti_init(void)
{
    int i;
    uint32_t exti_irq;
    rcc_periph_clock_enable(RCC_SYSCFG);

    for(i=0; i < MAX_EXTIS; i++)
    {
        DEV_EXTI[i] = NULL;
    }
    nvic_enable_irq(NVIC_EXTI0_IRQ);
    nvic_enable_irq(NVIC_EXTI1_IRQ);
    nvic_enable_irq(NVIC_EXTI2_IRQ);
    nvic_enable_irq(NVIC_EXTI3_IRQ);
    nvic_enable_irq(NVIC_EXTI4_IRQ);
    nvic_enable_irq(NVIC_EXTI9_5_IRQ);
    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
}