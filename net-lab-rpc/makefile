CC=clang
DEBUG= -g -O2
CFLAGS=$(DEBUG) -Wall -c
# 目标文件
TARGET=rpc.o
# 源文件
SRCS=$(wildcard *.cpp)

OBJS=$(SRCS:.cpp=.o)

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@

$(OBJS): %.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm *.o