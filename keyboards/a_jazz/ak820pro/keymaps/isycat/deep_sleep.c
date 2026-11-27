/* Deep sleep on battery for the AK820 Pro (SN32F290 / HFD80CP100).
 *
 * fpb's port only blanks the lights and screen when idle; the MCU keeps
 * running at 48 MHz. The stock firmware (V1.13, disassembled around 0x0CD4 and
 * 0x13C4/0x152A/0x176C) puts the MCU into the SN32 Deep-sleep mode instead:
 *   stop SysTick and timer IRQs -> pins to a wake-friendly state -> GPIO
 *   falling-edge interrupts on the key columns -> system clock back to IHRC,
 *   PLL off -> PMU_CTRL = 2 -> WFI -> PMU_CTRL = 0 -> PLL back on.
 * This file does the same. It runs with interrupts masked (PRIMASK), so the
 * GPIO interrupt only wakes the core and is never taken: no handler needed.
 *
 * Every key row is driven low and every column is an input with pull-up, so
 * any key press pulls its column low and wakes the chip. The knob and the
 * mode slider and plugging in the cable (charger status pins) also wake it.
 * The watchdog is off while asleep, as in stock (it clears WDTEN before WFI).
 * The key that woke it is still held
 * when scanning restarts, so it types normally.
 */
#include QMK_KEYBOARD_H
#include "deep_sleep.h"
#include "watchdog.h"

void SystemInit(void);

static const pin_t row_pins[]    = MATRIX_ROW_PINS;
static const pin_t col_pins[]    = MATRIX_COL_PINS;
static const pin_t led_rows[]    = SN32F2XX_RGB_MATRIX_ROW_PINS;
// Knob A/B, Mac/Win switch, mode slider, and the charger status pins (CHRG/STDBY)
// so plugging in the cable wakes it too. The stock firmware also arms P1.16/P1.17.
static const pin_t both_edges[]  = {B2, A10, B1, B12, B13, CHARGE_CHRG_PIN, CHARGE_STDBY_PIN};

typedef struct {
    PWMDriver *drv;
    uint8_t    ch;
} col_pwm_t;
static const col_pwm_t col_pwm[] = SN32F2XX_PWM_COL_MAP;

#define PORT_COUNT 4

static uint8_t port_index(pin_t pin) {
    switch ((uint32_t)PAL_PORT(pin)) {
        case SN_GPIO0_BASE: return 0;
        case SN_GPIO1_BASE: return 1;
        case SN_GPIO2_BASE: return 2;
        default: return 3;
    }
}

static const IRQn_Type port_irq[PORT_COUNT] = {P0_IRQn, P1_IRQn, P2_IRQn, P3_IRQn};
static ioportid_t const port_reg[PORT_COUNT] = {GPIOA, GPIOB, GPIOC, GPIOD};

static void wake_pin(pin_t pin, bool both, uint32_t *used) {
    uint8_t    p    = port_index(pin);
    ioportid_t port = port_reg[p];
    uint32_t   bit  = 1u << PAL_PAD(pin);
    port->IS &= ~bit; // edge sensitive
    if (both) {
        port->IBS |= bit;
    } else {
        port->IBS &= ~bit;
        port->IEV &= ~bit; // falling edge
    }
    port->IC = bit;
    port->IE |= bit;
    used[p] |= bit;
}

void deep_sleep_enter(void) {
    uint32_t used[PORT_COUNT] = {0};

    watchdog_stop();
    chSysLock(); // PRIMASK: interrupts wake WFI but are not taken

    // Only the GPIO interrupts may wake the core. Save the NVIC enables and
    // turn everything off (OS tick, PWM timers, RTC second tick, UART, USB).
    uint32_t iser = NVIC->ISER[0];
    NVIC->ICER[0] = 0xFFFFFFFFu;

    // LEDs off: PWM off on the column pins, RGB channel rows inactive (low).
    for (uint8_t i = 0; i < ARRAY_SIZE(col_pwm); i++) pwmDisableChannelI(col_pwm[i].drv, col_pwm[i].ch);
    for (uint8_t i = 0; i < ARRAY_SIZE(led_rows); i++) {
        gpio_set_pin_output_push_pull(led_rows[i]);
        gpio_write_pin_low(led_rows[i]);
    }
    // Key matrix: all rows low, columns pulled up and armed for a falling edge.
    for (uint8_t i = 0; i < ARRAY_SIZE(row_pins); i++) {
        gpio_set_pin_output_push_pull(row_pins[i]);
        gpio_write_pin_low(row_pins[i]);
    }
    for (uint8_t i = 0; i < ARRAY_SIZE(col_pins); i++) {
        gpio_set_pin_input_high(col_pins[i]);
    }
    for (volatile uint32_t i = 0; i < 500; i++) { // let the columns settle before arming the edges
    }
    for (uint8_t i = 0; i < ARRAY_SIZE(col_pins); i++) wake_pin(col_pins[i], false, used);
    for (uint8_t i = 0; i < ARRAY_SIZE(both_edges); i++) wake_pin(both_edges[i], true, used);
    for (uint8_t p = 0; p < PORT_COUNT; p++) {
        if (!used[p]) continue;
        NVIC_ClearPendingIRQ(port_irq[p]);
        NVIC_EnableIRQ(port_irq[p]);
    }

    // System clock back to IHRC and PLL off, as the stock firmware does.
    SN_SYS0->CLKCFG_b.SYSCLKSEL = 0;
    while (SN_SYS0->CLKCFG_b.SYSCLKST != 0) {
    }
    SN_SYS0->PLLCTRL_b.PLLEN = 0;

    SN_PMU->CTRL = 2; // WFI = Deep-sleep
    __DSB();
    __WFI();
    SN_PMU->CTRL = 0;

    // Back to 48 MHz from the PLL.
    SystemInit();
    SystemCoreClockUpdate();

    // Disarm the wake pins and restore the interrupts that were on before.
    for (uint8_t p = 0; p < PORT_COUNT; p++) {
        if (!used[p]) continue;
        port_reg[p]->IE &= ~used[p];
        port_reg[p]->IBS &= ~used[p];
        port_reg[p]->IC = used[p];
        NVIC_DisableIRQ(port_irq[p]);
        NVIC_ClearPendingIRQ(port_irq[p]);
    }
    // Rows go back to unselected (driven high, MATRIX_UNSELECT_DRIVE_HIGH);
    // the RGB driver re-drives the LED pins and columns on its next pass.
    for (uint8_t i = 0; i < ARRAY_SIZE(row_pins); i++) gpio_write_pin_high(row_pins[i]);
    NVIC->ISER[0] = iser;

    chSysUnlock();
    watchdog_start();
}
