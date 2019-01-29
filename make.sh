#!/bin/bash
#use dynamic lib
gcc -g use_slog.c -lslog -lm -o use_slog

#use static lib
#gcc -g use_slog.c -Wl,-Bstatic -lslog -Wl,-Bdynamic -lm -o use_slog
