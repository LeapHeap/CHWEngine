@echo off
gcc.exe demo.c ./output/demo.exe -fno-ms-extensions -O2 -pipe -Werror=implicit-int -Werror=return-type -Werror=vla -lws2_32 -Wl,--stack,12582912 -s -L../ -lCHWEngine -static
pause