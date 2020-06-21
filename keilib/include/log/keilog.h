#ifndef _KEILOG_H_
#define _KEILOG_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <linux/termios.h>

#ifdef __cplusplus
    extern "C"{
#endif

//"\033[0m"                 //关闭所有属性
//"\033[1m"                 //设置高亮度
//"\033[4m"                 //下划线
//"\033[5m"                 //闪烁
//"\033[7m"                 //反显
//"\033[8m"                 //消隐
//"\033[30m -- \033[37m"    //设置前景色
//"\033[40m -- \033[47m"    //设置背景色
//"\033[nA"                 //光标上移n行
//"\033[nB"                 //光标下移n行
//"\033[nC"                 //光标右移n列
//"\033[nD"                 //光标左移n列
//"\033[y;H"                //设置光标位置
//"\033[2J"                 //清屏
#define CLR_LINE "\033[K" //清除从光标到行尾的内容
//"\033[s"                  //保存光标位置
//"\033[u"                  //恢复光标位置
//"\033[?25l"               //隐藏光标
//"\033[?25h"               //显示光标

#define NONE "\e[0m"

#define BLACK "\e[1;30m"
#define RED "\e[31m"
#define GREEN "\e[32m"
#define YELLOW "\e[33m"
#define BLUE "\e[34m"
#define PURPLE "\e[35m"
#define CYAN "\e[36m"
#define GRAY "\e[37m"
#define WHITE "\e[1;37m"

#define B_RED "\e[41m"
#define B_GREEN "\e[42m"
#define B_YELLOW "\e[43m"
#define B_BLUE "\e[44m"
#define B_PURPLE "\e[45m"
#define B_CYAN "\e[46m"
#define B_GRAY "\e[47m"

#define SPECIFIER "~~"
typedef enum KLogPriority
{
    KLOG_UNKNOWN = 0,
    KLOG_DEFAULT, /* only for SetMinPriority() */
    KLOG_VERBOSE,
    KLOG_DEBUG,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR,
    KLOG_FATAL,
    KLOG_SILENT, /* only for SetMinPriority(); must be last */
} KLogPriority;

#define PRINT_DBG 0
#define KLOG_I(str, ...)                          \
    do                                            \
    {                                             \
        printf(GREEN "Info" NONE ": ");           \
        printf(CLR_LINE str"\n" NONE, ##__VA_ARGS__); \
    } while (0)
#define KLOG_E(str, arg...)                                                         \
    do                                                                              \
    {                                                                               \
        printf(RED "Error" NONE " in %s(%d)-<%s>: ", __FILE__, __LINE__, __func__); \
        printf(CLR_LINE str"\n" NONE, ##arg);                                           \
    } while (0)
#define KLOG_D(str, arg...)                                                                \
    do                                                                                     \
    {                                                                                      \
        if (PRINT_DBG == 1)                                                                \
        {                                                                                  \
            printf(PURPLE "Error" NONE " in %s(%d)-<%s>: ", __FILE__, __LINE__, __func__); \
            printf(CLR_LINE str"\n" NONE, ##arg);                                              \
        }                                                                                  \
    } while (0)

#define KCHECK_RETURN(exp, retval) \
    do                             \
    {                              \
        if (!(exp))                \
        {                          \
            return retval;         \
        }                          \
    } while (0)

#define kmin(x, y)                 \
    ({                            \
        const typeof(x) _x = (x); \
        const typeof(y) _y = (y); \
        (void)(&_x == &_y);       \
        _x < _y ? _x : _y;        \
    })

#define kmax(x, y)                 \
    ({                            \
        const typeof(x) _x = (x); \
        const typeof(y) _y = (y); \
        (void)(&_x == &_y);       \
        _x > _y ? _x : _y;        \
    })

int keilog_init(KLogPriority l, const char* p_logdir, const char *log_file_name, int flag);

int keilog(KLogPriority l, const char *fmt, ...);

int keilog_close();

int32_t get_win_width();

int32_t get_win_remain_width(int32_t offset);

/**
 * @brief print process bar in stdout
 * 
 * @param persent [0-100]
 * @param color GREEN for example
 * @return int32_t 
 */
int32_t print_proc_bar(int32_t persent, const char *color);

#ifdef __cplusplus
    }
#endif

#endif //_KEILOG_H_