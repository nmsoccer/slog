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
  SLT_LOCAL = 1, //local log file
  SLT_NET, //remote network
}
SLOG_TYPE;

typedef enum
{
  SLOG_LEVEL_MIN = 0,
  SL_VERBOSE = SLOG_LEVEL_MIN,
  SL_DEBUG,
  SL_INFO,
  SL_ERR,
  SL_FATAL,
  SLOG_LEVEL_MAX = SL_FATAL,
}
SLOG_LEVEL;
//#define SLOG_LEVEL_MIN SL_VERBOSE
//#define SLOG_LEVEL_MAX SL_FATAL

typedef enum
{
  SLD_SEC = 0, //second
  SLD_MILL, //milli sec
  SLD_MIC, //micro sec
  SLD_NANO, //nano sec
}
SLOG_DEGREE;

typedef enum
{
  SLF_PREFIX = 0, //each log has prefix like [time level].
  SLF_RAW, //raw info
}
SLOG_FORMAT;


#define SLOG_LOG_NAME_LEN 256
typedef struct
{
  union
  {
    //SLT_LOCAL
    struct
    {
      char log_name[SLOG_LOG_NAME_LEN]; 
    }_local;

    //SLT_NETWORK
    struct
    {
      char ip[64];
      int port;
    }_net;
    
  }type_value;


  SLOG_DEGREE log_degree; //ref SLD_XX
  SLOG_FORMAT format; //ref SLF_xx
  int log_size; //size of single log file(only SLT_LOCAL)
  int rotate;  //rotate max num log file(only SLT_LOCAL)
}
SLOG_OPTION;


/************API*****************/
/***
Open A SLOG Descriptor
@type:SL_LOCAL:logs to local log files.
      SL_NETWORK:logs to remote udp server.
@filt_level:Log filter.Only Print Log if LOG_LEVEL >= filt_level.
@option:option value of setting.
 @_type_value:value of diff type
  @_local.log_name:if type is SL_LOCAL. refers to local log file name.
  @_network.ip&port:if type is SL_NETWORK. refers to remote server ip and port
 @format:format of log. default is SLF_PREFIX,if sets to SLF_RAW,then print raw info.
 @log_degree:refer SLOG_DEGREE.the timing degree of log. default by seconds.
 @log_size:max single log_file size.if 0 then sets to defaut 1M
 @rotate:log file rotate limit.if 0 then sets to default 10
@err:return err msg if failed.
*RETVALUE:
*-1: FAILED
*>=0:ALLOCATED SLD(SLOG Descriptor)
*/
extern int slog_open(SLOG_TYPE type , SLOG_LEVEL filt_level , SLOG_OPTION *option , char *err);


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
@format:Change format of single item. If No change sets to -1.
*/
extern int slog_chg_attr(int sld , int filt_level , int degree , int size , int rotate , int format);

/************API END*****************/

#endif