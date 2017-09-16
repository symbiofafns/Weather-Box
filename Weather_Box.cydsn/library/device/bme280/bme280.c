#include <project.h>
#include <string.h>
#include "bme280.h"

#define BME280_ADDRESS                          0x76
#define Macro_Is_Fail()                         \
do{                                             \
    if(res != p_iic_bus_I2C_MSTR_NO_ERROR){     \
        status = -1;                            \
        break;                                  \
    }                                           \
}while(0)


static 
struct bme280_t bme280_cluster;

static
u8 bme280_chip_id = 0x00;

static
s8 bme280_bus_write(u8 reg, u8 data[], u8 count)
{
    uint32  res;
    u8      w_byte;
    s8      status;
    
    p_iic_bus_I2CMasterClearStatus();
    status = 0;
    do{
        res = p_iic_bus_I2CMasterSendStart(BME280_ADDRESS,p_iic_bus_I2C_WRITE_XFER_MODE );
        Macro_Is_Fail();
    
        res = p_iic_bus_I2CMasterWriteByte(reg);
        Macro_Is_Fail();
   
        while(count){
            count  -= 1; 
            w_byte  = *data;
            data   += 1;
            res     = p_iic_bus_I2CMasterWriteByte(w_byte);
            Macro_Is_Fail();
        }
    }while(0);

    p_iic_bus_I2CMasterSendStop();
    
    return(status);
}

static
s8 bme280_bus_read_register(u8 reg, u8 data[], u8 count)
{
    uint32  res;
    u8      r_byte;
    s8      status;    
    
    p_iic_bus_I2CMasterClearStatus();
    status = 0;
    do{
        res = p_iic_bus_I2CMasterSendStart(BME280_ADDRESS,p_iic_bus_I2C_WRITE_XFER_MODE);
        Macro_Is_Fail();
    
        res = p_iic_bus_I2CMasterWriteByte(reg);
        Macro_Is_Fail();
        
        res = p_iic_bus_I2CMasterSendRestart(BME280_ADDRESS,p_iic_bus_I2C_READ_XFER_MODE);
        Macro_Is_Fail();
        
        while(count){
            count  -= 1;
            if(count == 0){
                r_byte  = p_iic_bus_I2CMasterReadByte(p_iic_bus_I2C_NAK_DATA);
            }else{
                r_byte  = p_iic_bus_I2CMasterReadByte(p_iic_bus_I2C_ACK_DATA);
            }
            *data = r_byte;
            data++;
        }        
    }while(0);
    
    p_iic_bus_I2CMasterSendStop();
    
    return(status);
}

/*!
 * @brief Reads actual temperature from uncompensated temperature
 * @note Returns the value in 0.01 degree Centigrade
 * Output value of "5123" equals 51.23 DegC.
 *
 *
 *
 *  @param  v_uncomp_temperature_s32 : value of uncompensated temperature
 *
 *
 *  @return Returns the actual temperature
 *
*/
static
s32 bme280_compensate_temperature_int32(s32 v_uncomp_temperature_s32)
{
	s32 v_x1_u32r   = BME280_INIT_VALUE;
	s32 v_x2_u32r   = BME280_INIT_VALUE;
	s32 temperature = BME280_INIT_VALUE;

	/* calculate x1*/
	v_x1_u32r  = ((((v_uncomp_temperature_s32               >> BME280_SHIFT_BIT_POSITION_BY_03_BITS) -
	               ((s32)bme280_cluster.cal_param.dig_T1    << BME280_SHIFT_BIT_POSITION_BY_01_BIT))) *
	               ((s32)bme280_cluster.cal_param.dig_T2))  >> BME280_SHIFT_BIT_POSITION_BY_11_BITS;
	
    /* calculate x2*/
	v_x2_u32r  = (((((v_uncomp_temperature_s32               >> BME280_SHIFT_BIT_POSITION_BY_04_BITS) -
	                ((s32)bme280_cluster.cal_param.dig_T1))	 * ((v_uncomp_temperature_s32 >> BME280_SHIFT_BIT_POSITION_BY_04_BITS) -
	                ((s32)bme280_cluster.cal_param.dig_T1))) >> BME280_SHIFT_BIT_POSITION_BY_12_BITS) *
    	            ((s32)bme280_cluster.cal_param.dig_T3))  >> BME280_SHIFT_BIT_POSITION_BY_14_BITS;
	
    /* calculate t_fine*/
	bme280_cluster.cal_param.t_fine = v_x1_u32r + v_x2_u32r;
	
    /* calculate temperature*/
	temperature  = (bme280_cluster.cal_param.t_fine * 5 + 128) >> BME280_SHIFT_BIT_POSITION_BY_08_BITS;
	
    return temperature;
}
/*!
 * @brief Reads actual pressure from uncompensated pressure
 * @note Returns the value in Pascal(Pa)
 * Output value of "96386" equals 96386 Pa =
 * 963.86 hPa = 963.86 millibar
 *
 *
 *
 *  @param v_uncomp_pressure_s32 : value of uncompensated pressure
 *
 *
 *
 *  @return Return the actual pressure output as u32
 *
*/
static
u32 bme280_compensate_pressure_int32(s32 v_uncomp_pressure_s32)
{
	s32 v_x1_u32        = BME280_INIT_VALUE;
	s32 v_x2_u32        = BME280_INIT_VALUE;
	u32 v_pressure_u32  = BME280_INIT_VALUE;

	/* calculate x1*/
	v_x1_u32 = (((s32)bme280_cluster.cal_param.t_fine) >> BME280_SHIFT_BIT_POSITION_BY_01_BIT) - (s32)64000;
	
    /* calculate x2*/
	v_x2_u32 = (((v_x1_u32                             >> BME280_SHIFT_BIT_POSITION_BY_02_BITS)  * 
                 (v_x1_u32                             >> BME280_SHIFT_BIT_POSITION_BY_02_BITS)) >> BME280_SHIFT_BIT_POSITION_BY_11_BITS) * 
               ((s32)bme280_cluster.cal_param.dig_P6);
	
    /* calculate x2*/
	v_x2_u32 = v_x2_u32 + ((v_x1_u32 * ((s32)bme280_cluster.cal_param.dig_P5)) << BME280_SHIFT_BIT_POSITION_BY_01_BIT);
	
    /* calculate x2*/
	v_x2_u32 = (v_x2_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS) + (((s32)bme280_cluster.cal_param.dig_P4) << BME280_SHIFT_BIT_POSITION_BY_16_BITS);
	
    /* calculate x1*/
	v_x1_u32 = (((bme280_cluster.cal_param.dig_P3 * 
                (((v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS) *
	             (v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS)) >> BME280_SHIFT_BIT_POSITION_BY_13_BITS)) >> BME280_SHIFT_BIT_POSITION_BY_03_BITS) +
	            ((((s32)bme280_cluster.cal_param.dig_P2) * v_x1_u32) >> BME280_SHIFT_BIT_POSITION_BY_01_BIT))  >> BME280_SHIFT_BIT_POSITION_BY_18_BITS;
	
    /* calculate x1*/
	v_x1_u32 = ((((32768 + v_x1_u32)) * ((s32)bme280_cluster.cal_param.dig_P1)) >> BME280_SHIFT_BIT_POSITION_BY_15_BITS);
	
    /* calculate pressure*/
	v_pressure_u32 = (((u32)(((s32)1048576) - v_uncomp_pressure_s32) - (v_x2_u32 >> BME280_SHIFT_BIT_POSITION_BY_12_BITS))) * 3125;
	
    if(v_pressure_u32 < 0x80000000){
		/* Avoid exception caused by division by zero */
		if (v_x1_u32 != BME280_INIT_VALUE){
			v_pressure_u32 = (v_pressure_u32 << BME280_SHIFT_BIT_POSITION_BY_01_BIT) / ((u32)v_x1_u32);
        }else{
			return BME280_INVALID_DATA;
        }
	}else{
		/* Avoid exception caused by division by zero */
		if (v_x1_u32 != BME280_INIT_VALUE){
			v_pressure_u32 = (v_pressure_u32 / (u32)v_x1_u32) * 2;
		}else{
			return BME280_INVALID_DATA;
        }    

		v_x1_u32 = (((s32)bme280_cluster.cal_param.dig_P9)                            * 
                   ( (s32)(((v_pressure_u32 >> BME280_SHIFT_BIT_POSITION_BY_03_BITS)  * 
                            (v_pressure_u32 >> BME280_SHIFT_BIT_POSITION_BY_03_BITS)) >> 
                             BME280_SHIFT_BIT_POSITION_BY_13_BITS)))                  >> 
                             BME280_SHIFT_BIT_POSITION_BY_12_BITS;
		
        v_x2_u32 = (((s32)(v_pressure_u32                         >> 
                           BME280_SHIFT_BIT_POSITION_BY_02_BITS)) *
		            ((s32)bme280_cluster.cal_param.dig_P8))    	  >> 
                           BME280_SHIFT_BIT_POSITION_BY_13_BITS;
		
        v_pressure_u32 = (u32)((s32)v_pressure_u32  + 
                              ((v_x1_u32 + v_x2_u32 + bme280_cluster.cal_param.dig_P7) >> BME280_SHIFT_BIT_POSITION_BY_04_BITS));
    }
	return v_pressure_u32;
}

/*!
 * @brief Reads actual humidity from uncompensated humidity
 * @note Returns the value in %rH as unsigned 32bit integer
 * in Q22.10 format(22 integer 10 fractional bits).
 * @note An output value of 42313
 * represents 42313 / 1024 = 41.321 %rH
 *
 *
 *
 *  @param  v_uncomp_humidity_s32: value of uncompensated humidity
 *
 *  @return Return the actual relative humidity output as u32
 *
*/
static
u32 bme280_compensate_humidity_int32(s32 v_uncomp_humidity_s32)
{
	s32 v_x1_u32 = BME280_INIT_VALUE;

	/* calculate x1*/
	v_x1_u32 = (bme280_cluster.cal_param.t_fine - ((s32)76800));
	/* calculate x1*/
	v_x1_u32 = (((((v_uncomp_humidity_s32 << BME280_SHIFT_BIT_POSITION_BY_14_BITS)                                -
	            (((s32)bme280_cluster.cal_param.dig_H4)	<< BME280_SHIFT_BIT_POSITION_BY_20_BITS)                  -
	            (((s32)bme280_cluster.cal_param.dig_H5) * v_x1_u32))                                              +     
	            ((s32)16384)) >> BME280_SHIFT_BIT_POSITION_BY_15_BITS)                                            * 
               (((((((v_x1_u32 * ((s32)bme280_cluster.cal_param.dig_H6)) >> BME280_SHIFT_BIT_POSITION_BY_10_BITS) *
	            ((   (v_x1_u32 * ((s32)bme280_cluster.cal_param.dig_H3)) >> BME280_SHIFT_BIT_POSITION_BY_11_BITS) + 
                ((s32)32768))) >> BME280_SHIFT_BIT_POSITION_BY_10_BITS)                                           + 
                ((s32)2097152)) * ((s32)bme280_cluster.cal_param.dig_H2) + 8192) >> 14));
	
    v_x1_u32 = (v_x1_u32 - (((((v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_15_BITS) * 
               (v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_15_BITS)) >> BME280_SHIFT_BIT_POSITION_BY_07_BITS) *
	           ((s32)bme280_cluster.cal_param.dig_H1)) >> BME280_SHIFT_BIT_POSITION_BY_04_BITS));
	
    v_x1_u32 = (v_x1_u32 < 0 ? 0 : v_x1_u32);
	v_x1_u32 = (v_x1_u32 > 419430400 ? 419430400 : v_x1_u32);
	
    return (u32)(v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_12_BITS);
}

/*!
 *	@brief This API is used to
 *	calibration parameters used for calculation in the registers
 *
 *  parameter | Register address |   bit
 *------------|------------------|----------------
 *	dig_T1    |  0x88 and 0x89   | from 0 : 7 to 8: 15
 *	dig_T2    |  0x8A and 0x8B   | from 0 : 7 to 8: 15
 *	dig_T3    |  0x8C and 0x8D   | from 0 : 7 to 8: 15
 *	dig_P1    |  0x8E and 0x8F   | from 0 : 7 to 8: 15
 *	dig_P2    |  0x90 and 0x91   | from 0 : 7 to 8: 15
 *	dig_P3    |  0x92 and 0x93   | from 0 : 7 to 8: 15
 *	dig_P4    |  0x94 and 0x95   | from 0 : 7 to 8: 15
 *	dig_P5    |  0x96 and 0x97   | from 0 : 7 to 8: 15
 *	dig_P6    |  0x98 and 0x99   | from 0 : 7 to 8: 15
 *	dig_P7    |  0x9A and 0x9B   | from 0 : 7 to 8: 15
 *	dig_P8    |  0x9C and 0x9D   | from 0 : 7 to 8: 15
 *	dig_P9    |  0x9E and 0x9F   | from 0 : 7 to 8: 15
 *	dig_H1    |  0xA1            | from 0 to 7
 *	dig_H2    |  0xE1 and 0xE2   | from 0 : 7 to 8: 15
 *	dig_H3    |  0xE3            | from 0 to 7
 *	dig_H4    |  0xE4 and 0xE5   | from 4 : 11 to 0: 3
 *	dig_H5    |  0xE5 and 0xE6   | from 0 : 3 to 4: 11
 *	dig_H6    |  0xE7            | from 0 to 7
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
static
BME280_RETURN_FUNCTION_TYPE bme280_get_calib_param(void)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt  = ERROR;
    BME280_RETURN_FUNCTION_TYPE com1_rslt = ERROR;
	u8 a_data_u8[BME280_CALIB_DATA_SIZE] = {
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE};
	
    com_rslt =  bme280_bus_read_register(BME280_TEMPERATURE_CALIB_DIG_T1_LSB_REG,
                                         a_data_u8,
                                         BME280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH);

	bme280_cluster.cal_param.dig_T1 = (u16)((((u16)((u8)a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T1_MSB])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T1_LSB]);
	bme280_cluster.cal_param.dig_T2 = (s16)((((s16)((s8)a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T2_MSB])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T2_LSB]);
    bme280_cluster.cal_param.dig_T3 = (s16)((((s16)((s8)a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T3_MSB])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T3_LSB]);
    bme280_cluster.cal_param.dig_P1 = (u16)((((u16)((u8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P1_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P1_LSB   ]);
    bme280_cluster.cal_param.dig_P2 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P2_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P2_LSB   ]);
    bme280_cluster.cal_param.dig_P3 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P3_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P3_LSB   ]);
    bme280_cluster.cal_param.dig_P4 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P4_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) |
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P4_LSB   ]);
    bme280_cluster.cal_param.dig_P5 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P5_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) |
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P5_LSB   ]);
    bme280_cluster.cal_param.dig_P6 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P6_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) |
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P6_LSB   ]);
    bme280_cluster.cal_param.dig_P7 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P7_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P7_LSB   ]);
    bme280_cluster.cal_param.dig_P8 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P8_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P8_LSB   ]);
    bme280_cluster.cal_param.dig_P9 = (s16)((((s16)((s8)a_data_u8[BME280_PRESSURE_CALIB_DIG_P9_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) |
                                                        a_data_u8[BME280_PRESSURE_CALIB_DIG_P9_LSB   ]);
    bme280_cluster.cal_param.dig_H1 =                   a_data_u8[BME280_HUMIDITY_CALIB_DIG_H1       ];
    
    com1_rslt = bme280_bus_read_register(BME280_HUMIDITY_CALIB_DIG_H2_LSB_REG,
                                         a_data_u8,
                                         BME280_HUMIDITY_CALIB_DATA_LENGTH);
    com_rslt += com1_rslt;
    
    bme280_cluster.cal_param.dig_H2 = (s16)((((s16)((s8)a_data_u8[BME280_HUMIDITY_CALIB_DIG_H2_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS) | 
                                                        a_data_u8[BME280_HUMIDITY_CALIB_DIG_H2_LSB   ]);
    bme280_cluster.cal_param.dig_H3 =  a_data_u8[BME280_HUMIDITY_CALIB_DIG_H3];
    bme280_cluster.cal_param.dig_H4 = (s16)((((s16)((s8)a_data_u8[BME280_HUMIDITY_CALIB_DIG_H4_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
                                                   (((u8)BME280_MASK_DIG_H4) & a_data_u8[BME280_HUMIDITY_CALIB_DIG_H4_LSB]));
    bme280_cluster.cal_param.dig_H5 = (s16)((((s16)((s8)a_data_u8[BME280_HUMIDITY_CALIB_DIG_H5_MSB   ])) << BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
                                                       (a_data_u8[BME280_HUMIDITY_CALIB_DIG_H4_LSB   ]   >> BME280_SHIFT_BIT_POSITION_BY_04_BITS));
    bme280_cluster.cal_param.dig_H6 = (s8)a_data_u8[BME280_HUMIDITY_CALIB_DIG_H6];

    return com_rslt;
}

/*!
 * @brief This API used to read uncompensated
 * pressure,temperature and humidity
 *
 *
 *
 *
 *  @param  v_uncomp_pressure_s32   : The value of uncompensated pressure.
 *  @param  v_uncomp_temperature_s32: The value of uncompensated temperature
 *  @param  v_uncomp_humidity_s32   : The value of uncompensated humidity.
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_uncomp_pressure_temperature_humidity(s32 *v_uncomp_pressure_s32,
                                                                             s32 *v_uncomp_temperature_s32, 
                                                                             s32 *v_uncomp_humidity_s32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* Array holding the MSB and LSb value of
	a_data_u8[0] - Pressure MSB
	a_data_u8[1] - Pressure LSB
	a_data_u8[1] - Pressure LSB
	a_data_u8[1] - Temperature MSB
	a_data_u8[1] - Temperature LSB
	a_data_u8[1] - Temperature LSB
	a_data_u8[1] - Humidity MSB
	a_data_u8[1] - Humidity LSB
	*/
	u8 a_data_u8[BME280_DATA_FRAME_SIZE] = {
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE,
                                        	BME280_INIT_VALUE, BME280_INIT_VALUE};
    com_rslt = bme280_bus_read_register(BME280_PRESSURE_MSB_REG,
                                        a_data_u8,
                                        BME280_ALL_DATA_FRAME_LENGTH);
	/*Pressure*/
	*v_uncomp_pressure_s32    = (s32)((((u32) (a_data_u8[BME280_DATA_FRAME_PRESSURE_MSB_BYTE])) << BME280_SHIFT_BIT_POSITION_BY_12_BITS) |
			                        (((u32)(a_data_u8[BME280_DATA_FRAME_PRESSURE_LSB_BYTE])) << BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
			                        ((u32)  a_data_u8[BME280_DATA_FRAME_PRESSURE_XLSB_BYTE]  >> BME280_SHIFT_BIT_POSITION_BY_04_BITS));

	/* Temperature */
	*v_uncomp_temperature_s32 = (s32)((((u32)(a_data_u8[BME280_DATA_FRAME_TEMPERATURE_MSB_BYTE])) << BME280_SHIFT_BIT_POSITION_BY_12_BITS) |
			                          (((u32)(a_data_u8[BME280_DATA_FRAME_TEMPERATURE_LSB_BYTE])) << BME280_SHIFT_BIT_POSITION_BY_04_BITS) | 
                                       ((u32) a_data_u8[BME280_DATA_FRAME_TEMPERATURE_XLSB_BYTE]  >> BME280_SHIFT_BIT_POSITION_BY_04_BITS));

	/*Humidity*/
	*v_uncomp_humidity_s32    = (s32)((((u32)(a_data_u8[BME280_DATA_FRAME_HUMIDITY_MSB_BYTE])) << BME280_SHIFT_BIT_POSITION_BY_08_BITS)    |
			                           ((u32)(a_data_u8[BME280_DATA_FRAME_HUMIDITY_LSB_BYTE])));
	return com_rslt;
}

static
BME280_RETURN_FUNCTION_TYPE bme280_init(void)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt             = ERROR;
	BME280_RETURN_FUNCTION_TYPE com1_rslt            = ERROR;
    u8                          v_data_u8            = BME280_INIT_VALUE;
	u8                          v_chip_id_read_count = BME280_CHIP_ID_READ_COUNT;
    
    //Clear BME280 Variable
    memset(&bme280_cluster, 0x00, sizeof(struct bme280_t));
    
    if(bme280_chip_id != 0x00){
        com_rslt = BME280_CHIP_ID_READ_SUCCESS;
        goto BME280_INIT;
    }
    //I2C Device Open 
    p_iic_bus_Start();

    while (v_chip_id_read_count > 0){
		/* read Chip Id */
		com_rslt = bme280_bus_read_register(BME280_CHIP_ID_REG, 
                                           &v_data_u8,
                                            BME280_GEN_READ_WRITE_DATA_LENGTH);
		/* Check for the correct chip id */
		if(v_data_u8 == BME280_CHIP_ID){
            break;
        }
		v_chip_id_read_count--;
		/* Delay added concerning the low speed of power up system to
		facilitate the proper reading of the chip ID */
        CyDelay(BME280_REGISTER_READ_DELAY);
	}
	/*assign chip ID to the global structure*/
    bme280_chip_id = v_data_u8;
	/*com_rslt status of chip ID read*/
	com_rslt = (v_chip_id_read_count == BME280_INIT_VALUE) ?
    BME280_CHIP_ID_READ_FAIL : BME280_CHIP_ID_READ_SUCCESS;

    
BME280_INIT:
	if (com_rslt == BME280_CHIP_ID_READ_SUCCESS) {
		/* readout bme280 calibparam structure */
        com1_rslt = bme280_get_calib_param();
		com_rslt += com1_rslt;
        
        //Set Ctrl Humidity Register = oversampling x 1
        v_data_u8 = 0x01;
        com1_rslt = bme280_bus_write(BME280_CTRL_HUMIDITY_REG, 
                                    &v_data_u8,
                                     BME280_GEN_READ_WRITE_DATA_LENGTH);
        com_rslt += com1_rslt;
        
	    /*
            Set Ctrl Measure Register = Pressure oversampling    x 8
                                        Temperature oversampling x 1
                                        Forced mode
        */
        v_data_u8 = 0x31;
        com1_rslt = bme280_bus_write(BME280_CTRL_MEAS_REG, 
                                    &v_data_u8,
                                     BME280_GEN_READ_WRITE_DATA_LENGTH);
        com_rslt += com1_rslt;
        
        //Set Configuration Register =  Filter coefficient = 2
        v_data_u8 = 0x08;
        com1_rslt = bme280_bus_write(BME280_CONFIG_REG, 
                                    &v_data_u8,
                                     BME280_GEN_READ_WRITE_DATA_LENGTH);
        com_rslt += com1_rslt;
    }
	return com_rslt;
}

BME280_RETURN_FUNCTION_TYPE bme280_read_pressure_temperature_humidity(u32 *v_pressure_u32, 
                                                                      s32 *v_temperature_s32, 
                                                                      u32 *v_humidity_u32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	s32 v_uncomp_pressure_s32            = BME280_INIT_VALUE;
	s32 v_uncom_temperature_s32          = BME280_INIT_VALUE;
	s32 v_uncom_humidity_s32             = BME280_INIT_VALUE;
    
    if(bme280_init() != SUCCESS){
        return(com_rslt);
    }
    
    /* read the uncompensated pressure, temperature and humidity*/
	com_rslt = bme280_read_uncomp_pressure_temperature_humidity(&v_uncomp_pressure_s32, 
                                                                &v_uncom_temperature_s32,
			                                                    &v_uncom_humidity_s32);
    /* read the true pressure, temperature and humidity*/
	*v_temperature_s32  = bme280_compensate_temperature_int32   (v_uncom_temperature_s32);
	*v_pressure_u32     = bme280_compensate_pressure_int32      (v_uncomp_pressure_s32  );
	*v_humidity_u32     = bme280_compensate_humidity_int32      (v_uncom_humidity_s32   );

    return com_rslt;
}
