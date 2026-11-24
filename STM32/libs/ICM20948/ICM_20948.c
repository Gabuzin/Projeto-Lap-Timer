/*
 * ICM_20948.c
 *
 *  Created on: 25 de jul. de 2026
 *      Author: Gabriel D
 */


#include "ICM_20948.h"
#include <string.h>

ICM20948_e ICM20948_Reg_Write(ICM20948_t *handle,uint8_t reg, uint8_t data)
{
	HAL_StatusTypeDef _status;

    if (handle == NULL || handle->i2c == NULL)
        return ICM20948_I2C_NOT_FOUND;

    /* Endereços I2C possíveis do ICM-20948: 0x68 ou 0x69 */
    if (handle->address != 0x68 && handle->address != 0x69)
        return ICM20948_INVALID_ADDRESS;


    _status= HAL_I2C_Mem_Write(
        handle->i2c,
        handle->address << 1,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1,
        100);

return (_status == HAL_OK) ? ICM20948_OK : ICM20948_FAIL;
}
ICM20948_e ICM20948_Reg_Read(ICM20948_t *handle,uint8_t reg,uint8_t *data,uint8_t num_bytes)
{
    HAL_StatusTypeDef _status;
       if (handle == NULL || handle->i2c == NULL)
        return ICM20948_I2C_NOT_FOUND;

    if (handle->address != 0x68 && handle->address != 0x69)
        return ICM20948_INVALID_ADDRESS;

    if (data == NULL || num_bytes == 0)
        return ICM20948_FAIL;

    _status = HAL_I2C_Mem_Read(
        handle->i2c,
        handle->address << 1,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        num_bytes,
        100
    );

return (_status == HAL_OK) ? ICM20948_OK : ICM20948_FAIL;
}


ICM20948_e ICM20948_SelectBank(ICM20948_t *handle,UserBank_t bank)
{
    return ICM20948_Reg_Write(
        handle,
        REG_BANK_SEL,
        (uint8_t)bank
    );
}
ICM20948_e ICM20948_Who_Am_I(ICM20948_t *handle)
{
    uint8_t buffer;
    ICM20948_e status;

    status= ICM20948_SelectBank(handle,BANK_0);
    if(status != ICM20948_OK)
        return status;


    status=ICM20948_Reg_Read(handle,B0_WHO_AM_I,&buffer,1);
    if(status != ICM20948_OK)
        return status;

    if (buffer != ICM20948_ID)
    {
        return ICM20948_NOT_FOUND;
    }

    return ICM20948_OK;

}

ICM20948_e ICM20948_Set_Acel_CONFIG(ICM20948_t *handle,ICM20948_Accel_Config_t AcelConfig)
{
    uint8_t buffer=0;
    uint8_t sample_rate_MSB=0;
    uint8_t sample_rate_LSB=0;
    ICM20948_e status;
    buffer|=(AcelConfig.accel_dlpf << 3);
    buffer|=(AcelConfig.accel_scale << 1);
    buffer|=(AcelConfig.accel_dlpf_enable);
    sample_rate_MSB=(AcelConfig.accel_sample_rate_div >> 8) & 0x0FU;
    sample_rate_LSB=(AcelConfig.accel_sample_rate_div) & 0xFFU;

    status= ICM20948_SelectBank(handle,BANK_2);
    if(status != ICM20948_OK)
        return status;

    status= ICM20948_Reg_Write(handle,B2_ACCEL_CONFIG,buffer);
    if(status != ICM20948_OK)
        return status;

        status= ICM20948_Reg_Write(handle,B2_ACCEL_SMPLRT_DIV_1,sample_rate_MSB);
    if(status != ICM20948_OK)
        return status;

       status= ICM20948_Reg_Write(handle,B2_ACCEL_SMPLRT_DIV_2,sample_rate_LSB);
    if(status != ICM20948_OK)
        return status;

    handle->AcelConfig = AcelConfig;

    return ICM20948_OK;

}

ICM20948_e ICM20948_Set_Gyro_CONFIG(ICM20948_t *handle,ICM20948_Gyro_Config_t GyroConfig)
{
    uint8_t buffer=0;
    uint8_t sample_rate=0;

    ICM20948_e status;


    buffer|=(GyroConfig.gyro_dlpf << 3);
    buffer|=(GyroConfig.gyro_scale << 1);
    buffer|=(GyroConfig.gyro_dlpf_enable);

    sample_rate=(GyroConfig.gyro_sample_rate_div) & 0xFFU;



    status= ICM20948_SelectBank(handle,BANK_2);
    if(status != ICM20948_OK)
        return status;

    status= ICM20948_Reg_Write(handle,B2_GYRO_CONFIG,buffer);
    if(status != ICM20948_OK)
        return status;

        status= ICM20948_Reg_Write(handle,B2_GYRO_SMPLRT_DIV,sample_rate);
    if(status != ICM20948_OK)
        return status;

    handle->GyroConfig = GyroConfig;
    return ICM20948_OK;

}

ICM20948_e ICM20948_Calibrate(ICM20948_t *handle, int count, uint32_t intervalMS)
{
	ICM20948_e status;
    if (handle == NULL)
        return ICM20948_FAIL;

    float sum_ax = 0.0f;
    float sum_ay = 0.0f;
    float sum_az = 0.0f;
    float sum_gx = 0.0f;
    float sum_gy = 0.0f;
    float sum_gz = 0.0f;

    handle->gyro_offset.x = 0.0f;
    handle->gyro_offset.y = 0.0f;
    handle->gyro_offset.z = 0.0f;
    handle->acel_offset.x = 0.0f;
    handle->acel_offset.y = 0.0f;
    handle->acel_offset.z = 0.0f;

    for (uint16_t i = 0; i < count; i++)
    {
    	status = ICM20948_Read(handle);

        if (status != ICM20948_OK)
            return status;
        sum_ax += handle->accel.x;
        sum_ay += handle->accel.y;
        sum_az += handle->accel.z;
        sum_gx += handle->gyro.x;
        sum_gy += handle->gyro.y;
        sum_gz += handle->gyro.z;

        HAL_Delay(intervalMS);
    }

    handle->acel_offset.x = (sum_ax / (float)count);


    handle->acel_offset.y = (sum_ay / (float)count);


    handle->acel_offset.z = (sum_az / (float)count)-1;


    handle->gyro_offset.x = (sum_gx / (float)count);


    handle->gyro_offset.y =(sum_gy / (float)count);

    handle->gyro_offset.z = (sum_gz / (float)count);


    return ICM20948_OK;
}

ICM20948_e ICM20948_Init(ICM20948_t *handle)
{
    ICM20948_e status;
    ICM20948_Accel_Config_t accel_config;
    ICM20948_Gyro_Config_t gyro_config;

    if (handle == NULL || handle->i2c == NULL)
        return ICM20948_I2C_NOT_FOUND;

    if (handle->address != 0x68 && handle->address != 0x69)
        return ICM20948_INVALID_ADDRESS;

    status = ICM20948_Who_Am_I(handle);
    if (status != ICM20948_OK)
        return status;

    accel_config.accel_dlpf = ACEL_DLPF_24HZ;
    accel_config.accel_scale = ACCEL_16G;
    accel_config.accel_dlpf_enable = true;
    accel_config.accel_sample_rate_div = 4;

    status = ICM20948_Set_Acel_CONFIG(handle, accel_config);
    if (status != ICM20948_OK)
        return status;
    handle->AcelConfig = accel_config;

    gyro_config.gyro_dlpf_enable =true;
    gyro_config.gyro_dlpf= GYRO_DLPF_24HZ;
    gyro_config.gyro_scale= GYRO_2000DPS;
    gyro_config.gyro_sample_rate_div= 4;

    status = ICM20948_Set_Gyro_CONFIG(handle, gyro_config);
    if (status != ICM20948_OK)
        return status;
    handle->GyroConfig = gyro_config;


    status = ICM20948_Set_Sleep(handle, false);

    if (status != ICM20948_OK)
    {
        return status;
    }

    return ICM20948_SelectBank(handle, BANK_0);

}

static int16_t ICM20948_ConvertRaw(uint8_t msb, uint8_t lsb)
{
    return (int16_t)(((uint16_t)msb << 8) | lsb);
}

ICM20948_e ICM20948_Read(ICM20948_t *handle)
{
	uint8_t buffer[12];
	ICM20948_e status;
	float acel_div_factor=0,gyro_div_factor=0;


	switch(handle->AcelConfig.accel_scale)
	{
	case ACCEL_2G:
		acel_div_factor=16384.0f;
		break;
	case ACCEL_4G:
		acel_div_factor=8192.0f;
		break;
	case ACCEL_8G:
		acel_div_factor=4096.0f;
		break;
	case ACCEL_16G:
		acel_div_factor=2048.0f;
		break;
	}

	switch(handle->GyroConfig.gyro_scale)
	{
	case GYRO_250DPS:
		gyro_div_factor=131.0f;
		break;
	case GYRO_500DPS:
		gyro_div_factor=65.5f;
		break;
	case GYRO_1000DPS:
		gyro_div_factor=32.8f;
		break;
	case GYRO_2000DPS:
		gyro_div_factor=16.4f;
		break;
	}

    status= ICM20948_SelectBank(handle,BANK_0);

    if(status != ICM20948_OK)
        return status;


    status=ICM20948_Reg_Read(handle,B0_ACCEL_XOUT_H,buffer,12);
    if(status != ICM20948_OK)
        return status;



    handle->accel.x = (float)ICM20948_ConvertRaw(buffer[0], buffer[1])/acel_div_factor;
    handle->accel.y = (float)ICM20948_ConvertRaw(buffer[2], buffer[3])/acel_div_factor;
    handle->accel.z = (float)ICM20948_ConvertRaw(buffer[4], buffer[5])/acel_div_factor;
    handle->gyro.x =  (float)ICM20948_ConvertRaw(buffer[6], buffer[7])/gyro_div_factor;
    handle->gyro.y =  (float)ICM20948_ConvertRaw(buffer[8], buffer[9])/gyro_div_factor;
    handle->gyro.z =  (float)ICM20948_ConvertRaw(buffer[10], buffer[11])/gyro_div_factor;

    return ICM20948_OK;
}

ICM20948_e ICM20948_Reset(ICM20948_t *handle)
{

	uint8_t buffer= (1<<7);
	ICM20948_e status;
    status = ICM20948_SelectBank(handle, BANK_0);

    if (status != ICM20948_OK)
    {
        return status;
    }

    return ICM20948_Reg_Write(
        handle,
        B0_PWR_MGMT_1,
		buffer
    );

}

ICM20948_e ICM20948_Set_Sleep(ICM20948_t *handle, bool sleep)
{

	uint8_t config;
	ICM20948_e status;



    status = ICM20948_SelectBank(handle, BANK_0);

    if (status != ICM20948_OK)
    {
        return status;
    }

    status = ICM20948_Reg_Read(
        handle,
        B0_PWR_MGMT_1,
        &config,
        1
    );

    if (status != ICM20948_OK)
    {
        return status;
    }

	if(sleep)
		config|= (1<<6);
	else
		config&= ~(1<<6);



    return ICM20948_Reg_Write(
        handle,
        B0_PWR_MGMT_1,
		config
    );
}
