test_prg: main.cpp libmap.a goldchase.h
	g++ --std=c++11 -g main.cpp -o test_prg -L. -lmap -lpanel -lncurses -lrt -lpthread

libmap.a: Screen.o Map.o daemon.o
	ar -r libmap.a Screen.o Map.o daemon.o

Screen.o: Screen.cpp
	g++ --std=c++11 -c Screen.cpp

Map.o: Map.cpp
	g++ --std=c++11 -c Map.cpp

daemon.o: daemon.cpp
	g++ --std=c++11 -c daemon.cpp

clean:
	rm -f daemon.o Screen.o Map.o libmap.a test_prg goldchase
