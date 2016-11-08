test_prg: main.cpp libmap.a goldchase.h
	g++ main.cpp -o test_prg -L. -lmap -lpanel -lncurses -pthread -lrt

libmap.a: Screen.o Map.o
	ar -r libmap.a Screen.o Map.o

Screen.o: Screen.cpp
	g++ -c Screen.cpp

Map.o: Map.cpp
	g++ -c Map.cpp

clean:
	rm -f Screen.o Map.o libmap.a test_prg
