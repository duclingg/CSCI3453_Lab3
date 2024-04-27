#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

// Inode structure
struct Inode {
    char name[16];
    int size;
    int blockPointers[8];
    int used;
};

// class for managing file system operations
class MyFileSystem {
private:
    int diskFileDescriptor;
    const int blockSize = 1024;
    const int numBlocks = 127;
    const int numInodes = 16;
    const int superBlockSize = 128;
    const int inodeSize = 56;

public:
    // initialize the file system with a disk
    MyFileSystem(char diskName[16]) {
        diskFileDescriptor = open(diskName, O_RDWR);
        if (diskFileDescriptor == -1) {
            cerr << "Error: Failed to open disk file." << endl;
            exit(1);
        }
    }

    // create a file with a given name and size
    void create(char name[16], int size) {
        // move the file pointer to the beginning of the disk
        lseek(diskFileDescriptor, 0, SEEK_SET);

        // read the free block list from the disk
        char freeBlockList[superBlockSize];
        read(diskFileDescriptor, freeBlockList, superBlockSize);

        // count the number of free blocks available
        int freeBlocks = 0;
        for (int i = 0; i < superBlockSize; ++i) {
            if (freeBlockList[i] == 0) {
                ++freeBlocks;
                if (freeBlocks >= size) {
                    break;
                }
            }
        }

        // check if there are enough free blocks to create the file
        if (freeBlocks < size) {
            cerr << "Error: Not enough free space to create the file." << endl;
            return;
        }

        // find a free inode to store file information
        Inode inode;
        int inodeIndex = -1;
        for (int i = 0; i < numInodes; ++i) {
            lseek(diskFileDescriptor, superBlockSize + i * inodeSize, SEEK_SET);
            read(diskFileDescriptor, &inode, inodeSize);
            if (inode.used == 0) {
                inodeIndex = i;
                break;
            }
        }

        // check if a free inode is available
        if (inodeIndex == -1) {
            cerr << "Error: No free inode available." << endl;
            return;
        }

        // update inode information
        inode.used = 1;
        strcpy(inode.name, name);
        inode.size = size;

        // allocate blocks for the file
        int blocksAllocated = 0;
        for (int i = 0; i < numBlocks; ++i) {
            if (freeBlockList[i] == 0) {
                freeBlockList[i] = 1;
                inode.blockPointers[blocksAllocated++] = i;
                if (blocksAllocated == size) {
                    break;
                }
            }
        }

        // write the updated inode to the disk
        lseek(diskFileDescriptor, superBlockSize + inodeIndex * inodeSize, SEEK_SET);
        write(diskFileDescriptor, &inode, inodeSize);

        // write the updated free block list to the disk
        lseek(diskFileDescriptor, 0, SEEK_SET);
        write(diskFileDescriptor, freeBlockList, superBlockSize);

        //pPrint file creation information
        cout << "create | " << "File: " << name << " | Size: " << inode.size << endl;

        for (int i = 0; i < blocksAllocated; ++i) {
            cout << "       | Allocated block: " << inode.blockPointers[i] << endl;
        }
    }

    // delete a file with a given name
    void del(char name[16]) {
        Inode inode;
        int inodeIndex = -1;
        lseek(diskFileDescriptor, superBlockSize, SEEK_SET);
        for (int i = 0; i < numInodes; ++i) {
            read(diskFileDescriptor, &inode, inodeSize);
            if (inode.used && strcmp(inode.name, name) == 0) {
                inodeIndex = i;
                break;
            }
        }

        // check if the file exists
        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        // read the free block list from the disk
        char freeBlockList[superBlockSize];
        lseek(diskFileDescriptor, 0, SEEK_SET);
        read(diskFileDescriptor, freeBlockList, superBlockSize);

        // free the blocks allocated to the file
        for (int i = 0; i < inode.size; ++i) {
            freeBlockList[inode.blockPointers[i]] = 0;
        }

        // mark the inode as unused
        inode.used = 0;

        // write the updated inode to the disk
        lseek(diskFileDescriptor, superBlockSize + inodeIndex * inodeSize, SEEK_SET);
        write(diskFileDescriptor, &inode, inodeSize);

        // write the updated free block list to the disk
        lseek(diskFileDescriptor, 0, SEEK_SET);
        write(diskFileDescriptor, freeBlockList, superBlockSize);

        // print file deletion information
        cout << "delete | " << "File: " << name << endl;

        for (int i = 0; i < inode.size; ++i) {
            cout << "       | Deallocated block: " << inode.blockPointers[i] << endl;
        }
    }

    // list all files in the file system
    void ls() {
        Inode inode;
        lseek(diskFileDescriptor, superBlockSize, SEEK_SET);
        for (int i = 0; i < numInodes; ++i) {
            read(diskFileDescriptor, &inode, inodeSize);
            if (inode.used) {
                cout << "       | File: " << inode.name << " | Size: " << inode.size << " blocks" << endl;
            }
        }
    }

    // read a block from a file
    void readBlock(char name[16], int blockNum, char buf[1024]) {
        Inode inode;
        int inodeIndex = -1;
        lseek(diskFileDescriptor, superBlockSize, SEEK_SET);
        for (int i = 0; i < numInodes; ++i) {
            read(diskFileDescriptor, &inode, inodeSize);
            if (inode.used && strcmp(inode.name, name) == 0) {
                inodeIndex = i;
                break;
            }
        }

        // check if the file exists
        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        // check if the block number is valid
        if (blockNum >= inode.size) {
            cerr << "Error: Invalid block number." << endl;
            return;
        }

        // read the block from the disk
        int blockAddress = inode.blockPointers[blockNum];
        lseek(diskFileDescriptor, superBlockSize + blockAddress * blockSize, SEEK_SET);
        read(diskFileDescriptor, buf, blockSize);

        // print read operation information
        cout << "read   | " << "File: " << name << " | Block: " << blockNum << endl;
    }

    // write a block to a file
    void writeBlock(char name[16], int blockNum, char buf[1024]) {
        Inode inode;
        int inodeIndex = -1;
        lseek(diskFileDescriptor, superBlockSize, SEEK_SET);
        for (int i = 0; i < numInodes; ++i) {
            read(diskFileDescriptor, &inode, inodeSize);
            if (inode.used && strcmp(inode.name, name) == 0) {
                inodeIndex = i;
                break;
            }
        }

        // check if the file exists
        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        // check if the block number is valid
        if (blockNum >= inode.size) {
            cerr << "Error: Invalid block number." << endl;
            return;
        }

        // write the block to the disk
        int blockAddress = inode.blockPointers[blockNum];
        lseek(diskFileDescriptor, superBlockSize + blockAddress * blockSize, SEEK_SET);
        write(diskFileDescriptor, buf, blockSize);

        // print write operation information
        cout << "write  | " << "File: " << name << " | Block: " << blockNum << endl;
    }

    // close the disk file descriptor
    void close() {
        ::close(diskFileDescriptor);
    }

    // close the disk file descriptor when object is destroyed
    ~MyFileSystem() {
        close();
    }
};

// execute commands from a file
void executeCommandsFromFile(const char* filename, MyFileSystem& fs) {
    ifstream inputFile(filename);
    if (!inputFile) {
        cerr << "Error: Failed to open input file." << endl;
        return;
    }

    string command;
    while (getline(inputFile, command)) {
        char fileName[16];
        int size, blockNum;
        char buf[1024];

        if (command[0] == 'C') { // create
            sscanf(command.c_str(), "C %s %d", fileName, &size);
            fs.create(fileName, size);
            cout << "\n";
        } else if (command[0] == 'D') { // delete
            sscanf(command.c_str(), "D %s", fileName);
            fs.del(fileName);
            cout << "\n";
        } else if (command[0] == 'L') { // list
            cout << "ls     | " << endl;
            fs.ls();
            cout << "\n";
        } else if (command[0] == 'R') { // read
            sscanf(command.c_str(), "R %s %d", fileName, &blockNum);
            char buf[1024];
            fs.readBlock(fileName, blockNum, buf);
            cout << "\n";
        } else if (command[0] == 'W') { // write
            sscanf(command.c_str(), "W %s %d", fileName, &blockNum);
            char buf[1024];
            fs.writeBlock(fileName, blockNum, buf);
            cout << "\n";
        }
    }
}

int main() {
    char diskName[16];
    cout << "Enter the name of the disk: ";
    cin >> diskName;

    char filename[100];
    cout << "Enter the name of the input file: ";
    cin >> filename;

    MyFileSystem fs(diskName);

    executeCommandsFromFile(filename, fs);

    return 0;
}
