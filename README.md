# Virtual File System

#### Description:
This project is the implementation of a file system with a single level derectory organized in a file on a disk. A directory item is a file description containing at least a name, size and arrangement of the file on the virtual disk. 

The following operations available to the user of the program were implemented:

- creating a virtual disk,

- copying a file from the system disk to a virtual disk,

- copying a file from a virtual disk to the system disk,

- displaying the virtual disk directory,

- deleting a file from a virtual disk,

- removing a virtual disk,

- displaying a summary with the current virtual disk occupancy map - a list of subsequent areas of the virtual disk with the description: address, area type, size, state (e.g. for data blocks: free / busy)

The program controls the amount of available space on the virtual disk and directory capacity, react to attempts to exceed these sizes.

Sequential and exclusive access to the virtual disk is assumed.

In the *test.sh* file there is preparad a demonstration (grouping a series of commands in the form of a sh script interpreter) presenting the strengths and weaknesses of the adopted solution in the context of possible external and internal fragmentation.
