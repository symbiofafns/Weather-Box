#if !defined(RTC_H)
    #define RTC_H
    
    #include <project.h>
    
    void    rtc_initial            (void);
    uint32  rtc_get_current_tick   (void);
    
#endif    