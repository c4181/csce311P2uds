GPP = g++ -Wall -std=c++11

A = main.o

a: $A
	$(GPP) -o a.out $A

main.o: main.cpp
	$(GPP) -c main.cpp

clean: 
	rm *.o a.out
