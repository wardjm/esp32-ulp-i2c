# esp32-ulp-i2c
If you are new to ULP, do yourself a favor and watch https://www.youtube.com/watch?v=-QIcUTBB7Ww to get familiar with the code structure.

What this does is reads a value from an i2c device while in Ultra Low Power Mode (ULP) on an ESP32. The cpu allows for a convenient opcode to read or write from the i2c bus (in roughly 4 cycles). To read, you simply use:

i2c_rd sub-address, high bit, low bit, slave_selection

But what is the SCL speed? What is the address of the i2c device? What pins are being used? Much has to be set up ahead of time to be able to use this. We provide a skeleton to make this process much easier.

# PINS

i2c requires two pins: SDA and SCL. The ULP allows you to choose between two for these values.

Selection 0:
* TOUCH0 = GPIO4 = RTC_GPIO10 = SCL (you would wire to GPIO4 if labeled or pin 24 on baremetal)
* TOUCH1 = GPIO0 = RTC_GPIO11 = SDA (you would wire to GPIO0 if labeled or pin 23 on baremetal)

Selection 1 (used in the checked in example):
* TOUCH2 = GPIO2 = RTC_GPIO12 = SCL (you would wire to GPIO2 or pin 22 on baremetal)
* TOUCH3 = GPIO15 = RTC_GPIO13 = SDA (you would wire to GPIO15 or pin 21 on baremetal)

Why all the different names for the same thing? I have no idea. Tracking all this down and using the right name in the right spot is what took so long to put together such a small example.

*NOTE: If you have an adafruit feather board, they do not expose these pins, so you cannot do this!*

# Making selections

Much of our register handling is done via macros that can be read in a certain way:

WRITE_PERI_REG(REGISTER, VALUE)
SET_PERI_REG_BITS(REGISTER, BIT_TO_SET, VALUE, SHIFT_TO_GET_TO_BIT_TO_SET)
WRITE_RTC_REG(REGISTER, FIELD, FIELD_MASK, VALUE) (field is a shift underneath the hood)

Many fields and registers have their own #defines to set the appropriate value. So the field is usually found with an \_S (aka, shift) and the mask with an \_M (aka, mask). Then we can input the value we wish to set.

# Selecting i2c address

We set the i2c slave address using the SENS_I2C_SLAVE_ADDRx register. This amounts to a simple:

WRITE_RTC_REG(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR1_S, SENS_I2C_SLAVE_ADDR1_M, 0x5a);

on the ulp if the i2c slave we wish to talk to uses address 0x5a. Then we can issue the assembly instruction:

i2c_rd 0x20, 7, 0, 1

which says read bits 7-0 (1 byte) from sub-address 0x20 from the i2c device whose address is stored in SLAVE_ADDR1 and store it in r0.

# Selecting i2c pins

We must select either 0 or 1 for SDA and SCL. The pins section above tells which pins to use for each selection. The selection is made using the RTC_IO_SAR_I2C_IO_REG register with fields RTC_IO_SAR_I2C_SDA_SEL and RTC_IO_SAR_I2C_SCL_SEL. To select option 1 for both we can do:

WRITE_RTC_REG(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SDA_SEL_S, RTC_IO_SAR_I2C_SDA_SEL_M, 1);
WRITE_RTC_REG(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SCL_SEL_S, RTC_IO_SAR_I2C_SCL_SEL_M, 1);

on the ulp or:

SET_PERI_REG_BITS(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SDA_SEL, 1, RTC_IO_SAR_I2C_SDA_SEL_S);
SET_PERI_REG_BITS(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SCL_SEL, 1, RTC_IO_SAR_I2C_SCL_SEL_S);

in the main process. Some people say state is lost when migrating down to low power, so this example does everything in both places to illustrate how it could be done from anywhere.
