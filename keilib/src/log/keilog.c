#include "log/keilog.h"
#include <sys/file.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#ifndef gettid
    #define gettid() syscall(SYS_gettid)
#endif
#define	  _LOG_PATH_LEN  250
#define   _LOG_BUFFSIZE  1024*1024*4
#define   _SYS_BUFFSIZE  1024*1024*8

KLogPriority m_system_level = KLOG_DEFAULT;
FILE* fp = NULL;
bool m_isappend = false;
bool m_issync = false;
char m_filelocation[_LOG_PATH_LEN];
__thread char m_buffer[_LOG_BUFFSIZE];
pthread_mutex_t m_mutex;

const char* logLevelToString(KLogPriority l) {
    switch (l)
    {
    case KLOG_UNKNOWN:
        return "UNKNOWN";
    case KLOG_DEFAULT:
        return "DEFAULT";
    case KLOG_VERBOSE:
        return "VERBOSE";
    case KLOG_DEBUG:
        return "DEBUG  ";
    case KLOG_INFO:
        return "INFO   ";
    case KLOG_WARN:
        return "WARN   ";
    case KLOG_ERROR:
        return "ERROR  ";
    case KLOG_FATAL:
        return "FATAL  ";
    case KLOG_SILENT:
        return "SILENT ";
    default:
        return "UNKNOWN";
    }
}

bool loginit(KLogPriority l, const  char *filelocation, bool append, bool issync)
{
	KCHECK_RETURN(NULL == fp, false);
    m_system_level = l;
    m_isappend = append; 
    m_issync = issync; 
	pthread_mutex_init(&m_mutex, NULL);
	
    if(strlen(filelocation) >= (sizeof(m_filelocation) -1))
	{
		fprintf(stderr, "the path of log file is too long:%lu limit:%lu\n", strlen(filelocation), sizeof(m_filelocation) -1);
        return false;
	}
	//本地存储filelocation  以防止在栈上的非法调用调用
	strncpy(m_filelocation, filelocation, sizeof(m_filelocation));
	m_filelocation[sizeof(m_filelocation) -1] = '\0';
	
	if('\0' == m_filelocation[0])
	{
		fp = stdout;
		fprintf(stderr, "now all the running-information are going to put to stderr\n");
		return true;
	}
	
	fp = fopen(m_filelocation, append ? "a":"w");
	if(fp == NULL)
	{
		fprintf(stderr, "cannot open log file,file location is %s\n", m_filelocation);
		// exit(0);
        return false;
	}
	//setvbuf (fp, io_cached_buf, _IOLBF, sizeof(io_cached_buf)); //buf set _IONBF  _IOLBF  _IOFBF
	setvbuf (fp,  (char *)NULL, _IOLBF, 0);
	fprintf(stderr, "now all the running-information are going to the file %s\n", m_filelocation);
	return true;
}

int keilog_init(KLogPriority l, const char* p_logdir, const char *log_file_name, int flag)
{
    //如果路径存在文件夹，则判断是否存在
	if (access (p_logdir, 0) == -1)
	{
		if (mkdir (p_logdir, S_IREAD | S_IWRITE ) < 0)
			fprintf(stderr, "create folder failed\n");
	}
	char _location_str[_LOG_PATH_LEN];
	snprintf(_location_str, _LOG_PATH_LEN, "%s/%s.log", p_logdir, log_file_name);	
	if(true != loginit(l, _location_str, false, true))
    {
        return -1;
    }
	return 0;
}

bool checklevel(KLogPriority l)
{
	if(l >= m_system_level)
		return true;
	else
		return false;
}

int premakestr(char* m_buffer, KLogPriority l)
{
    time_t now;
	now = time(&now);;
	struct tm vtm; 
    localtime_r(&now, &vtm);
    return snprintf(m_buffer, _LOG_BUFFSIZE, "%s %05d: %02d-%02d %02d:%02d:%02d ", logLevelToString(l), (uint)gettid(),
            vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec);
}

bool logclose()
{
	if(fp == NULL)
		return false;
	fflush(fp);
	fclose(fp);
	fp = NULL;
	return true;
}

bool _write(char *_pbuffer, int len)
{
	if(0 != access(m_filelocation, W_OK))
	{	
		pthread_mutex_lock(&m_mutex);
		//锁内校验 access 看是否在等待锁过程中被其他线程loginit了  避免多线程多次close 和init
		if(0 != access(m_filelocation, W_OK))
		{
			logclose();
			loginit(m_system_level, m_filelocation, m_isappend, m_issync);
		}
		pthread_mutex_unlock(&m_mutex);
	}

	if(1 == fwrite(_pbuffer, len, 1, fp)) //only write 1 item
	{
		if(m_issync)
          	fflush(fp);
		*_pbuffer='\0';
    }
    else 
	{
        int x = errno;
	    fprintf(stderr, "Failed to write to logfile. errno:%s    message:%s", strerror(x), _pbuffer);
	    return false;
	}
	return true;
}

int keilog(KLogPriority l, const char *fmt, ...)
{
    KCHECK_RETURN(checklevel(l), false);
	int _size = 0;
    int tmp_size = 0;
	// int prestrlen = 0;
	
	char * star = m_buffer;
	_size += premakestr(star, l);
	star += _size;
	
	va_list args;
	va_start(args, fmt);
	tmp_size = vsnprintf(star, _LOG_BUFFSIZE - _size, fmt, args);
    _size += tmp_size;
    star += tmp_size;
	va_end(args);
	tmp_size = snprintf(star, _LOG_BUFFSIZE - _size, "\n");
    _size += tmp_size;
	if(NULL == fp)
		fprintf(stderr, "%s", m_buffer);
	else
		// _write(m_buffer, prestrlen + _size);
		_write(m_buffer, _size);
	return true;
}

int keilog_close()
{
    if(true != logclose())
    {
        return -1;
    }
    return 0;
}