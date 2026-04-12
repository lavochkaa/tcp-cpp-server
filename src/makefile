CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall

TARGET = server
SOURCES = $(wildcard *.cpp)

all:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)