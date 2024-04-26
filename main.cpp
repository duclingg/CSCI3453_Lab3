#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

struct Inode {
    char name[16];
    int size;
    int blockPointers[8];
    int used;
};

class MyFileSystem {
private:
    fstream diskFile;
    const int blockSize = 1024;
    const int numBlocks = 127;
    const int numInodes = 16;
    const int superBlockSize = 128;
    const int inodeSize = 56;

public:
    MyFileSystem(char diskName[16]) {
        diskFile.open(diskName, ios::binary | ios::in | ios::out);
        if (!diskFile) {
            cerr << "Error: Failed to open disk file." << endl;
            exit(1);
        }
    }

    // create a file
    void create(char name[16], int size) {
        // Move the file pointer to the start of the disk file
        diskFile.seekg(0, ios::beg);

        // Read the free block list
        char freeBlockList[superBlockSize];
        diskFile.read(freeBlockList, superBlockSize);

        // Scan the list to find if we have sufficient free blocks
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

        // Find a free inode
        Inode inode;
        int inodeIndex = -1;
        for (int i = 0; i < numInodes; ++i) {
            diskFile.read(reinterpret_cast<char*>(&inode), inodeSize);
            if (inode.used == 0) {
                inodeIndex = i;
                break;
            }
        }

        if (inodeIndex == -1) {
            cerr << "Error: No free inode available." << endl;
            return;
        }

        // Mark inode as used
        inode.used = 1;
        strcpy(inode.name, name);
        inode.size = size;

        // Allocate data blocks
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

        // Write inode back to disk
        diskFile.seekp(superBlockSize + inodeIndex * inodeSize, ios::beg);
        diskFile.write(reinterpret_cast<char*>(&inode), inodeSize);

        // Write updated free block list to disk
        diskFile.seekp(0, ios::beg);
        diskFile.write(freeBlockList, superBlockSize);

        cout << "File created: " << name << endl;
    }

    // delete a file
    void del(char name[16]) {
        // Step 1: Locate inode for the file
        Inode inode;
        int inodeIndex = -1;
        diskFile.seekg(superBlockSize, ios::beg);
        for (int i = 0; i < numInodes; ++i) {
            diskFile.read(reinterpret_cast<char*>(&inode), inodeSize);
            if (inode.used && strcmp(inode.name, name) == 0) {
                inodeIndex = i;
                break;
            }
        }

        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        // Step 2: Free blocks
        char freeBlockList[superBlockSize];
        diskFile.seekg(0, ios::beg);
        diskFile.read(freeBlockList, superBlockSize);

        for (int i = 0; i < inode.size; ++i) {
            freeBlockList[inode.blockPointers[i]] = 0;
        }

        // Step 3: Update superblock and inode
        inode.used = 0;
        diskFile.seekp(superBlockSize + inodeIndex * inodeSize, ios::beg);
        diskFile.write(reinterpret_cast<char*>(&inode), inodeSize);

        diskFile.seekp(0, ios::beg);
        diskFile.write(freeBlockList, superBlockSize);

        cout << "File deleted: " << name << endl;
    }

    // list files
    void ls() {
        Inode inode;
        diskFile.seekg(superBlockSize, ios::beg);
        for (int i = 0; i < numInodes; ++i) {
            diskFile.read(reinterpret_cast<char*>(&inode), inodeSize);
            if (inode.used) {
                cout << "File: " << inode.name << ", Size: " << inode.size << " blocks" << endl;
            }
        }
    }

    // read file
    void read(char name[16], int blockNum, char buf[1024]) {
        // Step 1: Locate inode for the file
        Inode inode;
        int inodeIndex = -1;
        diskFile.seekg(superBlockSize, ios::beg);
        for (int i = 0; i < numInodes; ++i) {
            diskFile.read(reinterpret_cast<char*>(&inode), inodeSize);
            if (inode.used && strcmp(inode.name, name) == 0) {
                inodeIndex = i;
                break;
            }
        }

        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        // Step 2: Read the specified block into the buffer
        if (blockNum >= inode.size) {
            cerr << "Error: Invalid block number." << endl;
            return;
        }

        int blockAddress = inode.blockPointers[blockNum];
        diskFile.seekg(superBlockSize + blockAddress * blockSize, ios::beg);
        diskFile.read(buf, blockSize);

        cout << "Block " << blockNum << " read from file: " << name << endl;
    }

    // write to file
    void write(char name[16], int blockNum, char buf[1024]) {
        // Step 1: Locate inode for the file
        Inode inode;
        int inodeIndex = -1;
        diskFile.seekg(superBlockSize, ios::beg);
        for (int i = 0; i < numInodes; ++i) {
            diskFile.read(reinterpret_cast<char*>(&inode), inodeSize);
            if (inode.used && strcmp(inode.name, name) == 0) {
                inodeIndex = i;
                break;
            }
        }

        if (inodeIndex == -1) {
            cerr << "Error: File not found." << endl;
            return;
        }

        // Step 2: Write data from buffer to the specified block
        if (blockNum >= inode.size) {
            cerr << "Error: Invalid block number." << endl;
            return;
        }

        int blockAddress = inode.blockPointers[blockNum];
        diskFile.seekp(superBlockSize + blockAddress * blockSize, ios::beg);
        diskFile.write(buf, blockSize);

        cout << "Block " << blockNum << " written to file: " << name << endl;
    }

    // close file
    void close() {
        diskFile.close();
    }

    ~MyFileSystem() {
        close();
    }
};

// execute commands from input file
void executeCommandsFromFile(const char* filename, MyFileSystem& fs) {
    ifstream inputFile(filename);
    if (!inputFile) {
        cerr << "Error: Failed to open input file." << endl;
        return;
    }

    string command;
    while (getline(inputFile, command)) {
        // Parse command and its arguments
        // Example: diskName, operation, fileName, size, blockNum, buf
        char diskName[16], fileName[16];
        int size, blockNum;
        char buf[1024];

        if (command[0] == 'C') { // create file
            sscanf(command.c_str(), "C %s %d", fileName, &size);
            fs.create(fileName, size);
        } else if (command[0] == 'D') { // delete file
            sscanf(command.c_str(), "D %s", fileName);
            fs.del(fileName);
        } else if (command[0] == 'L') { // list files
            fs.ls();
        } else if (command[0] == 'R') { // read file
            sscanf(command.c_str(), "R %s %d", fileName, &blockNum);
            char buf[1024];
            fs.read(fileName, blockNum, buf);
        } else if (command[0] == 'W') { // write to file
            sscanf(command.c_str(), "W %s %d", fileName, &blockNum);
            char buf[1024];
            // Assume buf contains some data
            fs.write(fileName, blockNum, buf);
        }
    }
}

int main() {
    char diskName[16] = "mydisk0";
    MyFileSystem fs(diskName);

    executeCommandsFromFile("sample-test.txt", fs);

    return 0;
}
