# README

## Lab 3: A Simple Unix-like File System

**Name:** Justin Hoang  
**StudentID:** 110069730  
**Class:** CSCI 3453  
**Due Date:** May 6, 2024  

## Description of Program
### Simple File System Implementation

#### Goal
This programming assignment aims to enhance understanding of file systems by implementing a Unix-like simple file system with specific constraints and operations.
- The file system is stored on a 128KB disk.
- Only one root directory is allowed without subdirectories.
- Supports a maximum of 16 files.
- Maximum file size is 8 blocks (1KB each).
- Each file has a unique name (max 15 characters) and associated metadata.

#### Specifications
- 128KB disk file as the "disk" for the file system.
- Disk blocks and inodes initialized to be free.
- File system maintains persistence across program shutdowns and restarts.
- Program should takes commands from an input file and perform specified actions while printing results.
- Input file format includes commands for creating, deleting, reading, writing files, and listing files:

#### Superblock Structure
- First 1KB block stores the free block list and index nodes (inode) for each file.
- Free block list indicates availability of disk blocks.
- 16 inodes store file metadata.
- Each inode includes file name, size, block pointers, and usage status.

#### System Operations
- `create(char name[16], int size)`: Create a new file with specified name and size.
- `delete(char name[16])`: Delete the file with specified name.
- `read(char name[16], int blockNum, char buf[1024])`: Read specified block from file into buffer.
- `write(char name[16], int blockNum, char buf[1024])`: Write data from buffer to specified block in file.
- `ls(void)`: List names and sizes of all files in the file system.
  
#### Input File Format
`diskName`: name of the file that emulates the disk   
`C fileName Size`: create a file of this size  
`D fileName`: delete this file  
`L`: list all files on disk and their sizes  
`R fileName blockNum`: read this block from this file  
`W fileName blockNum`: write to this block in the file (use a dummy 1KB buffer)  

## Program Status
The program is working as intended. Compiles and runs on csegrid.

## Source Files
`main.cpp`  
`make-disk`  
`Makefile`  

## How to Build and Run Program
1. Create a disk using the exectuable via the command `./make-disk <disk name>` in the terminal.
- Note this executable only runs in CSE Grid.
2. Next build the program using the **Makefile** via the command `make`
3. Run the executable for the program using `./main`
4. The program will begin to run.
- Enter the disk image you want to use (the one you just created)
- Enter the input `.txt` file of operations that you want to execute.