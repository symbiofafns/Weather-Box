#include "log_trace.h"

#ifdef DEBUG

int _write(int file, char *ptr, int len)
{ 
    int i;
    file = file;
    for (i = 0; i < len; i++){ 
        //Macro_Debug_Message(*ptr++);
        p_debug_UartPutChar(*ptr++);
    }        
    return(len); 
}

void log_trace_initial(void)
{
    p_debug_Start();
}

#else

void log_trace_initial(void)
{
    return;
}
    
#endif
