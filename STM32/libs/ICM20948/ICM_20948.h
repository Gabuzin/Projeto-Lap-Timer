/*
 * ICM_20948.h
 *
 *  Created on: 25 de jul. de 2026
 *      Author: Gabriel D
 */


#ifndef ICM_20948_H
#define ICM_20948_H

#include "stm32f1xx.h"
#include "stdint.h"
#include <stdbool.h>
#include "stm32f1xx_hal.h"

/*** MACROS ***/
#define ADDRESS_I2C_AD0_FROM_GND			0x68
#define ADDRESS_I2C_AD0_FROM_VDD			0x69

/* ICM-20948 Registers */
#define ICM20948_ID						0xEA
#define REG_BANK_SEL					0x7F

// USER BANK 0
#define B0_WHO_AM_I						0x00
#define B0_USER_CTRL					0x03
#define B0_LP_CONFIG					0x05
#define B0_PWR_MGMT_1					0x06
#define B0_PWR_MGMT_2					0x07
#define B0_INT_PIN_CFG					0x0F
#define B0_INT_ENABLE					0x10
#define B0_INT_ENABLE_1					0x11
#define B0_INT_ENABLE_2					0x12
#define B0_INT_ENABLE_3					0x13
#define B0_I2C_MST_STATUS				0x17
#define B0_INT_STATUS					0x19
#define B0_INT_STATUS_1					0x1A
#define B0_INT_STATUS_2					0x1B
#define B0_INT_STATUS_3					0x1C
#define B0_DELAY_TIMEH					0x28
#define B0_DELAY_TIMEL					0x29
#define B0_ACCEL_XOUT_H					0x2D
#define B0_ACCEL_XOUT_L					0x2E
#define B0_ACCEL_YOUT_H					0x2F
#define B0_ACCEL_YOUT_L					0x30
#define B0_ACCEL_ZOUT_H					0x31
#define B0_ACCEL_ZOUT_L					0x32
#define B0_GYRO_XOUT_H					0x33
#define B0_GYRO_XOUT_L					0x34
#define B0_GYRO_YOUT_H					0x35
#define B0_GYRO_YOUT_L					0x36
#define B0_GYRO_ZOUT_H					0x37
#define B0_GYRO_ZOUT_L					0x38
#define B0_TEMP_OUT_H					0x39
#define B0_TEMP_OUT_L					0x3A
#define B0_EXT_SLV_SENS_DATA_00			0x3B
#define B0_EXT_SLV_SENS_DATA_01			0x3C
#define B0_EXT_SLV_SENS_DATA_02			0x3D
#define B0_EXT_SLV_SENS_DATA_03			0x3E
#define B0_EXT_SLV_SENS_DATA_04			0x3F
#define B0_EXT_SLV_SENS_DATA_05			0x40
#define B0_EXT_SLV_SENS_DATA_06			0x41
#define B0_EXT_SLV_SENS_DATA_07			0x42
#define B0_EXT_SLV_SENS_DATA_08			0x43
#define B0_EXT_SLV_SENS_DATA_09			0x44
#define B0_EXT_SLV_SENS_DATA_10			0x45
#define B0_EXT_SLV_SENS_DATA_11			0x46
#define B0_EXT_SLV_SENS_DATA_12			0x47
#define B0_EXT_SLV_SENS_DATA_13			0x48
#define B0_EXT_SLV_SENS_DATA_14			0x49
#define B0_EXT_SLV_SENS_DATA_15			0x4A
#define B0_EXT_SLV_SENS_DATA_16			0x4B
#define B0_EXT_SLV_SENS_DATA_17			0x4C
#define B0_EXT_SLV_SENS_DATA_18			0x4D
#define B0_EXT_SLV_SENS_DATA_19			0x4E
#define B0_EXT_SLV_SENS_DATA_20			0x4F
#define B0_EXT_SLV_SENS_DATA_21			0x50
#define B0_EXT_SLV_SENS_DATA_22			0x51
#define B0_EXT_SLV_SENS_DATA_23			0x52
#define B0_FIFO_EN_1					0x66
#define B0_FIFO_EN_2					0x67
#define B0_FIFO_RST						0x68
#define B0_FIFO_MODE					0x69
#define B0_FIFO_COUNTH					0X70
#define B0_FIFO_COUNTL					0X71
#define B0_FIFO_R_W						0x72
#define B0_DATA_RDY_STATUS				0x74
#define B0_FIFO_CFG						0x76

// USER BANK 1
#define B1_SELF_TEST_X_GYRO				0x02
#define B1_SELF_TEST_Y_GYRO				0x03
#define B1_SELF_TEST_Z_GYRO				0x04
#define B1_SELF_TEST_X_ACCEL			0x0E
#define B1_SELF_TEST_Y_ACCEL			0x0F
#define B1_SELF_TEST_Z_ACCEL			0x10
#define B1_XA_OFFS_H					0x14
#define B1_XA_OFFS_L					0x15
#define B1_YA_OFFS_H					0x17
#define B1_YA_OFFS_L					0x18
#define B1_ZA_OFFS_H					0x1A
#define B1_ZA_OFFS_L					0x1B
#define B1_TIMEBASE_CORRECTION_PLL		0x28

// USER BANK 2
#define B2_GYRO_SMPLRT_DIV				0x00
#define B2_GYRO_CONFIG  				0x01
#define B2_GYRO_CONFIG_2				0x02
#define B2_XG_OFFS_USRH					0x03
#define B2_XG_OFFS_USRL 				0x04
#define B2_YG_OFFS_USRH					0x05
#define B2_YG_OFFS_USRL					0x06
#define B2_ZG_OFFS_USRH					0x07
#define B2_ZG_OFFS_USRL					0x08
#define B2_ODR_ALIGN_EN					0x09
#define B2_ACCEL_SMPLRT_DIV_1			0x10
#define B2_ACCEL_SMPLRT_DIV_2			0x11
#define B2_ACCEL_INTEL_CTRL				0x12
#define B2_ACCEL_WOM_THR				0x13
#define B2_ACCEL_CONFIG					0x14
#define B2_ACCEL_CONFIG_2				0x15
#define B2_FSYNC_CONFIG					0x52
#define B2_TEMP_CONFIG					0x53
#define B2_MOD_CTRL_USR					0X54
// USER BANK 3
#define B3_I2C_MST_ODR_CONFIG			0x00
#define B3_I2C_MST_CTRL					0x01
#define B3_I2C_MST_DELAY_CTRL			0x02
#define B3_I2C_SLV0_ADDR				0x03
#define B3_I2C_SLV0_REG					0x04
#define B3_I2C_SLV0_CTRL				0x05
#define B3_I2C_SLV0_DO					0x06
#define B3_I2C_SLV1_ADDR				0x07
#define B3_I2C_SLV1_REG					0x08
#define B3_I2C_SLV1_CTRL				0x09
#define B3_I2C_SLV1_DO					0x0A
#define B3_I2C_SLV2_ADDR				0x0B
#define B3_I2C_SLV2_REG					0x0C
#define B3_I2C_SLV2_CTRL				0x0D
#define B3_I2C_SLV2_DO					0x0E
#define B3_I2C_SLV3_ADDR				0x0F
#define B3_I2C_SLV3_REG					0x10
#define B3_I2C_SLV3_CTRL				0x11
#define B3_I2C_SLV3_DO					0x12
#define B3_I2C_SLV4_ADDR				0x13
#define B3_I2C_SLV4_REG					0x14
#define B3_I2C_SLV4_CTRL				0x15
#define B3_I2C_SLV4_DO					0x16
#define B3_I2C_SLV4_DI					0x17


/* AK09916 Registers */
#define AK09916_ID						0x09
#define MAG_SLAVE_ADDR                  0x0C

#define MAG_WIA2						0x01
#define MAG_ST1							0x10
#define MAG_HXL							0x11
#define MAG_HXH							0x12
#define MAG_HYL							0x13
#define MAG_HYH							0x14
#define MAG_HZL							0x15
#define MAG_HZH							0x16
#define MAG_ST2							0x18
#define MAG_CNTL2						0x31
#define MAG_CNTL3						0x32
#define MAG_TS1							0x33
#define MAG_TS2							0x34

/* Typedefs */

typedef enum
{
    ICM20948_OK,
    ICM20948_FAIL,
    ICM20948_NOT_FOUND,
    ICM20948_I2C_NOT_FOUND,
    ICM20948_INVALID_ADDRESS,
    ICM20948_INVALID_REGISTER,
    ICM20948_UNKNOWN = 0xFF
} ICM20948_e;

typedef enum
{
    BANK_0 = (0 << 4),
    BANK_1 = (1 << 4),
    BANK_2 = (2 << 4),
    BANK_3 = (3 << 4)
} UserBank_t;

typedef enum
{
    GYRO_250DPS,
    GYRO_500DPS,
    GYRO_1000DPS,
    GYRO_2000DPS
} GyroScale_t;

typedef enum
{
    ACCEL_2G,
    ACCEL_4G,
    ACCEL_8G,
    ACCEL_16G
} AccelScale_t;

typedef enum
{
    MAG_POWER_DOWN             = 0,
    MAG_SINGLE_MEASUREMENT     = 1,
    MAG_CONTINUOUS_10HZ        = 2,
    MAG_CONTINUOUS_20HZ        = 4,
    MAG_CONTINUOUS_50HZ        = 6,
    MAG_CONTINUOUS_100HZ       = 8
} OperationMode_t;


typedef enum
{
    GYRO_DLPF_196HZ = 0,
    GYRO_DLPF_152HZ = 1,
    GYRO_DLPF_120HZ = 2,
    GYRO_DLPF_51HZ  = 3,
    GYRO_DLPF_24HZ  = 4,
    GYRO_DLPF_12HZ  = 5,
    GYRO_DLPF_6HZ   = 6,
    GYRO_DLPF_361HZ = 7
} GyroDlpf_t;

typedef enum
{
    ACEL_DLPF_246HZ = 0,
    ACEL_DLPF_246HZ_H = 1,
    ACEL_DLPF_111HZ = 2,
    ACEL_DLPF_50HZ  = 3,
    ACEL_DLPF_24HZ  = 4,
    ACEL_DLPF_12HZ  = 5,
    ACEL_DLPF_6HZ   = 6,
    ACEL_DLPF_473HZ = 7
} AcelDlpf_t;

typedef struct
{ /*
    ACCEL_CONFIG:
    bit 0 enable / disable  Low Pass
    bit 1:2 Scale
    bit 3:5 Low Pass Config
   */
    AccelScale_t accel_scale;
    AcelDlpf_t accel_dlpf;
    uint16_t accel_sample_rate_div;
    bool accel_dlpf_enable;

} ICM20948_Accel_Config_t;


typedef struct
{ /*
    GYRO_CONFIG_1:
    bit 0 enable / disable  Low Pass
    bit 1:2 Scale
    bit 3:5 Low Pass Config
   */
    GyroScale_t gyro_scale;
    GyroDlpf_t gyro_dlpf;
    uint8_t gyro_sample_rate_div;
    bool gyro_dlpf_enable;

} ICM20948_Gyro_Config_t;

typedef struct
{
	float x;
	float y;
	float z;
} axis;

typedef struct
{
	I2C_HandleTypeDef *i2c;
	uint8_t address;

	axis accel;
	axis gyro;
	axis mag;

	 axis acel_offset;
	 axis gyro_offset;

	ICM20948_Accel_Config_t AcelConfig;
    ICM20948_Gyro_Config_t GyroConfig;

}ICM20948_t;


ICM20948_e ICM20948_Reg_Write(ICM20948_t *handle,uint8_t reg, uint8_t data);
ICM20948_e ICM20948_Reg_Read(ICM20948_t *handle,uint8_t reg, uint8_t *data,uint8_t num_bytes);
ICM20948_e ICM20948_Who_Am_I(ICM20948_t *handle);
ICM20948_e ICM20948_SelectBank(ICM20948_t *handle,UserBank_t bank);


ICM20948_e ICM20948_Init(ICM20948_t *handle);
ICM20948_e ICM20948_Set_Acel_CONFIG(ICM20948_t *handle,ICM20948_Accel_Config_t AcelConfig);
ICM20948_e ICM20948_Set_Gyro_CONFIG(ICM20948_t *handle,ICM20948_Gyro_Config_t GyroConfig);
ICM20948_e ICM20948_Read(ICM20948_t *handle);
ICM20948_e ICM20948_Set_Sleep(ICM20948_t *handle, bool sleep);
ICM20948_e ICM20948_Reset(ICM20948_t *handle);
ICM20948_e ICM20948_Calibrate(ICM20948_t *handle, int count, uint32_t intervalMS);
#endif	/* __ICM20948_H__ */

