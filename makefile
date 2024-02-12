CPPFLAGS := -std=c++20 -Wno-format -g

all: main.cpp
	$(CXX) main.cpp -o main $(CPPFLAGS);
