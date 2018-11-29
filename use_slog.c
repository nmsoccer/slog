#include "slog.h"
#include<stdio.h>
#include<errno.h>
#include<string.h>

extern int errno;
int main(int argc , char **argv)
{
  int sld = -1;
  int i = 0;
  char buff[512] = {0};
  char err_msg[1024] = {0};
  char value = 'a';
  int k = 0;

  //Test Open and Close
  /*  
  while(1)
  {
    if(i==10)
      break;
    memset(buff , 0 , sizeof(buff));
    snprintf(buff , sizeof(buff) , "av%d.log" , i);
    sld = slog_open(buff , SLOG_L_DEBUG , SLOG_D_SEC, 10*1024,5,err_msg);
    if(sld < 0)
    {
      printf("err! msg:%s\n" , err_msg);
      return -1;
    }
    printf("slog_open success! sld:%d\n" , sld);
    sleep(2);
    i++;
  }

  while(1)
  {
    if(i < 0)
      break;
    slog_close(i);
    i--;
  }
  */

  sld = slog_open("avman.log" , SL_VERBOSE , SLD_SEC, 0,5,err_msg);
  if(sld < 0)
  {
    printf("err! msg:%s\n" , err_msg);
    return -1;
  }
  printf("slog_open success! sld:%d\n" , sld);

  /*Test ChgAttr
  slog_log(sld , SL_VERBOSE , "This is:%s and age:%d" , "Verbose" , 31);
  slog_log(sld , SL_DEBUG , "This is:%s and age:%d" , "Debug" , 29);
  slog_chg_attr(sld , SL_VERBOSE , SLD_MILL , 10*1024 , -1);
  slog_log(sld , SL_INFO , "This is:%s and age:%d" , "Info" , 22);
  slog_log(sld , SL_ERR , "This is:%s and age:%d" , "Error" , 41);
  slog_log(sld , SL_FATAL , "This is:%s and age:%d" , "Fatal" , 60);

  slog_chg_attr(sld , SL_VERBOSE , SLD_MIC , 100*1024 , -1);
  slog_log(sld , SL_VERBOSE , "This is:%s and age:%d" , "Verbose" , 31);
  slog_log(sld , SL_DEBUG , "This is:%s and age:%d" , "Debug" , 29);
  slog_log(sld , SL_INFO , "This is:%s and age:%d" , "Info" , 22);
  slog_chg_attr(sld , -1 , SLD_NANO , -1 , -1);
  slog_log(sld , SL_ERR , "This is:%s and age:%d" , "Error" , 41);
  slog_log(sld , SL_FATAL , "This is:%s and age:%d" , "Fatal" , 60);
  */

  /*Test Rotate
  while(1)
  {
    if(i>=13)
      break;
    
    for(k=0; k<256; k++)
    {
      buff[k] = value;
    }
    buff[k] = 0;
    value++;
  

    slog_log(sld , SL_DEBUG , "TestLog:%s" , buff);
    i++;
  }*/

  /*Test Pressure
  slog_chg_attr(sld , SL_DEBUG , SLD_NANO , 10*1024*1024 , 9);
  long uid = 210001200;
  long zone_id = 712313746151703765;
  int exp = 1600;
  int gold = 1230;
  char *last_words="show me the money";
  int success = 0;
  int fail = 0;
  int ret = 0;  

  printf("test starts\n");
  for(i=0; i<1024*1024; i++)
  {
    ret = slog_log(sld , SL_INFO , "game_result end uid=%ld, zone_id=%ld, add_exp=%d, add_gold=%d, is_last_survive=true , is_ob_exit=nil,"
     "ob_time=nil, fight_type=101,send_to_client_flag=true , last_word:%s" , uid , zone_id , exp , gold , last_words);
    if(ret == 0)
      success ++;
    else
      fail++; 
  }
  printf("test done success:%d fail:%d\n" , success , fail);
  */
 

  slog_close(sld);
  return 0;
}
