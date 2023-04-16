# GhoshCord

GhoshCord is a multithreaded terminal user-interface chat application written in C. It communicates via TCP, uses pthreads for multithreading, and the TUI uses ncurses.

## Installation

1. Clone this repository:  
`git clone https://github.com/felixghosh/GhoshCord.git`

2. Enter the directory:  
`cd GhoshCord`

3. If you wish to run your own server, you must define the `SERVER_PORT` macro in `common.h` and forward this port on the network that the server will run on. The default port is 18000. *This is step is not necessary if you only plan on running the client.*
4. Run the Makefile to compile the program:  
`make`

5. The server can be run using:  
`bin/server`

6. Clients connect using:  
`bin/client`

7. Alternatively one can connect directly from the commandline using:
`bin/client <ip_address> <username>`
