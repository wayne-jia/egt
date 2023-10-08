CXX = g++
CXXFLAGS = -std=c++17 $(shell pkg-config --cflags libegt) \
    -I../../src/detail/ -I../../external/cxxopts/include/ \
    -I../../external/rapidxml/ -Wall

LDFLAGS = -L../../src/.libs -L/usr/local/lib
LIBS = -legt -lcairo

all: gfxconvert

gfxconvert: gfxconvert.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f gfxconvert eraw.*
