# net-lab-cpp
Computer network curriculum projects (analyzer &amp; rpc) based on cpp

## Requires
* net-lab-analyzer requires: Windows 10, Visual Studio 2017, WinPcap 4.1.3, MinGW (installed in C:/MinGW/), WinPcap 4.1.2 Developer's Pack (installed in C:/WpdPack/), etc.
* net-lab-rpc requires: Unix (macOS/Linux), clang.

## Usage
* net-lab-analyzer: open this project in VS2017 or complie it into *.exe by VS and run.
* net-lab-rpc: change directory to this project path in terminal:

1. compile: ``make -f ./server/makefile`` for server and ``make -f ./client/makefile`` for client.
2. run: ``./server/main.o`` for server and ``./client/main.o``.
3. clean: ``make clean`` in ``server`` or ``client`` folder for clean the output files if you want.

## Todos
### 1. net-lab-analyzer
- [x] scrap and analyze ARP/TCP/ICMP packets.
- [x] send ARP/TCP/ICMP packets.
- [x] scrap reply packets after sending.

### 2. net-lab-rpc
- [x] a simple shell based on C++ with built-in commands like cd, exit, etc.
- [x] devide the shell into server and client based on socket.
- [x] 1-n server/client structure.
- [x] test the project on real server.
- [ ] Chinese out-put.