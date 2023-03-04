# Interprocess Communication

File system as IPC channels


## Pipes

* unidirectonal
* Used between process belonging to common ancestor

read when writer is closed - EOF  
write when reader is closed - SIGPIPE

> See pipe.c


## FIFO

Named pipes  

* Unrelated process can exchange data  
* Operates same as file path exists!  

Normal :  
    Read when no writer - BLOCK  
    Write when no reader - BLOCK  

NONBLOCK :  
    Read when no writer - return immediately  
    Write when no reader - -1 , ENXIO  

> See user1 and user2 fifo programs


# Sending and Receiving Message formats

HTTP - Application Layer protocol 
Has its own message format

* Serialization ?
    Loss of information for real numbers
* Send in binary format