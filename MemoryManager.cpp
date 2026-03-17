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
    delete[] memoryBlock; //delete the whole memory block and set to nullptr for edge case testing
    memoryBlock = nullptr;
    holes.clear(); // clear out the holes list 
    sizeInWords = 0; // set the size to 0 for now
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

void MemoryManager::setAllocator(std::function<int(int, void*)> allocator) {
this->allocator = allocator;

}

int MemoryManager::dumpMemoryMap(char* filename) {
    if(memoryBlock == nullptr){
        return -1;  // edgecase check to make sure memoryBlock was initilaized
    }
   int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777); 
    if(fd == -1){
     return -1;  // file didn't open
    }
    uint16_t* holeList = (uint16_t*)getList();
    int count = holeList[0];
    for(int i = 0; i < count; i++){
        string line = "[" + to_string(holeList[1 + 2 * i]) + ", " 
        + to_string(holeList[2 + 2 * i]) + "]" ;
        if( i < count -1){
            line += " - "; // add separator if not the last hole
        } 
        write(fd, line.c_str(), line.length());
    }
    close(fd);
    delete[] holeList;
    return 0; 
}


void* MemoryManager::getList(){
    if(memoryBlock == nullptr){
        return nullptr; // if no memory is allocated return nullptr 
    }
    uint16_t* list = new uint16_t[(holes.size() * 2) +1]; // +1 for count
    list[0] = holes.size(); // order of elements count, offset 1, length 1...
    int indexCounter = 1; // counter to start index at second position of list 
    for(auto it  = holes.begin(); it != holes.end(); it++){
        list[indexCounter] = it->offset;
        list[indexCounter + 1] = it->length;
        indexCounter += 2; // increment by 2 to move to the next offset and length pair
    }
    return list; 

}

void* MemoryManager::getBitmap(){
    if(memoryBlock == nullptr){
        return nullptr; // if no memory is allocated return nullptr 
    }
    size_t byteSize = (sizeInWords + 7) / 8; // calculate the number of bytes needed for the bitmap
    uint8_t* bitmap = new uint8_t[byteSize + 2];
    for(size_t i = 0; i < sizeInWords; i++){
        bool hole = false; 
        for(auto it = holes.begin(); it != holes.end(); it++){
            if(it->offset && i < it->offset + it->length){
                hole = true; 
                break; 
            }
        }
        if(!hole){
            bitmap[2 + (i / 8)] |= (1 << (i % 8)); // set the bit to 1 if it is allocated
        }
    }
    bitmap[0] = byteSize & 0xFF; //store the lowbyte 
    bitmap[1] = (byteSize >> 8) & 0xFF; // store the highbyte since it is little endian
    return bitmap;


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




int bestFit(int sizeInWords, void* list){
    if(list == nullptr) {
        return -1; // edge case to check list isn't null
    }
    uint16_t* holeList = (uint16_t*)list;
    int count = holeList[0]; // first position is count of holes
    if(count == 0) {
        return -1;
    }
    int bestIndex = -1; 
    for(int i = 0; i < count; i++){
        int length = holeList[2 + 2 * i]; // ofset, then length 
        if(length >= sizeInWords){
            if(bestIndex == -1 || length < holeList[2 + 2*bestIndex]){
                bestIndex = i; // update best index if it is the first hole found or if it is smaller than the current best hole
            }
        }
    }   
    if(bestIndex == -1) { // if no hole big enough is found, return -1
        return bestIndex;}
    return holeList[1 + 2*bestIndex]; // return offset
}

int worstFit(int sizeInWords, void* list){ // should find the hole that is largest
    if(list == nullptr){
        return -1; // make sure if not initliaized to return -1
    }
    uint16_t* holeList = (uint16_t*)list;
    int count = holeList[0];
    if(count == 0){ // no holes available
        return -1; 
    }
    int worstIndex = -1;
    for(int i = 0; i < count; i++){
        int length = holeList[2 +2 * i]; 
        if(length >= sizeInWords){
            if(worstIndex == -1 || length > holeList[2 + 2*worstIndex]){
                worstIndex = i; // update worst index if it is the first hole found or if it is larger than the current worst hole
            }
        }
    }
    if(worstIndex == -1){ // if no hole big enough is found, return -1
        return worstIndex;
    }
    return holeList[1 + 2*worstIndex]; // return offset

} 
