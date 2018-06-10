# Sample makefile

cc = cl
link = link
cflags = $(cflags) /c -I"include"

all: cg.exe

.cpp.obj:
  $(cc) $(cdebug) $(cflags) $(cvars) $*.cpp

cg.exe: main.obj
  $(link) $(ldebug) $(conflags) -out:cg.exe $** $(conlibs) lib/ConsoleGraphicsLibrary.lib gdiplus.lib
