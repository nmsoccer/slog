package main 

import (
	"fmt"
	"time"
	slog "test_c/clib/slog"
)



func main() {
	fmt.Printf("TestC\n");
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

