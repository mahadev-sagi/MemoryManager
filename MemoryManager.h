#pragma once
#include<fstream>
#include<iostream> 
#include<sstream> 
#include<vector> 
#include<string> 
#include<iomanip> 
#include<algorithm> 
#include<list> 
#include<functional>
#include<cstdint>

struct Hole{

    unsigned int offset; 
    unsigned int length;
};

class MemoryManager{

    unsigned int wordSize; 
    size_t sizeInWords;
    uint8_t* memoryBlock; 
    std::list<Hole> holes;  
    std::function<int(int,void*)> allocator; 
public: 
    MemoryManager(unsigned wordSize, std::function<int(int,void*)> allocator);
    ~MemoryManager(); 
    void initialize(size_t sizeInWords);
    void shutdown(); 
    void *allocate(size_t sizeInBytes); 
    void free(void* address); 
    void setAllocator(std::function<int(int,void*)> allocator);
    int dumpMemoryMap(char* filename); 
    void *getList(); 
    void *getBitmap(); 
    unsigned getWordSize(); 
    void *getMemoryStart(); 
    unsigned getMemoryLimit(); 



};


int bestFit(int sizeInWords, void* list);
int worstFit(int sizeInWords, void* list);