#include "esp32/ulp.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "soc/sens_reg.h"
#include "soc/rtc_i2c_reg.h"
#include "esp_deep_sleep.h"
// Our header
#include "ulp_main.h"


// Unlike the esp-idf always use these binary blob names
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void init_run_ulp(uint32_t usec);

void setup() {
  
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        init_run_ulp(100 * 1000); // 100 msec
    }
  
    else {
    
      // ***** HERE YOUR SKETCH ***** 
      printf("Deep sleep wakeup\n");
      printf("ULP dummy i2c read value: %08x\n", ulp_dummy & UINT16_MAX);
    }

    // Re-entering deep sleep
    delay(10000); // Otherwise we are just spinning reading the same thing like crazy
    start_ulp_program();
    ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
    esp_deep_sleep_start();
}

void loop() {
    // Nothing goes here
}

// Use this function for all your first time init like setup() used to be
static void init_run_ulp(uint32_t usec) {
  
    // initialize ulp variables
    printf("initing...\n");
    
    ulp_set_wakeup_period(0, usec);

    // You MUST load the binary before setting shared variables!
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    // Setup RTC I2C controller, selection either 0 or 1, see below for exact description and wiring (you can mix and match and comment out unused gpios)
    SET_PERI_REG_BITS(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SDA_SEL, 1, RTC_IO_SAR_I2C_SDA_SEL_S);
    SET_PERI_REG_BITS(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SCL_SEL, 1, RTC_IO_SAR_I2C_SCL_SEL_S);

    // SDA/SCL selection 0 is for 
    // TOUCH0 = GPIO4 = SCL
    // TOUCH1 = GPIO0 = SDA
    rtc_gpio_init(GPIO_NUM_0);
    rtc_gpio_init(GPIO_NUM_4);

    // SDA/SCL selection 1 is for
    // TOUCH2 = GPIO2 = SCL
    // TOUCH3 = GPIO15 = SDA
    rtc_gpio_init(GPIO_NUM_2);
    rtc_gpio_init(GPIO_NUM_15);

    // We will be reading and writing on these gpio
    rtc_gpio_set_direction(GPIO_NUM_15, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_set_direction(GPIO_NUM_2, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_set_direction(GPIO_NUM_0, RTC_GPIO_MODE_INPUT_OUTPUT);

    // Look up your device for these standard values needed to do i2c:
    WRITE_PERI_REG(RTC_I2C_SCL_LOW_PERIOD_REG, 40); // SCL low/high period = 40, which result driving SCL with 100kHz.
    WRITE_PERI_REG(RTC_I2C_SCL_HIGH_PERIOD_REG, 40);  
    WRITE_PERI_REG(RTC_I2C_SDA_DUTY_REG, 16);     // SDA duty (delay) cycles from falling edge of SCL when SDA changes.
    WRITE_PERI_REG(RTC_I2C_SCL_START_PERIOD_REG, 30); // Number of cycles to wait after START condition.
    WRITE_PERI_REG(RTC_I2C_SCL_STOP_PERIOD_REG, 44);  // Number of cycles to wait after STOP condition.
    WRITE_PERI_REG(RTC_I2C_TIMEOUT_REG, 10000);     // Number of cycles before timeout.
    SET_PERI_REG_BITS(RTC_I2C_CTRL_REG, RTC_I2C_MS_MODE, 1, RTC_I2C_MS_MODE_S); // Set i2c mode to master for the ulp controller

    // Both aren't necessary, but for example of how to get ADDR0 when there is no ADDR0_REG
    // Also note this is SENS_SAR regs but everything else uses RTC_*
    SET_PERI_REG_BITS(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR0, 0x5a, SENS_I2C_SLAVE_ADDR0_S);  // Set I2C device address.
    SET_PERI_REG_BITS(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR1, 0x5a, SENS_I2C_SLAVE_ADDR1_S);  // Set I2C device address.

    
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Any other ulp variables can be assigned here
 
    printf("end of init");
}

static void start_ulp_program() {
    /* Start the program */
    printf("Going to sleep...");
    delay(100);
    esp_err_t err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
}
