# SLOG  
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu) [![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)   
A simple log library
一个简单的程序本地&网络日志库   

支持功能:
* **日志分级** 目前分为verbose,debug,info,err,fatal等五个等级，打印日志时设定。  
* **日志过滤** 在打开日志结构时设置过滤级别.  
* **日志滚动** 支持日志的自动滚动，可以设置滚动下标上限和单日志文件大小  
* **时间粒度** 支持设置日志的不同粒度，默认粒度到秒，可以到毫秒，微秒及纳秒  
* **属性修正** 在进程过程中可以动态修改打开日志时的一些基本属性  
* **网络日志** 支持将文本日志以UDP包的方式直接发到远程服务器  
_注意：该日志库目前非线程安全，多线程类型程序需斟酌使用._

### C安装  
下载安装包文件xx.zip  
unzip xx.zip  
./install.sh  
_默认头文件会装到/usr/local/include/slog 库文件位于/usr/local/lib/libslog.so_    

编译:  
gcc use_slog.c -lm -lslog -o use_slog  
_如果找不到动态库请先将/usr/local/lib加入到/etc/ld.so.conf 然后执行/sbin/ldconfig_  

### GO安装:   
首先编译安装slog的C头文件及库文件   
下载go/slog.go到工作目录 比如GOPATH/src/test_c/clib/slog 然后go intall 安装  
在源码里引用slog即可 比如import slog test_c/clib/slog(参考go/test_c.go)  

### CAPI:
- **``int slog_open(SLOG_TYPE type , SLOG_LEVEL filt_level , SLOG_OPTION *option , char *err);``**
  * SLOG_OPTION结构:
  ```
  struct
  {
    union
    {
      //SLT_LOCAL
      struct
      {
        char log_name[256]; 
      }_local;
      //SLT_NETWORK
      struct
      {
        char ip[64];
        int port;
      }_net;
    
    }_type_value;

    SLOG_DEGREE log_degree;
    SLOG_FORMAT format;
    int log_size;
    int rotate;  
  }
  ```
  * 用于程序中打开一个日志控制结构。成功则返回控制结构描述符，失败返回-1并填充错误信息到err  
  * type:日志类型，是本地文件日志还是输出到网络日志   
    _type_value._local.log_name:如果是本地日志类型，则用作日志的文件名  
    _type_value._network.ip&port:远端服务器监听udp的ip及端口  
  * filt_level:设定日志过滤级别，低于该级别的日志则不会输出。具体宏请参见slog.h:SL_XX    
  * log_degree:日志记录粒度。如果填0默认值为秒。具体请参见宏slog.h  
  * log_size:单个日志文件大小.如果填0则默认为10M.  
  * rotate:日志滚动下标上限。如果填0则使用默认值5  
  * format:标记是加前缀打印日志(包含日期 级别)还是原生输出.如果填0则默认带时间前缀  
  * err:错误时返回错误信息。  

_说明：相同文件名(包括路径)只能打开一次。另外的不同日志文件可以在程序里打开，返回不同的描述符_


- **``int slog_close(int sld);``**  
  * _关闭一个已经打开的日志控制描述符_ 

- **``int slog_log(int sld , SLOG_LEVEL log_level , char *fmt , ...);``**  
  * _打印一条日志_  
  * sld:通过slog_open调用成功之后返回的描述符  
  * log_level:该条日志的等级  
  * fmt及...：日志的模式及内容 形如printf  

- **``int slog_chg_attr(int sld , int filt_level , int degree , int size , int rotate);``**    
  * _动态更新已打开描述符的属性_  
  * sld:已打开的描述符  
  * filt_level:新的过滤等级.-1则忽略  
  * degree:新的日志粒度. -1则忽略  
  * size:新的单个日志文件大小(只对LOCAL日志有效) -1则忽略  
  * rotate:滚动下标上限(只对LOCAL日志有效) -1则忽略  

### GOAPI
- **``SLogLocalOpen(filt_level int , log_name string , format int , log_degree int) int``**    
  * 打开一个本地的日志句柄。失败则返回-1 >=0成功    
  * filt_level:设定日志过滤级别，低于该级别的日志则不会输出。具体宏请参见slog.go:SL_XX
  * log_name:日志名
  * format:标记是加前缀打印日志(包含日期 级别)还是原生输出.如果填0则默认带时间前缀
  * log_degree:日志记录粒度。如果填0默认值为秒。具体请参见宏slog.h  
  * log_size:单个日志文件大小.如果填0则默认为10M.  
  * rotate:日志滚动下标上限。如果填0则使用默认值5
  
- **``SLogNetOpen(filt_level int , ip string , port int , format int , log_degree int) int``**  
  * 打开一个网络的日志句柄(对端需监听一个UDP端口)。失败则返回-1 >=0成功    
  * filt_level:设定日志过滤级别，低于该级别的日志则不会输出。具体宏请参见slog.go:SL_XX
  * ip:接收端ip地址
  * port:接收端监听端口
  * format:标记是加前缀打印日志(包含日期 级别)还是原生输出.如果填0则默认带时间前缀
  * log_degree:日志记录粒度。如果填0默认值为秒。具体请参见宏slog.h  
 
- **``SLogClose(sld int) int``**
  * _关闭一个已经打开的日志控制描述符_

- **``SLog(sld int , log_level int , format string, arg ...interface{}) int``**
  * _打印一条日志_  
  * sld:通过slog_open调用成功之后返回的描述符  
  * log_level:该条日志的等级  
  * format及arg：日志的模式及内容 形如fmt.Printf()里的格式化参数  
  
- **``SLogChgAttr(sld int, filt_level int, degree int, size int, rotate int, format int) int``**  
  * _动态更新已打开描述符的属性_  
  * sld:已打开的描述符  
  * filt_level:新的过滤等级.-1则忽略  
  * degree:新的日志粒度. -1则忽略  
  * size:新的单个日志文件大小(只对LOCAL日志有效) -1则忽略  
  * rotate:滚动下标上限(只对LOCAL日志有效) -1则忽略  

**备注:**    
***以上所有API调用过程中产生的错误以及调试信息都被打印在主进程的执行目录下slog.log.*中***    

### SAMPLE
下面的测试代码均源于use_slog.c  
* 本地日志  
```
  ...
  int sld = -1;
  SLOG_OPTION slog_opt;

  memset(&slog_opt , 0 ,sizeof(slog_opt));
  strncpy(slog_opt.type_value._local.log_name , "avman.log" , 256);
  /*use 0 as default or set each one
  slog_opt.log_degree = SLD_SEC;
  slog_opt.log_size = (10*1024);
  slog_opt.rotate = 5;
  slog_opt.format = SLF_PREFIX;
  */
  
  sld = slog_open(SLT_LOCAL , SL_VERBOSE , &slog_opt, err_msg);
  if(sld < 0)
  {
    printf("err! msg:%s\n" , err_msg);
    return -1;
  }
  slog_log(sld , SL_INFO , "slog_open success! sld:%d" , sld);
  slog_log(sld , SL_INFO , "then try to open slog_net!");
  
  slog_close(sld);
  ...
  
```
上面的代码记录了本地日志，在设置日志选项时可以只填写日志名，其他使用0则会使用默认的参数配置，也可以自行设置相关参数  

* 网络日志
```
  ...
  int sld_net = -1;
  
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
  
  slog_close(sld_net);
  
```
上面例子打开了一个网络日志slog描述符并发送日志信息到远端机器。注意这里需要远端服务器自行监听对应端口号及收包.比如可以通过编译运行目录下的udp_serv.c来监听7777端口并进行测试收包:  
```
  gcc -g udp_serv.c -o udp_serv
  ./udp_serv 
  buff[56]>>>[2019-01-29 15:24:36:319362 debug] [0]nice to meet you!
  buff[61]>>>[2019-01-29 15:24:36:319397 debug] [1]suomei is a good girl!
  buff[60]>>>[2019-01-29 15:24:36:319418 debug] [2]cs age:37 is bad man!
  ...
```

### 简单压测：  
gcc -g use_slog.c -lm [-lrt] -lslog -o use_slog  
**环境：**   
CPU：Intel(R) Xeon(R) CPU E5-2682 v4 @ 2.50GHz 单核  
MEM：2G  
OS：CentOS 2.6.18-308.el5  

**数据：**    
循环打印100万条（1\*1024\*1024）日志数据,单条数据长度210Byte左右  
单条日志长度10M.滚动上限9  

#### 调整日志粒度为秒级:
_测试结果：成功率100%_  
* CPU 99%  
* MEM 2720K    
* usr_time 1.86s  
* sys_time 0.25s  
* total_time 2.11s  


#### 调整日志粒度为纳秒级:  
_测试结果：成功率100%_  
* CPU 99%
* MEM 2736K
* usr_time 2.19s
* sys_time 0.32s
* total_time 2.52s

#### 调整日志数量为1000万条（10\*1024\*1024） 其余测试条件不变 
_测试结果：成功率100%_  
* CPU 90%
* MEM 2752K
* usr_time 18.89s
* sys_time 3.41s
* total_time 24.77s

