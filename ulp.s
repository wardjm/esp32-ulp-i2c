#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/sens_reg.h"
#include "soc/rtc_i2c_reg.h"

/* Define variables, which go into .bss section (zero-initialized data) */
    .bss
.global dummy
dummy: .long 0

/* Code goes into .text section */
    .text


    .global entry
entry:
    // Release hold on pins, so that we can manipulate them
    WRITE_RTC_REG(RTC_IO_TOUCH_PAD3_REG, RTC_IO_TOUCH_PAD3_HOLD_S, RTC_IO_TOUCH_PAD3_HOLD_M, 0);
    WRITE_RTC_REG(RTC_IO_TOUCH_PAD2_REG, RTC_IO_TOUCH_PAD2_HOLD_S, RTC_IO_TOUCH_PAD2_HOLD_M, 0);

    // Select SDA/SCL pins to version 1, TOUCH2 and TOUCH3 (version 0 is TOUCH0 and TOUCH1)
    WRITE_RTC_REG(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SDA_SEL_S, RTC_IO_SAR_I2C_SDA_SEL_M, 1);
    WRITE_RTC_REG(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SCL_SEL_S, RTC_IO_SAR_I2C_SCL_SEL_M, 1);

    // Set slave address for i2c device we are talking to (ADDR1_REG contains both ADDR0 and ADDR1)
    // 0x5a is for CCS811
    WRITE_RTC_REG(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR0_S, SENS_I2C_SLAVE_ADDR0_M, 0x5a);
    WRITE_RTC_REG(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR1_S, SENS_I2C_SLAVE_ADDR1_M, 0x5a);

    // Set SCL speed to 100khz
    WRITE_RTC_REG(RTC_I2C_SCL_LOW_PERIOD_REG, RTC_I2C_SCL_LOW_PERIOD_S, RTC_I2C_SCL_LOW_PERIOD_M, 40);
    WRITE_RTC_REG(RTC_I2C_SCL_HIGH_PERIOD_REG, RTC_I2C_SCL_HIGH_PERIOD_S, RTC_I2C_SCL_HIGH_PERIOD_M, 40);

    // SDA duty (delay) cycles from falling edge of SCL when SDA changes.
    WRITE_RTC_REG(RTC_I2C_SDA_DUTY_REG, RTC_I2C_SDA_DUTY_S, RTC_I2C_SDA_DUTY_M, 16);

    // Number of cycles after start/stop condition
    WRITE_RTC_REG(RTC_I2C_SCL_START_PERIOD_REG, RTC_I2C_SCL_START_PERIOD_S, RTC_I2C_SCL_START_PERIOD_M, 30);
    WRITE_RTC_REG(RTC_I2C_SCL_STOP_PERIOD_REG, RTC_I2C_SCL_STOP_PERIOD_S, RTC_I2C_SCL_STOP_PERIOD_M, 44);

    // cycles before timeout
    WRITE_RTC_REG(RTC_I2C_TIMEOUT_REG, RTC_I2C_TIMEOUT_S, RTC_I2C_TIMEOUT_M, 10000);

    // Set mode to master
    WRITE_RTC_REG(RTC_I2C_CTRL_REG, RTC_I2C_MS_MODE_S, RTC_I2C_MS_MODE_M, 1);

    // Main code, store the i2c read (0x20 = get hardware id) into dummy and return the value to main process
    move    r3, dummy
    i2c_rd  0x20, 7, 0, 1
    st      r0, r3, 0
    jump    wake_up
    jump    entry

/* end the program */
.global exit
exit:
/* Set the GPIO13 output LOW (clear output) to signal that ULP is now going down (the 14 is because GPIO13 = RTC_GPIO_14) */
WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + 14, 1, 1)
/* Enable hold on GPIO13 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD4_REG, RTC_IO_TOUCH_PAD4_HOLD_S, 1, 1)
halt

.global wake_up
wake_up:
/* Check if the system can be woken up */
READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
and r0, r0, 1
jump exit, eq

/* Wake up the SoC, end program */
wake
WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
jump exit
