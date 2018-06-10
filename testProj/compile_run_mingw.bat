rm cg.exe *.obj
gcc main.cpp -Iinclude -Llib -lConsoleGraphicsLibrary -lstdc++ -lgdiplus -ocg.exe
cg.exe
