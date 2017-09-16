#if !defined(LOG_TRACE_H)    
    #define LOG_TRACE_H
    #include <project.h>
    #include <stdio.h> 
    
    void log_trace_initial(void);
    
#ifdef DEBUG
    #define DBG_PRINT_TEXT(a)        do\
                                        {\
                                            printf((a));\
                                        } while (0)

    #define DBG_PRINT_DEC(a)         do\
                                        {\
                                           printf("%02d ", a);\
                                        } while (0)


    #define DBG_PRINT_HEX(a)         do\
                                        {\
                                           printf("%08X ", a);\
                                        } while (0)


    #define DBG_PRINT_ARRAY(a,b)     do\
                                        {\
                                            uint32 i;\
                                            \
                                            for(i = 0u; i < (b); i++)\
                                            {\
                                                printf("%02X ", *(a+i));\
                                            }\
                                        } while (0)

    #define DBG_PRINTF(...)          (printf(__VA_ARGS__))

    
#else
    #define DBG_PRINT_TEXT(a)
    #define DBG_PRINT_DEC(a)
    #define DBG_PRINT_HEX(a)
    #define DBG_PRINT_ARRAY(a,b)
    #define DBG_PRINTF(...)
    #define DBG_NEW_PAGE             
#endif /* (DEBUG_UART_ENABLED == YES) */

#endif