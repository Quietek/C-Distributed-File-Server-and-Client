NOTE: this code works but is very buggy likely due to the way FORK is utilized for handling multiple clients, crashes are common and expected.

This is a basic distributed file system, where parts of the files are distributed across 4 file servers, where any one file server can be down after pushing the file to all 4 and still allow for reconstruction of the file. 

Additionally the md5hash of each file is used to determine how the files are distributed.

DFC.conf contains information on the clients connections with the default values being specified as localhost on ports 10001 through 10004. Also included is a plaintext username and password that the servers use for authentication and user separation.

DFS.conf  contains information on accepted username and password combinations in plaintext.

Usage: make in the directory you wish to compile both the server and client programs.

./DFC <config filename> - this runs the distributed file system client using the options specified via the configuration file you specify, should be DFC.conf by default.

./DFS <Directory> <port> - this runs the distributed file system server on the specified port where files are stored in the specified directory.

This client/server combo supports pushing files to the distributed server via the put <filename> command, listing files available on the server with incomplete files noted via the list command, and getting files from the distributed file server via the get <filename> command. 