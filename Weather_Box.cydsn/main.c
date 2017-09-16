/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>
#include "library\log_trace\log_trace.h"
#include "library\device\bme280\bme280.h"

int main()
{
    u32 pressure_u32;
    s32 temperature_s32;
    u32 humidity_u32;
    
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    /* CyGlobalIntEnable; */ /* Uncomment this line to enable global interrupts. */
    
    CyGlobalIntEnable;      /* Enable Global Interrupts*/
    
    log_trace_initial();
    p_lamp_Write(0x01);
    DBG_PRINT_TEXT("start\r\n");
    for(;;)
    {
        /* Place your application code here. */
        if(bme280_read_pressure_temperature_humidity(&pressure_u32,
                                                     &temperature_s32,
                                                     &humidity_u32               ) == SUCCESS){
                                                    
            DBG_PRINTF("p=%d\tt=%d\th=%d\r\n",pressure_u32,temperature_s32,humidity_u32);   
        }
        CyDelay(1000);
        p_lamp_Write(p_lamp_Read() ^ 0x01);
    }
}

/* [] END OF FILE */
