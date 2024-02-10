CPPFLAGS := -std=c++20

all: main.cpp
	$(CXX) main.cpp -o main $(CPPFLAGS);
