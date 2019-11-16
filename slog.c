#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "slog.h"

#define SLOG_MAX_LINE_LEN (1024*2)
#define SLOG_LOG_LINE_BUFF (SLOG_MAX_LINE_LEN)

#define SLOG_SELF_FILE "./slog.log"
#define SELF_FILE_SIZE (20*1024*1024) //self log size 20M
#define SELF_FILE_ROTATE_MAX 5 //rotate 5 file(s)

typedef struct
{
  short total;
  short count;
  short head;
  short tail;
  char rb[0]; //total*SLOG_MAX_LINE_LEN
}
NET_LOG_RINGBUFF;
#define MAX_RB_ITEM 0 //LOCK it TO 0.

typedef struct
{
  char stat; //0:empty 1:valid
  char type; //type of log
  int sld;
  union
  {
    struct
    {
      char log_name[SLOG_LOG_NAME_LEN];
    }_local;

    struct
    {
      char ip[64];
      int port;
      int sock_fd;
      struct sockaddr_in remote_addr;
      NET_LOG_RINGBUFF *prb_info;
    }_net;
  }type_info;
  int port;
  FILE *fp;
  int filt;
  int degree;
  int size;
  int log_seq;
  int max_seq;
  int write_bytes;
  char log_stated; //if stated log
  char format;
  char *log_buff; //SLOG_LOG_LINE_BUFF
}
SLOG_NODE;


#define SLOG_NODE_STAT_NONE  0
#define SLOG_NODE_STAT_VALID 1

typedef struct
{
  int valid_count; //valid node count
  int list_len; //list len == (log2(real_len))
  SLOG_NODE *node_list;
  int self_fd; //slog self fd
  int self_seq;
  int self_writes; //write bytes
  int self_stated; //if stated self log file
}
SLOG_ENV;

static SLOG_ENV slog_env = {0 , -1 , NULL , -1 , 0 , 0 , 0};
char *_LOG_LEVEL_LABEL[] = { //SLOG_L_XX
"debug", //SLOG_L_VERBOSE
"debug", //SLOG_L_DEBUG
"info",  //SLOG_L_INFO
"error",   //SLOG_L_ERR
"fatal", //SLOG_L_FATAL
};

/************INNER FUNC DEC*****************/
static int _print_node_list(SLOG_ENV *penv , int len);
static int _write_self_msg(SLOG_ENV *penv , char *fmt , ...);
static int _rotate_log_file(SLOG_ENV *penv , char *log_name , int curr_seq , int max_seq);
static int _slog_log(SLOG_ENV *penv , SLOG_NODE *pnode , SLOG_LEVEL level , SLOG_DEGREE log_degree , 
  SLOG_FORMAT format , char *fmt , va_list arg_ap);
static int _net_open(SLOG_ENV *penv , SLOG_NODE *pnode);
static void _net_log(SLOG_ENV *penv , SLOG_NODE *pnode , char *buff , int len);
static int _net_refresh_log(SLOG_ENV *penv , SLOG_NODE *pnode);
/************INNER FUNC DEC*****************/

extern int errno;
/************API FUNC DEFINE*****************/
/***
Open A SLOG Descriptor
@type:SL_LOCAL:logs to local log files.
      SL_NETWORK:logs to remote udp server.
@filt_level:Log filter.Only Print Log if LOG_LEVEL >= filt_level.
@option:option value of setting.
 @_type_value:value of diff type
  @_local.log_name:if type is SL_LOCAL. refers to local log file name.
  @_network.ip&port:if type is SL_NETWORK. refers to remote server ip and port
 @format:format of log. if 0 then default is SLF_PREFIX,if sets to SLF_RAW,then print raw info.
 @log_degree:refer SLOG_DEGREE.the timing degree of log. if 0 then default by seconds.
 @log_size:max single log_file size.if 0 then sets to defaut 20M
 @rotate:log file rotate limit.if 0 then sets to default 5
@err:return err msg if failed.
*RETVALUE:
*-1: FAILED
*>=0:ALLOCATED SLD(SLOG Descriptor)
*/
int slog_open(SLOG_TYPE type , SLOG_LEVEL filt_level , SLOG_OPTION *option , char *err)
{
  SLOG_NODE *pnode = NULL; 
  SLOG_ENV *penv = &slog_env;
  int i = 0;
  int real_len = 0;
  int new_len = 0;

  int sld = -1;
  char file_name[SLOG_LOG_NAME_LEN] = {0};
  char log_name[SLOG_LOG_NAME_LEN] = {0};
  char err_msg[1024] = {0};

  SLOG_OPTION *popt = option;
  int log_size = 0;
  int rotate = 0;
  int format = 0;
  int log_degree = 0;
  int ret = -1;
  
  
  /***Arg Check*/
  if(type!=SLT_LOCAL && type!=SLT_NET)
  {
    if(err)
      strcpy(err , "failed! type is illegal!");
    _write_self_msg(penv, "%s failed! type is illegal! type:%d", __FUNCTION__ , type);
    return -1;
  }
  
      
  if(filt_level<SLOG_LEVEL_MIN || filt_level>SLOG_LEVEL_MAX)
  {
    if(err)
      strcpy(err , "failed! fil_level is illegal!");
    _write_self_msg(penv, "%s failed! filt_level illegal! filt_level:%d", __FUNCTION__ , filt_level);
    return -1;
  }

  if(!popt)
  {
    if(err)
      strcpy(err , "failed! option is null!");
    _write_self_msg(penv, "%s failed! option is NULL!", __FUNCTION__);
    return -1;
  }

    //check local and info
  if(type==SLT_LOCAL )
  {
    if(strlen(popt->type_value._local.log_name)<=0)
    {
      if(err)
        strcpy(err , "failed! type is local but log_name null!");
      _write_self_msg(penv, "%s failed! type is local but log_name null!", __FUNCTION__);
      return -1;
    }
    else
    {
      strncpy(log_name , popt->type_value._local.log_name , sizeof(log_name));
    }
  }

    //check net and info
  if(type==SLT_NET && (strlen(popt->type_value._net.ip)<=0 || popt->type_value._net.port<=0))
  {
    if(err)
      strcpy(err , "failed! type is network but remote addr is null");
    _write_self_msg(penv, "%s failed! type is network but remote addr is null", __FUNCTION__);
    return -1;
  }

  /***Init*/
  log_size = popt->log_size;
  log_degree = popt->log_degree;
  rotate = popt->rotate;
  format = popt->format;


  /***Check Opt Value*/
  if(popt->log_degree<SLD_SEC || popt->log_degree>SLD_NANO)
  {
    log_degree = SLD_SEC;
    _write_self_msg(penv, "%s warn! log_degree illegal, and sets to default %d!", __FUNCTION__ , log_degree); 
  }
  
  if(popt->log_size <= 0)
  {
    log_size = SELF_FILE_SIZE;
    _write_self_msg(penv, "%s warn! log_size zero. then sets to default %d!", __FUNCTION__ , log_size);    
  }

  if(popt->rotate <= 0)
  {
    rotate = SELF_FILE_ROTATE_MAX;
    _write_self_msg(penv, "%s warn! rotate zero. then sets to default %d!", __FUNCTION__ , rotate);    
  }

  if(popt->format!=SLF_PREFIX && popt->format!=SLF_RAW)
  {
    format = SLF_PREFIX;
    _write_self_msg(penv, "%s warn! ori_format:%d sets to default %d!", __FUNCTION__ , popt->format , format);
  }

  
  /***Empty List*/
  if(!penv->node_list || penv->list_len<0)
  {
    _write_self_msg(penv, "Open in an empty List");
    //alloc mem
    pnode = (SLOG_NODE *)calloc(1 , sizeof(SLOG_NODE));
    if(!pnode)
    {
      strncpy(err_msg , strerror(errno) , sizeof(err_msg));
      if(err)
        strcpy(err , err_msg);
      
      fprintf(stderr , "%s\n" , err_msg);
      _write_self_msg(penv , err_msg);
      return -1;
    }

    //basic env info
    penv->list_len = 0;//log2(1)

    //fill node info
      //ling buff
    pnode->log_buff = (char *)calloc(1 , SLOG_LOG_LINE_BUFF);
    if(!pnode->log_buff)
    {
      strncpy(err_msg , strerror(errno),sizeof(err_msg));
      if(err)
        strcpy(err , err_msg);
      
      _write_self_msg(penv , "<%s> alloc 1 log buff failed! reason:%s" , __FUNCTION__ , err_msg);        
      return -1;  
    }
    
      //type_local
    if(type == SLT_LOCAL)
    {
      strncpy(pnode->type_info._local.log_name, log_name , SLOG_LOG_NAME_LEN);
      snprintf(file_name , sizeof(file_name) , "%s.%d" , log_name , pnode->log_seq);
      pnode->fp = fopen(file_name , "a+");
      if(!pnode->fp)
      {
        strncpy(err_msg , strerror(errno),sizeof(err_msg));
        if(err)
          strcpy(err , err_msg);
      
        _write_self_msg(penv , err_msg);
        free(pnode->log_buff);
        return -1;
      }
      else //set ling buf
      {
    	  if(setvbuf(pnode->fp , NULL , _IOLBF , 0) < 0)
    	  {
    		  _write_self_msg(penv , "%s change FILE* to line-buff failed. err:%s" , __FUNCTION__ , strerror(errno));
    	  }
      }
    }
    else  //type_network
    {
      strncpy(pnode->type_info._net.ip, popt->type_value._net.ip , sizeof(pnode->type_info._net.ip));
      pnode->type_info._net.port = popt->type_value._net.port;      

      ret = _net_open(penv , pnode);
      if(ret != 0)
      {
        free(pnode->log_buff);
        return -1;
      }
    }
    

      //other info
    pnode->stat = SLOG_NODE_STAT_VALID;
    pnode->type = type;
    pnode->sld = 0;
    pnode->filt = filt_level;
    pnode->degree = log_degree;
    pnode->size = log_size;
    pnode->max_seq = rotate;
    pnode->format = format;

    //final fill env info
    penv->valid_count = 1;
    penv->node_list = pnode;
    sld = 0;
    //log    
    _write_self_msg(penv, "%s open success! sld:%d" , __FUNCTION__ , sld);
    //print
    _print_node_list(penv , 1);
    return sld;
  }
  
  /***List*/
  real_len = (int)pow(2 , penv->list_len);
  
  //LOCAL TYPE
  if(type == SLT_LOCAL)
  {
    //Check Same LogName
    for(i=0; i<real_len; i++)
    {
      pnode = &penv->node_list[i];
      if(pnode->stat == SLOG_NODE_STAT_VALID)
      {
        if(strcmp(log_name , pnode->type_info._local.log_name) == 0)
        {
          if(err)
            strcpy(err , "SLOG DESCRIPTER of this Log is existed!");
          _write_self_msg(penv, "<%s>:Open Duplicated!sld:%d" ,__FUNCTION__ , i);
          return -1;
        }
      }
    }
  }
  else //NET TYPE 
  {
    //Check Same IP:PORT
    for(i=0; i<real_len; i++)
    {
      pnode = &penv->node_list[i];
      if(pnode->stat == SLOG_NODE_STAT_NONE)
        continue;

      if(pnode->type_info._net.port==popt->type_value._net.port &&
         strcmp(pnode->type_info._net.ip , popt->type_value._net.ip)==0)
      {
        if(err)
          strcpy(err , "SLOG DESCRIPTER of IP:PORT is existed!");
        _write_self_msg(penv, "<%s>:Open Duplicated!sld:%d" ,__FUNCTION__ , i);
        return -1;
      }
    }
  }

  
  //Choose from Existed List
  if(penv->valid_count < real_len)
  {
    _write_self_msg(penv, "Open in a non-empty and non-full List valid_count:%d real_len:%d list:%d" , 
      penv->valid_count , real_len , penv->list_len);
    for(i=0; i<real_len; i++)
    {
      if(penv->node_list[i].stat == 0)
        break;
    }
    if(i<real_len) //found
    {
      pnode = &penv->node_list[i];
      memset(pnode , 0 , sizeof(SLOG_NODE));

      //line buff
      pnode->log_buff = (char *)calloc(1 , SLOG_LOG_LINE_BUFF);
      if(!pnode->log_buff)
      {
        strncpy(err_msg , strerror(errno),sizeof(err_msg));
        if(err)
          strcpy(err , err_msg);
        
        _write_self_msg(penv , "<%s> alloc 2 log buff failed! reason:%s" , __FUNCTION__ , err_msg);        
        return -1;  
      }
    
      //type local
      if(type == SLT_LOCAL)
      {
        strncpy(pnode->type_info._local.log_name , log_name , SLOG_LOG_NAME_LEN);
        snprintf(file_name , sizeof(file_name) , "%s.%d" , log_name , pnode->log_seq);
        pnode->fp = fopen(file_name , "a+");
        if(!pnode->fp)
        {
          if(err)
            strcpy(err , strerror(errno));
      
          _write_self_msg(penv , "%s" , strerror(errno));
          free(pnode->log_buff);
          return -1;
        }
        else //set line buf
        {
        	if(setvbuf(pnode->fp , NULL , _IOLBF , 0) < 0)
            {
        		_write_self_msg(penv , "%s change FILE* to line-buff failed. err:%s" , __FUNCTION__ , strerror(errno));
            }
        }
      }
      else  //type_network
      {
        strncpy(pnode->type_info._net.ip , popt->type_value._net.ip , sizeof(pnode->type_info._net.ip));
        pnode->type_info._net.port = popt->type_value._net.port;
        

        ret = _net_open(penv , pnode);
        if(ret != 0)
        {
          free(pnode->log_buff);
          return -1;
        }
      }

        //other info
      pnode->stat = SLOG_NODE_STAT_VALID;
      pnode->type = type;
      pnode->filt = filt_level;
      pnode->degree = log_degree;
      pnode->size = log_size;
      pnode->max_seq = rotate;
      pnode->format = format;
      pnode->sld = i;
      
        //final fill env info
      penv->valid_count++;
      sld = i;
        //log
      _write_self_msg(penv, "%s open success! sld:%d" , __FUNCTION__ , sld);
        //print
      _print_node_list(penv , real_len);
      return sld;     
    }
    else //report error
    {      
      _write_self_msg(penv, "An Error Happened When SLOG_OPEN! valid_count<len but no empty found!" 
        "valid:%d len:%d real_len:%d" , penv->valid_count , penv->list_len , real_len);
    }
  }

  //Allocate a New List  
  new_len = (int)pow(2 , penv->list_len + 1);
  if(new_len <= real_len)
  {
    snprintf(err_msg , sizeof(err_msg) , "Create New List Failed! new_len -lt old_len! new:%d old:%d list_len:%d" , 
      new_len , real_len , penv->list_len);
    return -1;
  }

  _write_self_msg(penv, "Try to Alloc a new List!old_len:%d new_len:%d ori:%d" , real_len , new_len , 
    penv->list_len);
    //alloc mem
  pnode = (SLOG_NODE *)calloc(new_len , sizeof(SLOG_NODE));
  if(!pnode)
  {
    strncpy(err_msg , strerror(errno) , sizeof(err_msg));
    if(err)
      strcpy(err , err_msg);
      
    fprintf(stderr , "%s" , err_msg);
    _write_self_msg(penv , err_msg);
    return -1;
  }

    //copy old list
  memcpy(pnode , penv->node_list , sizeof(SLOG_NODE)*real_len);

    //destroy old
  free(penv->node_list);

    //attach
  penv->node_list = pnode;
  penv->list_len++;

    //print new
  //_print_node_list(penv , real_len);

    //Empty Node
  //sld = real_len;
  //pnode = penv->node_list[sld]
  //for safe search again
  real_len = new_len;
  if(penv->valid_count < real_len)
  {
    for(i=0; i<real_len; i++)
    {
      if(penv->node_list[i].stat == 0)
        break;
    }
    if(i<real_len) //found
    {
      pnode = &penv->node_list[i];
      memset(pnode , 0 , sizeof(SLOG_NODE));
      //ling buff
      pnode->log_buff = (char *)calloc(1 , SLOG_LOG_LINE_BUFF);
      if(!pnode->log_buff)
      {
        strncpy(err_msg , strerror(errno),sizeof(err_msg));
        if(err)
          strcpy(err , err_msg);
        
        _write_self_msg(penv , "<%s> alloc 3 log buff failed! reason:%s" , __FUNCTION__ , err_msg);        
        return -1;  
      }
    
      //type local
      if(strlen(log_name)>0)
      {
        strncpy(pnode->type_info._local.log_name , log_name , SLOG_LOG_NAME_LEN);
        snprintf(file_name , sizeof(file_name) , "%s.%d" , log_name , pnode->log_seq);
        pnode->fp = fopen(file_name , "a+");
        if(!pnode->fp)
        {
          if(err)
            strcpy(err , strerror(errno));

          free(pnode->log_buff);
          _write_self_msg(penv , strerror(errno));        
          return -1;
        }
        else //set line buf
        {
        	if(setvbuf(pnode->fp , NULL , _IOLBF , 0) < 0)
        	{
        		_write_self_msg(penv , "%s change FILE* to line-buff failed. err:%s" , __FUNCTION__ , strerror(errno));
        	}
        }

      }
      else  //type network
      {
        strncpy(pnode->type_info._net.ip , popt->type_value._net.ip , sizeof(pnode->type_info._net.ip));
        pnode->type_info._net.port = popt->type_value._net.port;


        ret = _net_open(penv , pnode);
        if(ret != 0)
        {
          free(pnode->log_buff);
          return -1;             
        }
      }

        //other info
      pnode->stat = SLOG_NODE_STAT_VALID;
      pnode->type = type;
      pnode->filt = filt_level;
      pnode->degree = log_degree;
      pnode->size = log_size;
      pnode->max_seq = rotate;
      pnode->format = format;
      pnode->sld = i; 
      
        //final fill env info
      penv->valid_count++;
      sld = i;
        //log
      _write_self_msg(penv, "%s open success! sld:%d" , __FUNCTION__ , sld);
      _print_node_list(penv , real_len);
      return sld;     
    }
    else //report error
    {
      snprintf(err_msg , sizeof(err_msg) , "An Error Happened When SLOG_OPEN,After Relloc MEM! valid_count<len" 
       "but no empty found! valid:%d len:%d real_len:%d" , penv->valid_count , penv->list_len , real_len);
      if(err)
        strcpy(err , err_msg);
      
      _write_self_msg(penv, err_msg);
      return -1;
    }
  }
    
  return -1;
}

/***
Close A SLOG Descriptor
*RETVALUE:
*-1: FAILED
*=0: SUCCESS
*/
int slog_close(int sld)
{
  SLOG_NODE *pnode = NULL;
  SLOG_ENV *penv = &slog_env;
  int real_len = 0;
  int i = 0;

  /***Arg Check*/
  if(sld < 0)
    return -1;
    
  /***Empty List*/
  if(!penv->node_list || penv->list_len<0)
  {
    _write_self_msg(penv, "%s Failed! Try Close %d in Empty List", __FUNCTION__ , sld);
    return -1;
  }
  
  /***Len List*/
  real_len = (int)pow(2 , penv->list_len);
  if(sld >= real_len)
  {
    _write_self_msg(penv, "%s Failed!lsd overmit! sld:%d real_len:%d", __FUNCTION__ , sld , real_len);
    return -1;
  }

  do
  {
    /***first Close by sld Directly*/
    pnode = &penv->node_list[sld];
    if(pnode->sld == sld)
    {
      break;
    }
  
    /***sld not match*/
    _write_self_msg(penv, "%s Direct Close Not Match!There may be something wrong!sld:%d", __FUNCTION__ , sld);
    _print_node_list(penv, real_len);
  
    for(i=0; i<real_len; i++)
    {
      pnode = &penv->node_list[i];
      if(pnode->sld == sld)
        break;
    }
    if(i >= real_len)
    {
      _write_self_msg(penv, "%s Failed! Not Proper sld Found! sld:%d", __FUNCTION__ , sld);
      return -1;
    }
    break;
  }
  while(0);

  //found pnode
    //recyle memory
  _write_self_msg(penv, "%s Close Sucess! sld:%d", __FUNCTION__ , sld);
  if(pnode->type==SLT_LOCAL)
  {
    if(pnode->fp)
      fclose(pnode->fp);
  }
  else
  {
    if(pnode->type_info._net.sock_fd >= 0)
      close(pnode->type_info._net.sock_fd);

    if(pnode->type_info._net.prb_info)
      free(pnode->type_info._net.prb_info);
  }

  if(pnode->log_buff)
    free(pnode->log_buff);

  
    //reset node
  memset(pnode , 0 , sizeof(SLOG_NODE));
  penv->valid_count--;

  _print_node_list(penv, real_len);
  //If Node List Empty
  if(penv->valid_count <= 0)
  {
    _write_self_msg(penv, "%s Will Destroy Node List!", __FUNCTION__);
    //_print_node_list(penv, real_len);
    
    penv->list_len = -1;
    penv->valid_count = 0;
    free(penv->node_list);
    penv->node_list = NULL;
  }
  return 0;
}

/***
Print A Log
@sld:opened slog descriptor
@log_level:refer to SLOG_L_XX.the level of this log.If log_level < filt_level(slog_open),it will not printed.
@RETVALUE:
-1:failed(err msg can be found in slog.log). 0:success
*/
int slog_log(int sld , SLOG_LEVEL log_level , char *fmt , ...)
{
  SLOG_ENV *penv = &slog_env;
  SLOG_NODE *pnode = NULL;
  char log_name[SLOG_LOG_NAME_LEN] = {0};
  char file_name[SLOG_LOG_NAME_LEN] = {0};
  int real_len = 0;
  int ret = -1;

  struct stat stat_info;
  va_list ap;
  
  
  /***Basic Arg Check*/
  if(sld<0)
  {
    _write_self_msg(penv, "%s failed! sld illegal! sld:%d", __FUNCTION__ , sld);
    return -1;
  }

  if(!penv->node_list || penv->list_len<0)
  {
    _write_self_msg(penv, "%s failed! log node list empty!", __FUNCTION__);
    return -1;
  }

  real_len = (int)pow(2 , penv->list_len);
  if(sld >= real_len)
  {
    _write_self_msg(penv, "%s failed! sld overmit! sld:%d real_len:%d", __FUNCTION__ , sld , real_len);
    return -1;
  }
  
  /***Search pnode*/
  pnode = &penv->node_list[sld];
  if(pnode->stat != SLOG_NODE_STAT_VALID || pnode->sld != sld)
  {
    _write_self_msg(penv, "%s failed! sld not opened or sld not match!node stat:%d node sld:%d sld:%d", 
      __FUNCTION__ , pnode->stat , pnode->sld , sld);
    return -1;  
  }

  if(pnode->type==SLT_LOCAL && pnode->type_info._local.log_name)
    strncpy(log_name , pnode->type_info._local.log_name , sizeof(log_name));


  /***Other Arg Check*/
  if(log_level<SLOG_LEVEL_MIN || log_level>SLOG_LEVEL_MAX)
  {
    _write_self_msg(penv, "%s failed! log:%s level illegal! level:%d",__FUNCTION__ , log_name , log_level);
    return -1;
  }

  /***FILT LOG*/
  if(log_level < pnode->filt)
  {
    return 0;
  }

  /***Open File*/
  if(pnode->type==SLT_LOCAL && !pnode->fp)
  {    
    snprintf(file_name , sizeof(file_name) , "%s.%d" , pnode->type_info._local.log_name , pnode->log_seq);
    _write_self_msg(penv, "%s. reopen %s again!", __FUNCTION__ , file_name);
    
    pnode->fp = fopen(file_name , "a+");
    if(!pnode->fp)
    {
      _write_self_msg(penv , "%s. open %s failed! err:%s" , __FUNCTION__ , file_name , strerror(errno));        
      return -1;
    }
    else //set line buf
    {
    	if(setvbuf(pnode->fp , NULL , _IOLBF , 0) < 0)
        {
    		_write_self_msg(penv , "%s change FILE* to line-buff failed. err:%s" , __FUNCTION__ , strerror(errno));
        }
    }
  }

  /***Stat File*/
  if(pnode->type==SLT_LOCAL && pnode->log_stated == 0)
  {
    snprintf(file_name , sizeof(file_name) , "%s.%d" , pnode->type_info._local.log_name , pnode->log_seq);
    ret = stat(file_name, &stat_info);
    do
    {
      if(ret < 0)
      {
        _write_self_msg(penv , "%s. stat %s failed! err:%s" , __FUNCTION__ , file_name , strerror(errno));
        break;
      }

      pnode->log_stated = 1;
      pnode->write_bytes = stat_info.st_size;
      _write_self_msg(penv, "%s. stat %s success! size:%d", __FUNCTION__ , file_name , pnode->write_bytes);
    }
    while(0);
  }
  
  /***Record Log*/
  va_start(ap , fmt);
  ret = _slog_log(penv , pnode , log_level , pnode->degree , pnode->format , fmt , ap);
  va_end(ap);

  /***Rotate*/
  pnode->write_bytes += ret;
  if(pnode->type==SLT_LOCAL && pnode->write_bytes >= pnode->size)
  {
    //printf("try to rotate %s and %d:%d\n" , file_name , pnode->write_bytes , pnode->size);
    fflush(pnode->fp);
    ret = _rotate_log_file(penv, pnode->type_info._local.log_name , pnode->log_seq , pnode->max_seq);
    if(ret == 0)
    {      
      fclose(pnode->fp);
      pnode->fp = NULL;
      pnode->write_bytes = 0;
      pnode->log_stated = 0;

      //open a new file
      snprintf(file_name , sizeof(file_name) , "%s.%d" , pnode->type_info._local.log_name , pnode->log_seq);
      _write_self_msg(penv, "%s. rotate and reopen %s again!", __FUNCTION__ , file_name);
      
      pnode->fp = fopen(file_name , "a+");
      if(!pnode->fp)
        _write_self_msg(penv , "%s. rotate and open %s failed! err:%s" , __FUNCTION__ , file_name , strerror(errno));
      else //set line buf
      {
    	  if(setvbuf(pnode->fp , NULL , _IOLBF , 0) < 0)
          {
          		_write_self_msg(penv , "%s change roate FILE* to line-buff failed. err:%s" , __FUNCTION__ , strerror(errno));
          }
      }

    }
  }
  return 0;
}

/***
Change Attr
@sld:opened slog descriptor
@filt_level:refer to SLOG_LEVEL. If No Change Sets to -1.
@degree:refer to SLOG_DEGREE. If No Change sets to -1.
@size:Change log size. If No Change sets to -1.
@rotate:Change Max Rotate Number. If No Change sets to -1.
@format:Change format of single item. If No change sets to -1.
@RET:
-1:failed(check at slog.log). 0:success
*/
int slog_chg_attr(int sld , int filt_level , int degree , int size , int rotate , int format)
{
  SLOG_NODE *pnode = NULL;
  SLOG_ENV *penv = &slog_env;
  int real_len;
  
  /***Check*/
  if(!penv->node_list || penv->list_len<0)
  {
    _write_self_msg(penv, "%s failed! No Opened SLD!", __FUNCTION__);
    return -1;
  }

  real_len = (int)pow(2 , penv->list_len);
  /***Get PNODE*/
  if(sld >= real_len)
  {
    _write_self_msg(penv, "%s failed! sld overmit! sld:%d real_len:%d list_len:%d", __FUNCTION__ , 
      sld , real_len , penv->list_len);
    return -1;
  }

  pnode = &penv->node_list[sld];
  if(pnode->stat!=SLOG_NODE_STAT_VALID || pnode->sld!=sld)
  {
    _write_self_msg(penv, "%s failed! stat or sld not match! stat:%d node_sld:%d sld:%d", __FUNCTION__ , 
      pnode->stat , pnode->sld , sld);
    return -1;
  }

  /***Check arg*/
  if(filt_level > SLOG_LEVEL_MAX)
  {
    _write_self_msg(penv, "%s failed! filt_level illegal! filt_level:%d", __FUNCTION__ , filt_level);
    return -1;
  }

  if(degree > SLD_NANO)
  {
    _write_self_msg(penv, "%s failed! degree illegal! degree:%d", __FUNCTION__ , degree);
    return -1;
  }

  if(size == 0)
  {
    _write_self_msg(penv, "%s failed! size zero! size:%d", __FUNCTION__ , size);
    return -1;  
  }

  if(rotate == 0)
  {
    _write_self_msg(penv, "%s failed! rotate zero! rotate:%d", __FUNCTION__ , rotate);
    return -1;  
  }

  if(format > SLF_RAW)
  {
    _write_self_msg(penv, "%s failed! format illegal! format:%d", __FUNCTION__ , format);
    return -1;
  }

  _write_self_msg(penv , "%s.filt:%d degree:%d size:%d rotate:%d format:%d" , __FUNCTION__ , filt_level , 
    degree , size , rotate , format);
  /***Start Chg*/
  _write_self_msg(penv, "%s. >>>Before Change:filt:%d degree:%d size:%d rotate:%d format:%d", __FUNCTION__ , 
    pnode->filt , pnode->degree , pnode->size , pnode->max_seq , pnode->format);
  
  if(filt_level >= 0)
    pnode->filt = filt_level;

  if(degree >= 0)
    pnode->degree = degree;

  if(size > 0)
    pnode->size = size;

  if(rotate > 0)
    pnode->max_seq = rotate;

  if(format >= 0)
    pnode->format = format;

  _write_self_msg(penv, "%s. <<<After Change:filt:%d degree:%d size:%d rotate:%d format:%d", __FUNCTION__ , 
    pnode->filt , pnode->degree , pnode->size , pnode->max_seq , pnode->format);

  /***flush*/

  return 0;
}

/************INNER FUNC DEFINE*****************/
static int _print_node_list(SLOG_ENV *penv , int len)
{
  SLOG_NODE *pnode = NULL;
  int i = 0;
  /***Arg Check*/
  if(!penv)
    return -1;

  /***Print*/
  _write_self_msg(penv, "==========PRINT===========");
  for(i=0; i<len; i++)
  {
    pnode = &penv->node_list[i];
    //_write_self_msg(penv, "-----");
    if(pnode->type == SLT_LOCAL)
    {
      _write_self_msg(penv, "STAT:%d TYPE:%d ID:%d LOG:%s FILT:%d DEGREE:%d SIZE:%d SEQ:%d MAX_SEQ:%d FORMAT:%d ", pnode->stat , 
        pnode->type , pnode->sld , pnode->type_info._local.log_name , pnode->filt , pnode->degree , pnode->size , pnode->log_seq , 
        pnode->max_seq , pnode->format);
    }
    else
    {
      _write_self_msg(penv, "STAT:%d TYPE:%d ID:%d FILT:%d DEGREE:%d SIZE:%d SEQ:%d MAX_SEQ:%d FORMAT:%d "
        "IP:%s PORT:%d " , pnode->stat , pnode->type , pnode->sld, pnode->filt , pnode->degree , pnode->size , pnode->log_seq , 
        pnode->max_seq , pnode->format , pnode->type_info._net.ip , pnode->type_info._net.port);
    }
   
  }
  _write_self_msg(penv, "==========END===========");

  return 0;    
}

//rotate log file
//sequently mv from max_seq to curr_seq
//return: 0 success 1 failed
//Never Use _write_self_msg in this Function._rotate-->_write-->_rotate-->...
static int _rotate_log_file(SLOG_ENV *penv , char *log_name , int curr_seq , int max_seq)
{
  char old_file_name[2048] = {0};
  char new_file_name[2048] = {0};
  int seq = max_seq - 1;
  int ret = 0;
  char buff[1024] = {0};

  /***Arg Check*/
  if(!log_name)
  {
    snprintf(buff , sizeof(buff) , "%s Failed! log_name NULL\n", __FUNCTION__);
    write(penv->self_fd , buff , strlen(buff));
    //_write_self_msg(penv, "%s Failed! log_name NULL", __FUNCTION__);
    return -1;
  }

  if(max_seq<0 || curr_seq>=max_seq)
  {
    snprintf(buff , sizeof(buff) , "%s Failed! seq illegal! curr_seq:%d max_seq:%d" , __FUNCTION__ , 
      curr_seq , max_seq);
    write(penv->self_fd , buff , strlen(buff));
    return -1;
  }

  /***Remove*/
  while(1)
  {
    //no need to again
    if(seq < curr_seq)
      break;

    
    //rename    
    snprintf(old_file_name , sizeof(old_file_name) , "%s.%d" , log_name , seq);
    snprintf(new_file_name , sizeof(new_file_name) , "%s.%d" , log_name , (seq+1));

    ret = rename(old_file_name , new_file_name);
    if(ret < 0)
    {
      //snprintf(buff , sizeof(buff) , "%s:trace info. msg:%s", __FUNCTION__ , strerror(errno));
      //write(penv->self_fd , buff , strlen(buff));
    }

    //rotate
    seq--;
  }

  return 0;
}


static int _write_self_msg(SLOG_ENV *penv , char *fmt , ...)
{
  va_list ap;
  
  struct stat stat_info;
  char file_name[SLOG_LOG_NAME_LEN] = {0};
  char msg[SLOG_MAX_LINE_LEN] = {0};
  char buff[SLOG_MAX_LINE_LEN] = {0};

  struct tm *local_tm = NULL;
  time_t ts;  
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int min = 0;
  int sec = 0;
  
  int ret = -1;
  /***Arg Check*/
  if(!penv || !fmt)
  {
    printf("%s: env or msg null\n" , __FUNCTION__);
    return -1;
  }

  /***Open Local*/
  if(penv->self_fd < 0)
  {
    snprintf(file_name , sizeof(file_name) , "%s.%d" , SLOG_SELF_FILE , penv->self_seq);
    penv->self_fd = open(file_name , O_RDWR|O_APPEND|O_CREAT , S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(penv->self_fd < 0)
    {
      fprintf(stderr , "%s:open %s failed!err:%s\n" , __FUNCTION__ , file_name , strerror(errno));
      return -1;
    }    
  }

  /***Time*/
  ts = time(NULL);
  local_tm = localtime(&ts);
  if(local_tm)
  {
    year = local_tm->tm_year+1900;
    month = local_tm->tm_mon+1;
    day = local_tm->tm_mday;
    hour = local_tm->tm_hour;
    min = local_tm->tm_min;
    sec = local_tm->tm_sec;
  }

  /***Get File Size*/
  do
  {
    //0.fstated filed
    if(penv->self_stated > 0)
      break;

    snprintf(file_name , sizeof(file_name) , "%s.%d" , SLOG_SELF_FILE , penv->self_seq);
    //1.stat opened file
    ret = fstat(penv->self_fd , &stat_info);
    if(ret < 0)
    {      
      snprintf(buff , sizeof(buff) , "[%d-%02d-%02d %02d:%02d:%02d] %s:fstat %s failed! err:%s\n" , year , month , day , hour , min , sec ,
        __FUNCTION__ , file_name , strerror(errno));
      write(penv->self_fd , buff , strlen(buff));
      break;
    }

    penv->self_stated = 1;
    //2.set write_bytes    
    penv->self_writes = stat_info.st_size;
    //printf("%s fstated %s. size:%d\n" , __FUNCTION__ , file_name , penv->self_writes);
    break;
  }
  while(0);

  /***Get Msg*/
  va_start(ap , fmt);
  vsnprintf(msg , sizeof(msg) , fmt , ap);
  va_end(ap);

  /***Construct Buff*/
  memset(buff , 0 , sizeof(buff));
  snprintf(buff , sizeof(buff) , "[%d-%02d-%02d %02d:%02d:%02d] %s\n" , year , month , day , hour , min , sec ,
   msg);

  /***Write File*/
  //printf("%s" , buff);
  write(penv->self_fd , buff , strlen(buff));
  penv->self_writes += strlen(buff);


  /***Check Rotate*/
  if(penv->self_writes >= SELF_FILE_SIZE)
  {
    ret = _rotate_log_file(penv, SLOG_SELF_FILE , penv->self_seq , SELF_FILE_ROTATE_MAX);
    if(ret == 0)
    {
      //close current file.when writes will open self again(new file)
      close(penv->self_fd);
      penv->self_fd = -1;
      penv->self_stated = 0;
      penv->self_writes = 0;
    }
  }
  return 0;
}


/*
Inner Function No Check arg
*/
static int _slog_log(SLOG_ENV *penv , SLOG_NODE *pnode , SLOG_LEVEL level , SLOG_DEGREE log_degree , 
  SLOG_FORMAT format , char *fmt , va_list arg_ap)
{
  struct tm *local_tm = NULL;  
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int min = 0;
  int sec = 0;

  FILE *log_fp = NULL;
  //char buff[SLOG_MAX_LINE_LEN] = {0};
  char *buff = pnode->log_buff;
  char time_appened[16] = {0};
  int len = 0;
  int ret = -1;

  time_t ts = 0;
  long append_num = 0;
  
  struct timeval tv;
  struct timespec tp;
  va_list ap;
 
  /***Init*/
  log_fp = pnode->fp;

  /***va_list*/
  va_copy(ap , arg_ap);

  /***Format*/
  if(format == SLF_RAW)
    goto _print;
   
  /***Time and Label*/
  if(log_degree>=SLD_SEC && log_degree<=SLD_MIC)
  {
    ret = gettimeofday(&tv , NULL);
    do
    {
      //check err
      if(ret < 0)
      {
        _write_self_msg(penv, "%s err.call gettimeofday failed! err:%s", __FUNCTION__ , strerror(errno));
        ts = time(NULL); //use default
        break;
      }
  
      ts = tv.tv_sec;
      //switch
      switch(log_degree)
      {
      case SLD_SEC:
        //time_appened[0]=':';
      break;
  
      case SLD_MILL:
        append_num = tv.tv_usec/1000;
        snprintf(time_appened , sizeof(time_appened) , ":%3ld" , append_num);
      break;
  
      case SLD_MIC:
        append_num = tv.tv_usec;
        snprintf(time_appened , sizeof(time_appened) , ":%6ld" , append_num);
      break;
  
      default:
      break;
      }
  
      break;
    }
    while(0);
  }
  else  //nano
  {
    ret = clock_gettime(CLOCK_REALTIME, &tp);
    if(ret < 0)
    {
      _write_self_msg(penv, "%s err.call clock_gettime failed! err:%s", __FUNCTION__ , strerror(errno));
      ts = time(NULL); //use default  
    }
    else
    {
      ts = tp.tv_sec;
      append_num = tp.tv_nsec;
      snprintf(time_appened , sizeof(time_appened) , ":%9ld" , append_num);
    }
  }

  local_tm = localtime(&ts);
  if(local_tm)
  {
    year = local_tm->tm_year+1900;
    month = local_tm->tm_mon+1;
    day = local_tm->tm_mday;
    hour = local_tm->tm_hour;
    min = local_tm->tm_min;
    sec = local_tm->tm_sec;
  }

  snprintf(buff , SLOG_LOG_LINE_BUFF , "[%d-%02d-%02d %02d:%02d:%02d%s %5s] " , year , month , day , hour , min , sec ,
    time_appened , _LOG_LEVEL_LABEL[level]);
  len = strlen(buff);

  /***Print Arg*/
_print:
  vsnprintf(&buff[len] , SLOG_LOG_LINE_BUFF-len-1 , fmt , ap); //content+\n+\0
  va_end(ap);

  len = strlen(buff);
  /***Print*/
  if(pnode->type == SLT_LOCAL) //FILE
  {
    //Do not flush buffer now. improve Performance.
    //printf("buff is :%s\n" , buff);
    fprintf(log_fp , "%s\n" , buff);
    //fflush(log_fp);
  }
  else //NET
  {
    do
    {
      if(len <= 0)
        break;

      buff[len]='\n';      
      len++;

      /*
      ret = send(pnode->type_info._net.sock_fd , buff , len , 0);
      if(ret < 0)
      {
        _write_self_msg(penv, "<%s> send to %s:%d failed! err:%s", __FUNCTION__ , pnode->type_info._net.ip , 
          pnode->type_info._net.port , strerror(errno));
        return 0;
      }*/

      _net_log(penv, pnode, buff, len);
      break;
    }
    while(0);
  }
  /***Return*/  
  return len;
}

static void _net_log(SLOG_ENV *penv , SLOG_NODE *pnode , char *buff , int len)
{
  NET_LOG_RINGBUFF *prb = NULL;
  int ret = -1; 
  char *dst = NULL;
  
  /***Send*/
  ret = send(pnode->type_info._net.sock_fd , buff , len , 0);
  if(ret < 0)
  {
    _write_self_msg(penv, "<%s> failed! err:%s" , __FUNCTION__ , strerror(errno));
  }

  /***BUFF ACTIVATED*/
  /*
  Now InActive.
  */
  if(MAX_RB_ITEM <= 0)
  {    
    return;
  }

  /**********Not Here Right Now****/
  prb = pnode->type_info._net.prb_info;
  /***Ret Success*/
  /*we don't know whether this sent is a real successful send.
  * even we don't know whether last send success.
  * save current item
  * and clear all saved item
  */
  if(ret >= 0)
  {
    _write_self_msg(penv, "<%s> send temp done! msg:%s", __FUNCTION__ , buff);
    //rest
    prb->count = 0;
    prb->head = 0;
    prb->tail = 0;

    //cpy
    dst = &prb->rb[prb->tail*SLOG_MAX_LINE_LEN];
    strncpy(dst , buff , len);
    len++;
    dst[len] = 0;

    //update
    prb->tail++;
    prb->count++;

    return;
  }

  /***Ret Fail*/  
  switch(errno)
  {
    //caused by sys.
    case EAGAIN:
     //case EWOULDBLOCK:
    case EINTR:
    case ENOBUFS:
    //cased by peer.[no listen etc.]
    case ECONNRESET:
    case ECONNREFUSED:
      _write_self_msg(penv , "<%s> store in ringbuff temp." , __FUNCTION__);
      //append to ringbuffer
      if(prb->count >= prb->total)
      {
        prb->head = (prb->head + 1)% prb->total;
        prb->count--;
      }
        //cpy
      dst = &prb->rb[prb->tail*SLOG_MAX_LINE_LEN];
      strncpy(dst , buff , len);
      len++;
      dst[len] = 0;
    
        //upt
      prb->tail = (prb->tail + 1) % prb->total;
      prb->count++;
      _write_self_msg(penv, "<%s> ringbuff>> total:%d count:%d head:%d tail:%d", __FUNCTION__ , prb->total , 
        prb->count , prb->head , prb->tail);

      //refresh ringbuffer
      _net_refresh_log(penv, pnode);
      
    break;

    default:
      _write_self_msg(penv, "<%s> drop log directly! errno:%d content:%s" , __FUNCTION__ , errno , buff);
      return;
    break;    
  }

  return;
}

static int _net_refresh_log(SLOG_ENV *penv , SLOG_NODE *pnode)
{
  NET_LOG_RINGBUFF *prb = NULL;
  int ret = -1; 
  char *dst = NULL;
  int i = 0;

  prb = pnode->type_info._net.prb_info; 
  /***RB FULL*/
  if(prb->head == prb->tail)
  {
    _write_self_msg(penv, "<%s> resend all!", __FUNCTION__);
    //error
    if(prb->count != prb->tail)
    {
      _write_self_msg(penv, "<%s> ringbuff meets an problem. count:%d neq total:%d. refresh all!", __FUNCTION__ , 
        prb->count , prb->total);
      prb->count = 0;
      prb->head = 0;
      prb->tail = 0;
      return -1;
    }

    //head-->end
    for(i=prb->head; i<prb->total; i++)
    {
      dst = &prb->rb[i*SLOG_MAX_LINE_LEN];
      ret = send(pnode->type_info._net.sock_fd , dst , strlen(dst) , 0);
      if(ret < 0)
        _write_self_msg(penv, "<%s> resend all rb[%d]:%s failed! err:%s", __FUNCTION__ , i , dst , strerror(errno));
      else
        _write_self_msg(penv, "<%s> resend all rb[%d]:%s done!", __FUNCTION__ , i , dst);
   
    }

    //first-->tail
    for(i=0; i<prb->tail; i++)
    {
      dst = &prb->rb[i*SLOG_MAX_LINE_LEN];
      ret = send(pnode->type_info._net.sock_fd , dst , strlen(dst) , 0);
      if(ret < 0)
        _write_self_msg(penv, "<%s> resend all rb[%d]:%s failed! err:%s", __FUNCTION__ , i , dst , strerror(errno));  
      else
        _write_self_msg(penv, "<%s> resend all rb[%d]:%s done!", __FUNCTION__ , i , dst);
    }

    return 0; 
  }
  
  /**head-->tail*/
  if(prb->head < prb->tail) 
  {
    _write_self_msg(penv, "<%s> resend rb from head to tail!", __FUNCTION__);
    for(i=prb->head; i<prb->tail; i++)
    {
      dst = &prb->rb[i*SLOG_MAX_LINE_LEN];
      ret = send(pnode->type_info._net.sock_fd , dst , strlen(dst) , 0);
      if(ret < 0)
        _write_self_msg(penv, "<%s> resend h->t rb[%d]:%s failed! err:%s", __FUNCTION__ , i , dst , strerror(errno));  
      else
        _write_self_msg(penv, "<%s> resend h->t rb[%d]:%s done!", __FUNCTION__ , i , dst);
    }

    return 0;
  }

  /***head > tail*/
  //will not happend here
  _write_self_msg(penv, "<%s> fresh rb. head > tail! should not happen here!", __FUNCTION__);
  prb->count = 0;
  prb->head = 0;
  prb->tail = 0;
  return -1;    
}

static int _net_open(SLOG_ENV *penv , SLOG_NODE *pnode)
{
  int ret = -1;
  int flags = 0;
  int sock_fd = -1;
  NET_LOG_RINGBUFF *prb = NULL;

  memset(&pnode->type_info._net.remote_addr , 0 , sizeof(struct sockaddr_in));
  //create
  sock_fd = socket(AF_INET , SOCK_DGRAM , 0);
  if(sock_fd < 0)
  {
    _write_self_msg(penv, "<%s> socket failed! err:%s",__FUNCTION__ , strerror(errno));
    return -1;
  }

  //non-block
  flags = fcntl(sock_fd , F_GETFL , NULL);
  if(flags < 0)
  {
    _write_self_msg(penv, "<%s> fcntl get flag failed! err:%s",__FUNCTION__ , strerror(errno));
    close(sock_fd);
    return -1;
  }

  flags |= O_NONBLOCK;
  ret = fcntl(sock_fd , F_SETFL , &flags);
  if(flags < 0)
  {
    _write_self_msg(penv, "<%s> fcntl set flag non-block failed! err:%s",__FUNCTION__ , strerror(errno));
    close(sock_fd);
    return -1;
  }
     
  //send buff
  flags = (1*1024*1024); //1M. real is doubled
  ret = setsockopt(sock_fd , SOL_SOCKET , SO_SNDBUF , &flags , sizeof(flags));
  if(ret < 0)
  {
    _write_self_msg(penv, "<%s> setsockopt set sendbuf failed! err:%s",__FUNCTION__ , strerror(errno));
    close(sock_fd);
    return -1;
  }

  //connect
  pnode->type_info._net.remote_addr.sin_family = AF_INET;
  pnode->type_info._net.remote_addr.sin_port = htons(pnode->type_info._net.port);
  ret = inet_aton(pnode->type_info._net.ip , &pnode->type_info._net.remote_addr.sin_addr);
  if(ret == 0)
  {
    _write_self_msg(penv, "<%s> inet_aton %s failed!",__FUNCTION__ , pnode->type_info._net.ip);
    close(sock_fd);
    return -1;  
  }
  ret = connect(sock_fd , (struct sockaddr *)&pnode->type_info._net.remote_addr , 
        sizeof(pnode->type_info._net.remote_addr));
  if(ret < 0)
  {
    _write_self_msg(penv, "<%s> connect %s:%d failed!",__FUNCTION__ , pnode->type_info._net.ip , pnode->type_info._net.port);
    close(sock_fd);
    return -1;  
  }

  //alloc ringbuff
  if(MAX_RB_ITEM > 0)
  {
    prb = (NET_LOG_RINGBUFF *)calloc(1 , (sizeof(NET_LOG_RINGBUFF) + MAX_RB_ITEM*SLOG_MAX_LINE_LEN));
    if(!prb)
    {
      _write_self_msg(penv, "<%s> alloc ringbuff %s:%d failed!",__FUNCTION__ , pnode->type_info._net.ip , pnode->type_info._net.port);
      close(sock_fd);
      return -1;
    }
    prb->total = MAX_RB_ITEM;
    pnode->type_info._net.prb_info = prb;
  }

  //set info  
  pnode->type_info._net.sock_fd = sock_fd;
  return 0;
}

