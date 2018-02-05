Systems Programming - Project 3 - Mirroring System

1. Introduction
    1.1. Compilation and execution example
            Compilation:
                make cs/ms/mi/all (Default: all)
            Execution: 
                mirror-serv -m output -w <num_workers> -p <port> (-s) </br>
                content-serv -d (relative)(/_or_absolue)/path/to/dir/or/file -w <num_workers> -p <port> </br>
                mirror-init -n <address> -p <port> -s <ContentServerAddress1>:<ContentServerPort1>:<Directory or filename1><Delay1> ... </br>
                Use <executable> --help for a list of all command line options

2. Implementation
    2.1. Content Server:
        Upon execution, each Content Server creates <N> worker threads. Communication with the Master thread is performed through a queue, whose synchronisation is ensured by a number of mutexes
        The Master thread waits for incoming connections while the worker threads await the insertion of an element in the work queue.
        Once a request arrives, its socket file descriptor is inserted into the work queue and the Master thread returns to its waiting state.
        When an element is inserted into the work queue, one of the worker threads responds to the request (see below)

    2.2. File transfer procedure:
        2.2.1.
            Upon execution, each Mirror Server creates <N> worker threads, each of which sends a LIST request toe the Content Server that has been assigned to it. When it receives the response, it checks if the path to each file matches the 
        path requested by the user (this check depends on the -s flag. Please see [mirror_manager.cpp::mirror_manager](mirror-server/source/mirror_manager.cpp) for more information).
        If such a match is made, the file, along with the address port of the Content Server and its produced ID, is inserted into the list with the following format:
            path/to/file:address:port:ID
        Once an element is inserted into the list, a worker thread is notified, which, in turn, removes it from the list and sends a FETCH request to address:port along with the ID that the Mirror Manager has produced.
        Lastly, it reads the bytes of the file from the Content Server and saves the file in a local directory. (for more information, please see [2.2.3. File Transfer](#file_transfer)).
        2.2.2. File name transfer:
            When a LIST request is received by a Content Server, it is inserted into a work queue, and each element of that work queue is assigned to a worker thread.
            Each thread, after receiving the request, recursively check the directory and sub-directories that have been assigned to the Content Server, splits each file's name into blocks and sends them back to the Mirror Initiator that 
            made the request. After that, the connection is closed and the worker thread returns to its waiting state.
        2.2.3. File transfer:
            When a FETCH request is received by a Content Server worker thread, it reads the delay according to the received ID, sleeps for that amount of time and then begins the actual file transfer:
            Initially, the file is opened in binary mode, its size is calculated and the file is split into (1024 byte) blocks, which are then iteratively sent toe the Mirror Server. After this process has been completed,
            the connection is closed and the worker thread returns to it waiting state.
            
