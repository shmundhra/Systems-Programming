CC = g++ -std=c++17
source = ObjectRotate.cpp
binary = a.out
threads = 1
axis = Axis1.txt
obj = Object1.txt
theta = 360
output = Output.txt

run	:	$(binary)
	./$(binary) $(threads) Axis/$(axis) Object/$(obj) $(theta) > $(output)

$(binary)	:
	$(CC) $(source) -o $(binary) -fopenmp

compile	:
	$(CC) $(source) -o $(binary) -fopenmp

clean	:
	rm *.o
	clear

remove	:
	rm *.o