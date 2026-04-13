CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -I.

TARGET = server
SOURCES = $(shell find . -name '*.cpp')

all:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
