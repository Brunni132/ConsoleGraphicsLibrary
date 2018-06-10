rm cg.exe *.obj
call vcvars32.bat
cl.exe /c -I"include" main.cpp
link -out:cg.exe main.obj lib/ConsoleGraphicsLibrary.lib gdiplus.lib
cg.exe
