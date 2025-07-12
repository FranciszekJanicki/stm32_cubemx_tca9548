#include "main.h"
#include "as5600.h"
#include "as5600_config.h"
#include "gpio.h"
#include "i2c.h"
#include "main.h"
#include "stdio.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "tca9548.h"
#include "usart.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

tca9548_err_t tca9548_gpio_write_pin(void* user, uint32_t pin, bool state)
{
    GPIO_TypeDef* gpio = (GPIO_TypeDef*)user;

    HAL_GPIO_WritePin(gpio, (uint16_t)pin, (GPIO_PinState)state);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_transmit_data(void* user, uint8_t const* data, size_t data_size)
{
    HAL_I2C_Master_Transmit(&hi2c2, TCA9548_SLAVE_ADDRESS_A2L_A1L_A0L << 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_receive_data(void* user, uint8_t* data, size_t data_size)
{
    HAL_I2C_Master_Receive(&hi2c2, TCA9548_SLAVE_ADDRESS_A2L_A1L_A0L << 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_slave_write_data(void* user,
                                           uint16_t slave_address,
                                           uint8_t reg_address,
                                           uint8_t const* data,
                                           size_t data_size)
{
    HAL_I2C_Mem_Write(&hi2c2, slave_address << 1, reg_address, 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_slave_read_data(void* user,
                                          uint16_t slave_address,
                                          uint8_t reg_address,
                                          uint8_t* data,
                                          size_t data_size)
{
    HAL_I2C_Mem_Read(&hi2c2, slave_address << 1, reg_address, 1, data, data_size, 100);

    return TCA9548_ERR_OK;
}

as5600_err_t as5600_bus_write_data(void* user,
                                   uint8_t address,
                                   uint8_t const* data,
                                   size_t data_size)
{
    tca9548_t* tca9548 = (tca9548_t*)user;

    tca9548_write_channel_data(tca9548,
                               TCA9548_CHANNEL_0,
                               AS5600_SLAVE_ADDRESS,
                               address,
                               data,
                               data_size);

    return AS5600_ERR_OK;
}

as5600_err_t as5600_bus_read_data(void* user, uint8_t address, uint8_t* data, size_t data_size)
{
    tca9548_t* tca9548 = (tca9548_t*)user;

    tca9548_read_channel_data(tca9548,
                              TCA9548_CHANNEL_0,
                              AS5600_SLAVE_ADDRESS,
                              address,
                              data,
                              data_size);

    return AS5600_ERR_OK;
}

as5600_err_t as5600_init_chip(as5600_t* as5600, float32_t min_angle, float32_t max_angle)
{
    as5600_status_reg_t status;
    as5600_err_t err = as5600_get_status_reg(as5600, &status);
    if (err != AS5600_ERR_OK)
        return err;

    float32_t angle_range = (max_angle - min_angle);

    uint16_t min_raw = (uint16_t)(min_angle / angle_range * 4095.0F);
    uint16_t max_raw = (uint16_t)(max_angle / angle_range * 4095.0F);

    as5600_zpos_reg_t zpos = {.zpos = min_raw & 0x0FFF};
    err = as5600_set_zpos_reg(as5600, &zpos);
    if (err != AS5600_ERR_OK)
        return err;

    as5600_mpos_reg_t mpos = {.mpos = max_raw & 0x0FFF};
    err = as5600_set_mpos_reg(as5600, &mpos);
    if (err != AS5600_ERR_OK)
        return err;

    as5600_conf_reg_t conf = {.wd = AS5600_WATCHDOG_OFF,
                              .fth = AS5600_SLOW_FILTER_X16,
                              .sf = AS5600_SLOW_FILTER_X16,
                              .pwmf = AS5600_PWM_FREQUENCY_115HZ,
                              .outs = AS5600_FAST_FILTER_THRESH_SLOW,
                              .hyst = AS5600_HYSTERESIS_OFF,
                              .pm = AS5600_POWER_MODE_NOM};
    err = as5600_set_conf_reg(as5600, &conf);
    if (err != AS5600_ERR_OK)
        return err;

    as5600_zmco_reg_t zmco;
    err = as5600_get_zmco_reg(as5600, &zmco);
    if (err != AS5600_ERR_OK)
        return err;

    return AS5600_ERR_OK;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C2_Init();
    MX_USART2_UART_Init();

    HAL_Delay(500);

    tca9548_t tca9548 = {};

    tca9548_initialize(&tca9548,
                       &(tca9548_config_t){},
                       &(tca9548_interface_t){.bus_transmit_data = tca9548_bus_transmit_data,
                                              .bus_receive_data = tca9548_bus_receive_data,
                                              .bus_slave_write_data = tca9548_bus_slave_write_data,
                                              .bus_slave_read_data = tca9548_bus_slave_read_data});

    tca9548_channel_t channel = TCA9548_CHANNEL_0;
    tca9548_set_channel(&tca9548, channel);

    HAL_Delay(500);

    as5600_t as5600 = {};

    as5600_initialize(&as5600,
                      &(as5600_config_t){.min_angle = 0.0F, .max_angle = 180.0F},
                      &(as5600_interface_t){.bus_user = &tca9548,
                                            .bus_write_data = as5600_bus_write_data,
                                            .bus_read_data = as5600_bus_read_data});

    as5600_init_chip(&as5600, 0.0F, 180.0F);

    float32_t scaled;
    while (1) {
        as5600_get_angle_data_scaled_bus(&as5600, &scaled);
        printf("scaled: %f\n\r", scaled);
    }
}
