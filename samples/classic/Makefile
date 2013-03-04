all: classic inlined_classic classic2 inlined_classic_stress

classic: classic.cpp
	g++ -g -o classic classic.cpp

improved_classic: improved_classic.cpp
	g++ -g -o improved_classic improved_classic.cpp

inlined_classic: classic.cpp
	g++ -g -O2 -o inlined_classic classic.cpp

inlined_classic_stress: inlined_classic_stress.cpp
	g++ -g -O2 -o inlined_classic_stress inlined_classic_stress.cpp

classic2: classic2.cpp classic2.h
	g++ -g -O2 -o classic2 classic2.cpp

clean:
	rm -rf *.o classic inlined_classic classic2 inlined_classic_stress

