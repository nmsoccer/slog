package main 

import (
	"fmt"
	"time"
	slog "test_c/clib/slog"
	"strconv"
	"sync"
)

func test_pressure() {
	sld := slog.SLogLocalOpen(slog.SL_VERBOSE ,"./press_slog" , slog.SLF_PREFIX , slog.SLD_SEC , 1024*1024*10 , 9);
	fmt.Printf("slogd:%d\n", sld);
	if sld < 0 {
		fmt.Printf("open press failed!\n");
	}	

	var uid int64 = 210001200;
    var zone_id int64 = 712313746151703765;
    var exp int = 1600;
    var gold int = 1230;
    var last_words string ="show me the money";
    var success int = 0;
    var fail int = 0;
    var ret int = 0;  
    fmt.Printf("test starts\n");
    for i:=0; i<1024*1024; i++ {
        ret = slog.SLog(sld , slog.SL_INFO , "game_result end uid=%d, zone_id=%d, add_exp=%d, add_gold=%d, is_last_survive=true , is_ob_exit=nil, ob_time=nil, fight_type=101,send_to_client_flag=true , last_word:%s" , 
        	uid , zone_id , exp , gold , last_words);
        if ret == 0 {
          success ++;
        }  else {
          fail++;
        } 
  }
  fmt.Printf("test done success:%d fail:%d\n" , success , fail);
}

func test_go() int {
	slds := [10]int{-1};
	check := make(chan int);
	var lock sync.Mutex;
	
	for i:=0; i<len(slds); i++ {
		slds[i] = slog.SLogLocalOpen(slog.SL_DEBUG , "test_go."+strconv.Itoa(i)+".log" , slog.SLF_PREFIX , slog.SLD_SEC , 1024*1024 , 5);
		if slds[i] < 0 {
			fmt.Printf("open %d log failed!\n", i);
			return -1;
		} else {
			fmt.Printf("open %d log success!\n", i);
		}
	}
	
	//write
	for i:=0; i<len(slds); i++ {
		go func(sld int) {
			for count:=0; count<100; count++ {
				slog.SLog(sld , slog.SL_DEBUG , "[%d] writes string %s beat %s %d times!" , sld , "cs" , "suomei" , count);
			}
			fmt.Printf("[%d] exit!\n" , sld);
			lock.Lock();
			slog.SLogClose(sld);
			lock.Unlock();
			check <- 1;
		}(slds[i]);
				
	}
	
	//main	
	total := 0;
	for v := range check {
		total += v;
		if total >= 10 {
			fmt.Printf("all exist\n");
			return 0;
		}
	}
	//time.Sleep(10e9);
	
	return 0;
}


func normal() {
	/*local*/
	l_slogd := slog.SLogLocalOpen(slog.SL_VERBOSE ,"./local_log" , slog.SLF_PREFIX , slog.SLD_MILL , 1024*100 , 5);
	fmt.Printf("l_slogd:%d\n", l_slogd);
	
	//log
	slog.SLog(l_slogd , slog.SL_DEBUG , "good night %s test %s total %d times!" , "cs" , "suomei" , 20);
	
	//chgattr
	slog.SLogChgAttr(l_slogd, -1, slog.SLD_SEC, -1, -1, -1);
	slog.SLog(l_slogd , slog.SL_INFO , "slog:%d type:%T" , l_slogd , l_slogd);
	
	
	/*net*/
	n_slogd := slog.SLogNetOpen(slog.SL_DEBUG ,"127.0.0.1" , 7777 , slog.SLF_PREFIX , slog.SLD_MILL);
	fmt.Printf("n_slogd:%d\n", n_slogd);
	
	if n_slogd >= 0 {
		slog.SLog(n_slogd , slog.SL_DEBUG , "%s is a piece of d! he tests %s %d times!" , "cs" , "suomei" , 13);
	}
	
	slog.SLogClose(l_slogd);
	slog.SLogClose(n_slogd);
	time.Sleep(1e9);
}

func main() {
	fmt.Printf("TestC\n");
	normal();
	//test_pressure();
	//test_go();
}

