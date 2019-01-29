#!/bin/bash
#use dynamic lib
gcc -g use_slog.c -lslog -lm -lrt -o use_slog

#use static lib
#gcc -g use_slog.c -Wl,-Bstatic -lslog -Wl,-Bdynamic -lm -lrt -o use_slog
