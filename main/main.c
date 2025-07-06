#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "tca9548.h"

typedef struct {
    I2C_HandleTypeDef* i2c_handle;
    uint8_t i2c_address;
} tca9548_bus_user_t;

tca9548_err_t tca9548_gpio_write_pin(void* user, uint32_t pin, bool state)
{
    GPIO_TypeDef* gpio = (GPIO_TypeDef*)user;

    HAL_GPIO_WritePin(gpio, (uint16_t)pin, (GPIO_PinState)state);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_transmit_data(void* user, uint8_t const* data, size_t data_size)
{
    tca9548_bus_user_t* bus_user = (tca9548_bus_user_t*)user;

    HAL_I2C_Master_Transmit(bus_user->i2c_handle, bus_user->i2c_address, data, data_size, 100);

    return TCA9548_ERR_OK;
}

tca9548_err_t tca9548_bus_receive_data(void* user, uint8_t const* data, size_t data_size)
{
    tca9548_bus_user_t* bus_user = (tca9548_bus_user_t*)user;

    HAL_I2C_Master_Receive(bus_user->i2c_handle, bus_user->i2c_address, data, data_size, 100);

    return TCA9548_ERR_OK;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    tca9548_t tca9548 = {};

    tca9548_initialize(
        &tca9548,
        &(tca9548_config_t){.reset_pin = 1 << 2},
        &(tca9548_interface_t){
            .gpio_user = GPIOC,
            .gpio_write_pin = tca9548_gpio_write_pin,
            .bus_user = &(tca9548_bus_user_t){.i2c_address = TCA9548_DEV_ADDRESS_A2_L_A1_L_A0_L,
                                              .i2c_handle = &hi2c3},
            .bus_transmit_data = tca9548_bus_transmit_data});

    while (1) {
        tca9548_err_t err = tca9548_set_channel(&tca9548, TCA9548_CHANNEL_0);
    }
}