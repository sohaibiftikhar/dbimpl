//
//  ExternalSort.cpp
//  dbimpl
//
//  Created by Sohaib Iftikhar on 02/05/17.
//  Copyright © 2017 Sohaib Iftikhar. All rights reserved.
//

#include "ExternalSort.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <queue>

using namespace std;

class RandomLong
{
    /// The state
    uint64_t state;
    
public:
    /// Constructor
    explicit RandomLong(uint64_t seed=88172645463325252ull) : state(seed) {}
    
    /// Get the next value
    uint64_t next() { state^=(state<<13); state^=(state>>7); return (state^=(state<<17)); }
};


void ExternalSort::kMerge(int k, int noFiles, uint64_t memSize, int fdOutput) {
    // The last is for the output buffer which also needs memory
    ssize_t szEachBuffer = memSize/(k+1);
    auto cmp = [](MergeBundle* m1, MergeBundle* m2) { return m1->cmp(m2); };
    // Use lambda comparator inside.
    priority_queue<MergeBundle*, std::vector<MergeBundle*>, decltype(cmp)> out(cmp);
    int itr = 0;
    while(noFiles > 0) {
        string baseFileName = "/tmp/dbimpl_" + to_string(runId) + "_" + to_string(itr);
        string baseOutfile = "/tmp/dbimpl_" + to_string(runId) + "_" + to_string(itr+1); // input for the next iteration
        for (int fileNo = 0; fileNo < noFiles; fileNo+=k) {
            string outFileName = baseOutfile + "_" + to_string(fileNo/k);
            int fdIntOut;
            if (noFiles>k) {
                fdIntOut = open(outFileName.c_str(),
                                O_CREAT | O_APPEND | O_WRONLY,
                                S_IWRITE | S_IREAD);
            } else { // This is the last file
                fdIntOut = fdOutput;
            }
            // Pick a batch of k files and merge it to one
            for (int i=0; i<k && (fileNo + i) < noFiles; i++) {
                out.push(new MergeBundle(baseFileName, szEachBuffer, fileNo + i));
            }
            do {
                MergeBundle *b = out.top();
                out.pop();
                uint64_t key = b->next();
                if (b->currentPos != -1) { // Still some elements in this queue
                    // cout<< "Writing in itr "<<itr<<" file no " <<fileNo<<": " <<key<<" to "<<outFileName<<endl;
                    ssize_t written = write(fdIntOut, &key, sizeof(uint64_t));
                    if (written < 0) {
                        cerr<<"Error writing to file. "<<strerror(errno)<<endl;
                    }
                    out.push(b);
                } else { // Else cleanup last element
                    delete b;
                }
            } while (out.size() > 0);
            if (noFiles>k) { // Close only if we created this file
                close(fdIntOut);
            }
        }
        noFiles=(noFiles/k) + (( (noFiles>k) && ((noFiles%k) != 0)) ? 1 : 0); // After k way merging
        cout<<"Finished iteration "<<itr<<". Generated "<<noFiles<< " intermediate files."<<endl;
        // We can remove the intermediate files from the previous iteration here.
        string command = "rm " + baseFileName + "*";
        if (system((command).c_str())) {
            cerr<<"Unable to cleanup files for iteration "<<itr<<". "<<strerror(errno)<<endl
                <<"You may need to clean these yourself using "<<command<<endl;
            exit(255);
        }
        itr++;
    }
}

void ExternalSort::externalSort(int fdInput, uint64_t size, int fdOutput, uint64_t memSize) {
    // Memory size must be multiple of 8 since each int is 8 bytes
    if (memSize <= 0 || memSize % 8 != 0) {
        cerr<<"Memory size must be non zero and positive multiple of 8"<<endl;
        exit(1);
    }
    runId = RandomLong((uint)(time(NULL))).next();
    cout<<"RunId: "<<runId<<endl;
    int k = 10; // Controls the number of tapes/files directly written to
    size_t bufSize = memSize/8;
    uint64_t buf[bufSize];
    ssize_t cnt;
    int fileNo = 0;
    while( (cnt=read(fdInput, (char*)buf, sizeof(buf))) > 0) {
        // sort inmem read
        sort(buf, buf + cnt/8);
        const string outputFile = "/tmp/dbimpl_" + to_string(runId) + "_0_" + to_string(fileNo++);
        int fOut = open(outputFile.c_str(), O_CREAT | O_APPEND | O_WRONLY, S_IWRITE | S_IREAD);
        if (fOut == -1) {
            cerr<<"Could not open temp file: " + outputFile<<endl<<strerror(errno)<<endl;
            exit(1);
        }
        // cout<<"writing: "<<fOut<<endl;
        write(fOut, (char*)buf, cnt);
        close(fOut);
    }
    kMerge(k, fileNo, memSize, fdOutput);
}

bool ExternalSort::isFileSorted(int fd) {
    uint64_t prev = 0;
    ssize_t readInts = 0;
    uint64_t *buf= new uint64_t[64];
    while((readInts = read(fd, (char*)buf, 64*sizeof(buf))) > 0) {
        readInts/=8;
        for (int i=0; i<readInts; i++) {
            if (prev > buf[i]) {
                return false;
            }
            prev = buf[i];
        }
    }
    delete[] buf;
    return true;
}
