gcc main.cpp -Iinclude -Llib -lConsoleGraphicsLibrary -lstdc++ -ocg -framework ApplicationServices -framework AppKit
if [ -f $FILE ];
then
	./cg
else
	read
fi