#include <slog/slog.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>

extern int errno;
int main(int argc , char **argv)
{
  int sld = -1;
  int sld_net = -1;
  int i = 0;
  char buff[512] = {0};
  char err_msg[1024] = {0};
  char value = 'a';
  int k = 0;
  SLOG_OPTION slog_opt;

  memset(&slog_opt , 0 ,sizeof(slog_opt));
  strncpy(slog_opt.type_value._local.log_name , "avman.log" , 256);
  slog_opt.log_degree = SLD_SEC;
  slog_opt.log_size = (10*1024);
  slog_opt.rotate = 5;
  slog_opt.format = SLF_RAW;

  //Test Open and Close
  /*    
  while(1)
  {
    if(i==10)
      break;
    memset(buff , 0 , sizeof(buff));
    snprintf(buff , sizeof(buff) , "av%d.log" , i);
    strncpy(slog_opt._type_value._local.log_name , buff , 256);
    sld = slog_open(SLT_LOCAL , SL_DEBUG , &slog_opt ,err_msg);
    if(sld < 0)
    {
      printf("err! msg:%s\n" , err_msg);
      return -1;
    }
    printf("%d open success!\n" , sld);
    slog_log(sld , SL_INFO , "slog_open success! sld:%d" , sld);
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
  return 0; 
  */
  

  sld = slog_open(SLT_LOCAL , SL_VERBOSE , &slog_opt, err_msg);
  if(sld < 0)
  {
    printf("err! msg:%s\n" , err_msg);
    return -1;
  }
  slog_log(sld , SL_INFO , "slog_open success! sld:%d" , sld);

  /*Test Net*/
  memset(&slog_opt , 0 ,sizeof(slog_opt));
  strncpy(slog_opt.type_value._net.ip , "127.0.0.1" , sizeof(slog_opt.type_value._net.ip));
  slog_opt.type_value._net.port = 7777;
  slog_opt.log_degree = SLD_MIC;
  //slog_opt.log_size = 1024;
  //slog_opt.rotate = 5;
  slog_opt.format = SLF_PREFIX;
  
  sld_net = slog_open(SLT_NET , SL_DEBUG , &slog_opt , err_msg);
  if(sld_net < 0)
  {
    printf("open net failed! msg:%s\n" , err_msg);
    return -1;
  }
  printf("open sld net success!\n");
  slog_log(sld_net , SL_DEBUG , "[%d]nice to meet you!" , i++);
  slog_log(sld_net , SL_DEBUG , "[%d]%s is a good girl!" , i++ , "suomei");
  slog_log(sld_net , SL_DEBUG , "[%d]%s age:%d is %s!" , i++ , "cs" , 37 , "bad man");
  

  /*Test ChgAttr
  slog_log(sld , SL_VERBOSE , "This is:%s and age:%d" , "Verbose" , 31);
  slog_log(sld , SL_DEBUG , "This is:%s and age:%d" , "Debug" , 29);
  slog_chg_attr(sld , SL_ERR , SLD_MILL , 11*1024 , -1 , SLF_PREFIX);
  slog_log(sld , SL_INFO , "This is:%s and age:%d" , "Info" , 22);
  slog_log(sld , SL_ERR , "This is:%s and age:%d" , "Error" , 41);
  slog_log(sld , SL_FATAL , "This is:%s and age:%d" , "Fatal" , 60);

  slog_chg_attr(sld , SL_DEBUG , SLD_MIC , 10*1024 , -1 , -1);
  slog_log(sld , SL_VERBOSE , "This is:%s and age:%d" , "Verbose" , 31);
  slog_log(sld , SL_DEBUG , "This is:%s and age:%d" , "Debug" , 29);
  slog_log(sld , SL_INFO , "This is:%s and age:%d" , "Info" , 22);
  slog_chg_attr(sld , -1 , SLD_NANO , -1 , -1 , -1);
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
  slog_close(sld_net);
  return 0;
}
