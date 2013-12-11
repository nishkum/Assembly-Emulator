emulator: emulator.o
	g++ -o emulator emulator.o
	chmod +x run.sh
emulator.o:emulator.cpp 
	g++ -c emulator.cpp
clean:
	rm *.o emulator