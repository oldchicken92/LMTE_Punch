punch: src/*.cpp src/*.h
	g++ -std=c++17 -Wall -Wextra src/*.cpp -o punch

clean:
	rm -f punch

	