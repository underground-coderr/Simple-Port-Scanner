CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -pthread
TARGET   = scanner
SRCDIR   = src

$(TARGET): $(SRCDIR)/main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCDIR)/main.cpp

clean:
	rm -f $(TARGET)