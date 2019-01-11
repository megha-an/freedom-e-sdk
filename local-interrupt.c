
#include <stdio.h>
#include <mee/cpu.h>
#include <mee/led.h>
#include <mee/button.h>
#include <mee/switch.h>

#define RTC_FREQ	32768

int sw_status = 99;
struct mee_cpu *cpu0;
struct mee_interrupt *cpu_intr, *tmr_intr;
struct mee_interrupt *but0_ic, *but1_ic, *but2_ic, *but3_ic, *swch3_ic;
int tmr_id, but0_irq, but1_irq, but2_irq, but3_irq, swch3_irq;

void display_instruction (void) {
    printf("\n");
    printf("SIFIVE, INC.\n!!\n");
    printf("\n");
    printf("E31/E51 Coreplex IP Eval Kit 'local-interrupt' Example.\n\n");
    printf("Buttons 0-3 and Switch 3 are enabled as local interrupt sources.\n");
    printf("A 1s debounce timer is used between these interupts.\n");
    printf("Pressing Buttons 0-2 toggle LEDs, while turn ON Switch 3 to exit.\n");
    printf("\n");
}

void timer_isr (int id, void *data) {
    // Disable Timer interrupt
    mee_interrupt_disable(tmr_intr, tmr_id);
    printf("Awaken\n");

    // Enable local IRQs
    mee_interrupt_enable(but0_ic, but0_irq);
    mee_interrupt_enable(but1_ic, but1_irq);
    mee_interrupt_enable(but2_ic, but2_irq);
    mee_interrupt_enable(but3_ic, but3_irq);
    mee_interrupt_enable(swch3_ic, swch3_irq);
}

void debounce (void) {
    // Disable local IRQs
    mee_interrupt_disable(but0_ic, but0_irq);
    mee_interrupt_disable(but1_ic, but1_irq);
    mee_interrupt_disable(but2_ic, but2_irq);
    mee_interrupt_disable(but3_ic, but3_irq);
    mee_interrupt_disable(swch3_ic, swch3_irq);

    printf("Sleep for %d more times\n", sw_status--);
    mee_cpu_set_mtimecmp(cpu0, mee_cpu_get_mtime(cpu0) + RTC_FREQ);

    // Enable Timer interrupt
    mee_interrupt_enable(tmr_intr, tmr_id);
}

void button0_isr (int id, void *data) {
    printf("Button 0 was pressed. Toggle Red LED.\n");
    mee_led_toggle((struct mee_led *)data);
    debounce();  
};
void button1_isr (int id, void *data) {
    printf("Button 1 was pressed. Toggle Green LED.\n");
    mee_led_toggle((struct mee_led *)data);
    debounce();
};
void button2_isr (int id, void *data) {
    printf("Button 2 was pressed. Toggle Blue LED.\n");
    mee_led_toggle((struct mee_led *)data);
    debounce();
};
void button3_isr (int id, void *data) {
    printf("Button 3 was pressed. No LED change!\n");
    debounce();
};

void switch3_isr(int id, void *data) {
    mee_interrupt_disable(swch3_ic, swch3_irq);
    printf("Switch 3 is on!\n");
    sw_status = 0;
    printf("Time to exit!\n");
}

int main (void)
{
    int rc;
    struct mee_led *led0_red, *led0_green, *led0_blue;
    struct mee_button *but0, *but1, *but2, *but3;
    struct mee_switch *swch3;


    // Lets get start with getting LEDs and turn only RED ON
    led0_red = mee_led_get_rgb("LD0", "red");
    led0_green = mee_led_get_rgb("LD0", "green");
    led0_blue = mee_led_get_rgb("LD0", "blue");
    if ((led0_red == NULL) || (led0_green == NULL) || (led0_blue == NULL)) {
        printf("At least one of LEDs is null.\n");
        return 1;
    }
    mee_led_enable(led0_red);
    mee_led_enable(led0_green);
    mee_led_enable(led0_blue);
    mee_led_on(led0_red);
    mee_led_off(led0_green);
    mee_led_off(led0_blue);
 
    // Lets get the CPU and and its interrupt
    cpu0 = mee_cpu_get(0);
    if (cpu0 == NULL) {
        printf("CPU null.\n");
        return 2;
    }
    cpu_intr = mee_cpu_interrupt_controller(cpu0);
    if (cpu_intr == NULL) {
        printf("CPU interrupt controller is null.\n");
        return 3;
    }
    mee_interrupt_init(cpu_intr);

    // Setup Timer and its interrupt
    tmr_intr = mee_cpu_timer_interrupt_controller(cpu0);
    if (tmr_intr == NULL) {
        printf("TIMER interrupt controller is  null.\n");
        return 4;
    }
    mee_interrupt_init(tmr_intr);
    tmr_id = mee_cpu_timer_get_interrupt_id(cpu0);
    rc = mee_interrupt_register_handler(tmr_intr, tmr_id, timer_isr, cpu0);
    if (rc < 0) {
        printf("TIMER interrupt handler registration failed\n");
        return (rc * -1);
    }

    // Setup Buttons 0-3 and its interrupt
    but0 = mee_button_get("BTN0");
    but1 = mee_button_get("BTN1");
    but2 = mee_button_get("BTN2");
    but3 = mee_button_get("BTN3");
    if ((but0 == NULL) || (but1 == NULL) || (but2 == NULL) || (but3 == NULL)) {
        printf("At least one of Buttons is null.\n");
        return 1;
    }
    but0_ic = mee_button_interrupt_controller(but0);
    if (but0_ic == NULL) {
        printf("BTN0 interrupt controller is null.\n");
        return 4;
    }
    mee_interrupt_init(but0_ic);
    but0_irq = mee_button_get_interrupt_id(but0);
    rc = mee_interrupt_register_handler(but0_ic, but0_irq, button0_isr, led0_red);
    if (rc < 0) {
        printf("BTN0 interrupt handler registration failed\n");
        return (rc * -1);
    }
    but1_ic = mee_button_interrupt_controller(but1);
    if (but1_ic == NULL) {
        printf("BTN1 interrupt controller is null.\n");
        return 4;
    }
    mee_interrupt_init(but1_ic);
    but1_irq = mee_button_get_interrupt_id(but1);
    rc = mee_interrupt_register_handler(but1_ic, but1_irq, button1_isr, led0_green);
    if (rc < 0) {
        printf("BTN1 interrupt handler registration failed\n");
        return (rc * -1);
    }
    but2_ic = mee_button_interrupt_controller(but2);
    if (but2_ic == NULL) {
        printf("BTN2 interrupt controller is null.\n");
        return 4;
    }
    mee_interrupt_init(but2_ic);
    but2_irq = mee_button_get_interrupt_id(but2);
    rc = mee_interrupt_register_handler(but2_ic, but2_irq, button2_isr, led0_blue);
    if (rc < 0) {
        printf("BTN2 interrupt handler registration failed\n");
        return (rc * -1);
    }
    but3_ic = mee_button_interrupt_controller(but3);
    if (but3_ic == NULL) {
        printf("BTN3 interrupt controller is null.\n");
        return 4;
    }
    mee_interrupt_init(but3_ic);
    but3_irq = mee_button_get_interrupt_id(but3);
    rc = mee_interrupt_register_handler(but3_ic, but3_irq, button3_isr, 0);
    if (rc < 0) {
        printf("BTN3 interrupt handler registration failed\n");
        return (rc * -1);
    }

    // Setup Switch3 and its interrupt
    swch3 = mee_switch_get("SW3");
    if (swch3 == NULL) {
        printf("SW3 is null.\n");
        return 1;
    }
    swch3_ic = mee_switch_interrupt_controller(swch3);
    if (swch3_ic == NULL) {
        printf("SW3 interrupt controller is null.\n");
        return 4;
    }
    mee_interrupt_init(swch3_ic);
    swch3_irq = mee_switch_get_interrupt_id(swch3);
    rc = mee_interrupt_register_handler(swch3_ic, swch3_irq, switch3_isr, swch3);
    if (rc < 0) {
        printf("SW3 interrupt handler registration failed\n");
        return (rc * -1);
    }

    // Lets enable the Buttons and Switch interrupts
    if (mee_interrupt_enable(but0_ic, but0_irq) == -1) {
        printf("BTN0 interrupt enable failed\n");
        return 5;
    }
    if (mee_interrupt_enable(but1_ic, but1_irq) == -1) {
        printf("BTN1 interrupt enable failed\n");
    printf("BTN1 enable failed\n");
        return 5;
    }
    if (mee_interrupt_enable(but2_ic, but2_irq) == -1) {
        printf("BTN2 interrupt enable failed\n");
    printf("BTN2 enable failed\n");
        return 5;
    }
    if (mee_interrupt_enable(but3_ic, but3_irq) == -1) {
        printf("BTN3 interrupt enable failed\n");
    printf("BTN3 enable failed\n");
        return 5;
    }
    if (mee_interrupt_enable(swch3_ic, swch3_irq) == -1) {
        printf("SW3 interrupt enable failed\n");
    printf("SW3 enable failed\n");
        return 5;
    }
    // Lastly CPU interrupt
    if (mee_interrupt_enable(cpu_intr, 0) == -1) {
        printf("CPU interrupt enable failed\n");
        return 6;
    }

    display_instruction();

    while (sw_status) {
    }

    return sw_status;
}
