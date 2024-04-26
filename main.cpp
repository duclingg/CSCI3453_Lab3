#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

struct Inode {
    char name[16];
    int size;
    int blockPointers[8];
    int used;
};

class MyFileSystem {
private:
    int diskFileDescriptor;
    const int blockSize = 1024;
    const int numBlocks = 127;
    const int numInodes = 16;
    const int superBlockSize = 128;
    const int inodeSize = 56;

public:
    MyFileSystem(char diskName[16]) {
        diskFileDescriptor = open(diskName, O_RDWR);
        if (diskFileDescriptor == -1) {
            cerr << "Error: Failed to open disk file." << endl;
            exit(1);
        }
    }

    void create(char name[16], int size) {
        lseek(diskFileDescriptor, 0, SEEK_SET);

        char freeBlockList[superBlockSize];
        read(diskFileDescriptor, freeBlockList, superBlockSize);

        int freeBlocks = 0;
        for (int i = 0; i < superBlockSize; ++i) {
            if (freeBlockList[i] == 0) {
                ++freeBlocks;
                if (freeBlocks >= size) {
                    break;
                }
            }
        }

        if (freeBlocks < size) {
            cerr << "Error: Not enough free space to create the file." << endl;
            return;
        }

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

        if (inodeIndex == -1) {
            cerr << "Error: No free inode available." << endl;
            return;
        }

        inode.used = 1;
        strcpy(inode.name, name);
        inode.size = size;

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

        lseek(diskFileDescriptor, superBlockSize + inodeIndex * inodeSize, SEEK_SET);
        write(diskFileDescriptor, &inode, inodeSize);

        lseek(diskFileDescriptor, 0, SEEK_SET);
        write(diskFileDescriptor, freeBlockList, superBlockSize);

        cout << "create | " << "File: " << name << " | Size: " << inode.size << endl;

        // print block allocations
        for (int i = 0; i < blocksAllocated; ++i) {
            cout << "       | Allocated block: " << inode.blockPointers[i] << endl;
        }
    }

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

        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        char freeBlockList[superBlockSize];
        lseek(diskFileDescriptor, 0, SEEK_SET);
        read(diskFileDescriptor, freeBlockList, superBlockSize);

        for (int i = 0; i < inode.size; ++i) {
            freeBlockList[inode.blockPointers[i]] = 0;
        }

        inode.used = 0;
        lseek(diskFileDescriptor, superBlockSize + inodeIndex * inodeSize, SEEK_SET);
        write(diskFileDescriptor, &inode, inodeSize);

        lseek(diskFileDescriptor, 0, SEEK_SET);
        write(diskFileDescriptor, freeBlockList, superBlockSize);

        cout << "delete | " << "File: " << name << endl;

        // Print block deallocations
        for (int i = 0; i < inode.size; ++i) {
            cout << "       | Deallocated block: " << inode.blockPointers[i] << endl;
        }
    }

    void ls() {
        Inode inode;
        lseek(diskFileDescriptor, superBlockSize, SEEK_SET);
        for (int i = 0; i < numInodes; ++i) {
            read(diskFileDescriptor, &inode, inodeSize);
            if (inode.used) {
                cout << "ls     | " << "File: " << inode.name << " | Size: " << inode.size << " blocks" << endl;
            }
        }
    }

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

        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        if (blockNum >= inode.size) {
            cerr << "Error: Invalid block number." << endl;
            return;
        }

        int blockAddress = inode.blockPointers[blockNum];
        lseek(diskFileDescriptor, superBlockSize + blockAddress * blockSize, SEEK_SET);
        read(diskFileDescriptor, buf, blockSize);

        cout << "read   | " << "File: " << name << " | Block: " << blockNum << endl;
    }

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

        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        if (blockNum >= inode.size) {
            cerr << "Error: Invalid block number." << endl;
            return;
        }

        int blockAddress = inode.blockPointers[blockNum];
        lseek(diskFileDescriptor, superBlockSize + blockAddress * blockSize, SEEK_SET);
        write(diskFileDescriptor, buf, blockSize);

        cout << "write  | " << "File: " << name << " | Block: " << blockNum << endl;
    }

    void close() {
        ::close(diskFileDescriptor);
    }

    ~MyFileSystem() {
        close();
    }
};

void executeCommandsFromFile(const char* filename, MyFileSystem& fs) {
    ifstream inputFile(filename);
    if (!inputFile) {
        cerr << "Error: Failed to open input file." << endl;
        return;
    }

    string command;
    while (getline(inputFile, command)) {
        char diskName[16], fileName[16];
        int size, blockNum;
        char buf[1024];

        if (command[0] == 'C') {
            sscanf(command.c_str(), "C %s %d", fileName, &size);
            fs.create(fileName, size);
            cout << "\n";
        } else if (command[0] == 'D') {
            sscanf(command.c_str(), "D %s", fileName);
            fs.del(fileName);
            cout << "\n";
        } else if (command[0] == 'L') {
            fs.ls();
            cout << "\n";
        } else if (command[0] == 'R') {
            sscanf(command.c_str(), "R %s %d", fileName, &blockNum);
            char buf[1024];
            fs.readBlock(fileName, blockNum, buf);
            cout << "\n";
        } else if (command[0] == 'W') {
            sscanf(command.c_str(), "W %s %d", fileName, &blockNum);
            char buf[1024];
            fs.writeBlock(fileName, blockNum, buf);
            cout << "\n";
        }
    }
}

int main() {
    char diskName[16] = "mydisk0";
    MyFileSystem fs(diskName);

    executeCommandsFromFile("sample-test.txt", fs);

    return 0;
}
