#include "MemoryManager.h"
#include<iostream>
using namespace std;



MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator) {
    this->wordSize = wordSize;
    this->allocator = allocator;
    memoryBlock = nullptr; 
    sizeInWords = 0;
    holes.clear();
}



MemoryManager::~MemoryManager() {
    delete[] memoryBlock;
    memoryBlock = nullptr;
    holes.clear(); 
    allocator = nullptr;
}


void MemoryManager::initialize(size_t sizeInWords) {
     if(sizeInWords > 65535){
        return; 
    }
    if(memoryBlock != nullptr){
        shutdown(); 
    }
    this->sizeInWords = sizeInWords;
    memoryBlock = new uint8_t[sizeInWords * wordSize];
    holes.push_back({0,sizeInWords});


}

void MemoryManager::shutdown() {
    delete[] memoryBlock;
    memoryBlock = nullptr;
    holes.clear();
    sizeInWords = 0;
}


void* MemoryManager::allocate(size_t sizeInBytes) {
    if (allocator == nullptr) {
        return nullptr; 
    }
    


}

unsigned MemoryManager::getWordSize() {
return wordSize;
}

void* MemoryManager::getMemoryStart() {
return memoryBlock;
}

unsigned MemoryManager::getMemoryLimit() {
return sizeInWords * wordSize;
}

void MemoryManager::setAllocator(std::function<int(int, void*)> allocator) {
this->allocator = allocator;

}