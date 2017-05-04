//
//  ExternalSort.hpp
//  dbimpl
//
//  Created by Sohaib Iftikhar on 02/05/17.
//  Copyright © 2017 Sohaib Iftikhar. All rights reserved.
//

#ifndef ExternalSort_hpp
#define ExternalSort_hpp

#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <algorithm>

#endif /* ExternalSort_hpp */

using namespace std;

class ExternalSort {
    
private:
    
    /**
     * Private variables declarations
     **/
    
    uint64_t runId;
    
    /**
     * Private functions declarations
     **/
    
    void kMerge(int k, int NoFiles, uint64_t memSize, int fdOutput);
    
    /**
     * Private class declarations
     **/
    
    class MergeBundle {
    public:
        uint64_t *buf;
        int fd = -1;
        int currentPos = -1;
        ssize_t readInts = -1;
        ssize_t szEachBuffer = -1;
        
        MergeBundle(string baseName, ssize_t szEachBuffer, int fileNo) {
            // cout<<"constructor called"<<endl;
            this->szEachBuffer = szEachBuffer;
            buf = new uint64_t[szEachBuffer];
            string fileName = baseName + "_" + to_string(fileNo);
            if ((fd = open(fileName.c_str(), O_RDONLY)) == -1) {
                cerr<<"Error opening file "<<fileName<<": "<<strerror(errno)<<endl;
                exit(1);
            }
            fillBuffer();
        }
        
        void fillBuffer() {
            readInts = read(fd, (char*)buf, szEachBuffer * 8);
            // Divide by 8 since each int is 8 bytes
            readInts/=8;
            if (readInts > 0) {
                currentPos = 0;
            } else {
                clean();
                // cout<<"closed";
            }
        }
        
        int cmp(MergeBundle* m2) {
            uint64_t a = get();
            uint64_t b = m2->get();
            // Evaluates to true if the element is larger
            if (m2->currentPos == -1) {
                // If the second buffer is finished it is the largest and should be expelled first
                return false;
            } else if (currentPos == -1) {
                // If the first buffer is finished it is the largest and should be expelled
                return true;
            } else {
                return a > b;
            }
        }
        
        /**
         * This seems like bad practice but works in this case
         **/
        void clean() {
            readInts = -1;
            currentPos = -1;
            buf = NULL;
            delete[] buf;
            close(fd);
        }
        
        uint64_t get() {
            if(currentPos < readInts) {
                return buf[currentPos];
            } else {
                if (readInts == szEachBuffer) {
                    fillBuffer();
                    return get();
                } else {
                    clean();
                    return 0; // This file is done
                }
            }
        }
        
        
        uint64_t next() {
            uint64_t currElem = get();
            if (currentPos != -1) currentPos++;
            return currElem;
        }
        
        ~MergeBundle() {
            // This is a bad way to write it. Need to learn how to write clean destructors.
        }
    };
    
public:
    
    
    void externalSort(int fdInput, uint64_t size, int fdOutput, uint64_t memSize);
    
    bool isFileSorted(int fd);
};
