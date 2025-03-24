@echo off
echo. > temp_out.txt
start /B stockfish.exe < temp_in.txt > temp_out.txt
ping 1.1.1.1 -n 1 -w 4500 >NUL