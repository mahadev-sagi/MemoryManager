#include "MemoryManager.h"
using namespace std;



MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator) {
    this->wordSize = wordSize;
    this->allocator = allocator;
    memoryBlock = nullptr; 
    sizeInWords = 0;
    holes.clear();
}



MemoryManager::~MemoryManager() {
    shutdown(); 
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
    if(memoryBlock == nullptr){
        return nullptr; 
    }
    if(sizeInBytes == 0){
        return nullptr; 
    }
    size_t allocSize = (size_t)ceil((double)sizeInBytes / wordSize);
    uint16_t* list = (uint16_t*)getList();
    int offset  = allocator(allocSize , list); 
    if(offset == -1){
        delete[] list;
        return nullptr; 
    }
    for(auto it = holes.begin(); it != holes.end(); it++){
        if(it->offset == offset){
            if(it->length == allocSize){
                holes.erase(it);
                break; 
            }else{
                it->length -= allocSize;
                it->offset += allocSize;
                break; 
            }
            }   
        }
    delete[] list;
    return memoryBlock + (offset * wordSize); 
    }


void MemoryManager::free(void* address) {
    if(memoryBlock == nullptr){ //edge case check to make sure memoryBlock was initilaized
        return; 
    }
    uint8_t* addressPtr = (uint8_t*)address;
    size_t offset = (addressPtr - memoryBlock) / wordSize;
    auto placeIt = holes.end(); 
    for(auto it = holes.begin(); it != holes.end(); it++){
        if(it->offset > offset){
            size_t blockSize = it->offset - offset; 
            placeIt = holes.insert(it, {offset, blockSize}); // insert before the first hole with offset greater than the freed block
            break; 
        }
    }
      if(placeIt == holes.end()){
            size_t blockSize = sizeInWords - offset; 
            holes.push_back({offset, blockSize}); // insert to the end if no hole after
            placeIt = prev(holes.end()); // make sure to move the iterator 
            
        }
        //merge with neighbors if needed 
        auto nextIt = next(placeIt);
        if(nextIt != holes.end()){
            if(nextIt->offset == placeIt->offset + placeIt->length){
                placeIt->length += nextIt->length;
                holes.erase(nextIt);
            }
        
        }
        if(placeIt != holes.begin()){
        auto prevIt = prev(placeIt);
        if(prevIt->offset + prevIt->length == placeIt->offset){
            prevIt->length += placeIt->length;
            holes.erase(placeIt);
        }
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