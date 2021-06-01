CC=gcc
CXX=g++
CXXFLAGS=-I../curl/include/curl -std=c++11 -g
LDFLAGS=-lmysqlclient -lstdc++  -L/usr/lib/x86_64-linux-gnu  -lmosquitto -g
DIRBIN=../bin
CP=cp

OBJS=main.o db.o log.o mqtt.o

all : mqttserver

mqttserver : $(OBJS)
	$(CXX) -o mqttserver $(OBJS) $(LDFLAGS)

install :
	$(CP) ./mqttserver $(DIRBIN)

clean :
	rm -f *.o
	rm -f mqttserver

