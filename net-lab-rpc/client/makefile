CC=clang
DIR=./client
DEBUG= -g -O2
CFLAGS=$(DEBUG) -Wall -c
# 目标文件
TARGET=${DIR}/main.o
# 源文件
SRCS=$(wildcard ${DIR}/*.cpp)

OBJS=$(SRCS:.cpp=.o)

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@  -lcurses

$(OBJS): %.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm *.o