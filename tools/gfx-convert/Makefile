CXX = g++
CXXFLAGS = -std=c++17 $(shell pkg-config --cflags libegt) \
    -I../../src/detail/ -I../../external/cxxopts/include/ \
    -I../../external/rapidxml/ -Wall

LDFLAGS = -L../../src/.libs -L/usr/local/lib
LIBS = -legt -lcairo

all: gfx-convert

gfx-convert: gfx-convert.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f gfx-convert *eraw.*
