CXX=g++-7
CXXFLAGS=-std=c++11 -Wall -O2 -MMD -MP
TARGET=ffsearch

SRC=main.cpp ffsearch.cpp

all: $(TARGET)

$(TARGET): $(SRC:.cpp=.o)
	$(CXX) $(CXXFLAGS) $+ -o $@

-include $(SRC:.cpp=.d)

clean:
	rm *.o *.d $(TARGET)


