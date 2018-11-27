#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#if defined(GLOBAL_DEBUG)

//默认只输出debug等级
static uint8_t default_level = LOG_LEVEL_DEBUG;


static void lowlevelPrefix_log(uint8_t level)
{
    switch (level){
    case LOG_LEVEL_ALERT:
        printf("[A]:");
        break;
    case LOG_LEVEL_ERROR:
        printf("[E]:");
        break;
    case LOG_LEVEL_WARN:
        printf("[W]:");
        break;
    case LOG_LEVEL_NOTICE:
        printf("[N]:");
        break;
    case LOG_LEVEL_INFO:
        printf("[I]:");
        break;
    case LOG_LEVEL_DEBUG:
        printf("[D]:");
        break;
    default:
        break;
    }
}

// 默认输出到stdout
void log_ll(uint8_t level,const char *format,...)
{
    va_list ap;

    if( level <= default_level ){ 
        lowlevelPrefix_log(level);
        va_start(ap,format);
        vprintf(format,ap);
        va_end(ap);
    }
    
}
void log_llln(uint8_t level,const char *format,...)
{
    va_list ap;

    if( level <= default_level ){ 
        lowlevelPrefix_log(level);
        va_start(ap,format);
        vprintf(format,ap);
        va_end(ap);
        printf("\r\n");
    }
    
}
void log_set_max_level(uint8_t level) 
{
    default_level = MIN(level, LOG_LEVEL_DEBUG);
}
void log_Init(void)
{
    lowlogInit();
}

//#ifdef __GNUC__
__near_func int putchar(int ch)
//#else
//int fputc(int ch, FILE *f)
//#endif
{
    /* e.g. write a character to the USART */
    lowlogputChar(ch);
        
    return ch;
}
#else
void log_ll(uint8_t level,const char *format,...)
{
    (void)level;
}
void log_llln(uint8_t level,const char *format,...)
{
    (void)level;
}

void log_set_max_level(uint8_t level) 
{
    (void)level;
}
void log_Init(void)
{

}

#endif
