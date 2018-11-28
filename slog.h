/*
Simple Log Library
Created by soullei 2018-11-26
*/
#ifndef _S_LOG_H
#define _S_LOG_H
#include<stdio.h>
#include<stdlib.h>

typedef enum
{
  SL_VERBOSE = 0,
  SL_DEBUG,
  SL_INFO,
  SL_ERR,
  SL_FATAL,
}
SLOG_LEVEL;

#define SLOG_LEVEL_MIN SL_VERBOSE
#define SLOG_LEVEL_MAX SL_FATAL

typedef enum
{
  SLD_SEC = 0, //second
  SLD_MILL, //milli sec
  SLD_MAC, //macro sec
  SLD_NANO, //nano sec
}
SLOG_DEGREE;

/************API*****************/
/***
Open A SLOG Descriptor
@log_name:log file name.(<256) if NULL logs print to stdout.
@filt_level:Log filter.Only Print Log if LOG_LEVEL >= filt_level.
@log_degree:refer SLOG_DEGREE.the timing degree of log. default by seconds.
@log_size:max single log_file size.if 0 then sets to defaut 1M
@rotate:log file rotate limit.if 0 then sets to default 10
@err:return err msg if failed.
*RETVALUE:
*-1: FAILED
*>=0:ALLOCATED SLD(SLOG Descriptor)
*/
extern int slog_open(char *log_name , SLOG_LEVEL filt_level , SLOG_DEGREE log_degree , int log_size , 
  int rotate , char *err);

/***
Close A SLOG Descriptor
*RETVALUE:
*-1: FAILED
*=0: SUCCESS
*/
extern int slog_close(int sld);

/***
Log
@sld:opened slog descriptor
@log_level:refer to SL_XX.the level of this log.If log_level < filt_level(slog_open),it will not printed.
@RETVALUE:
-1:failed(err msg can be found in slog.log). 0:success
*/
extern int slog_log(int sld , SLOG_LEVEL log_level , char *fmt , ...);

/***
Change Attr
@sld:opened slog descriptor
@filt_level:refer to SLOG_LEVEL. If No Change Sets to -1.
@degree:refer to SLOG_DEGREE. If No Change sets to -1.
@size:Change log size. If No Change sets to -1.
@rotate:Change Max Rotate Number. If No Change sets to -1.
*/
extern int slog_chg_attr(int sld , int filt_level , int degree , int size , int rotate);

/************API*****************/
#endif