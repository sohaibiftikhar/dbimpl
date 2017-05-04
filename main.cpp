//
//  main.cpp
//  dbimpl
//
//  Created by Sohaib Iftikhar on 02/05/17.
//  Copyright © 2017 Sohaib Iftikhar. All rights reserved.
//
#include "ExternalSort.hpp"
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

int main(int argc, const char * argv[]) {
    // cout<<argv[0]<<endl;
//    int fdInput = open("/Users/sohaib/IdeaProjects/dbimpl/dbimpl/data/inputfile", O_RDONLY);
//    int fdOutput = open("/Users/sohaib/IdeaProjects/dbimpl/dbimpl/data/outputfile",
//                        O_CREAT | O_WRONLY,
//                        S_IWRITE | S_IREAD);
//    if (fdInput == -1 || fdOutput == -1) {
//        cerr<<"Could not open input/output file. Check path and permissions. "<<strerror(errno)<<endl;
//        exit(1);
//    }
    ExternalSort sort = ExternalSort();
//    sort.externalSort(fdInput, 10, fdOutput, 128);
//    close(fdInput);
//    close(fdOutput);
    int fdVerify = open("/Users/sohaib/IdeaProjects/dbimpl/dbimpl/data/outputfile", O_RDONLY);
    cout<<"Verfying that output is sorted..."<<endl;
    if (sort.isFileSorted(fdVerify)) {
        cout<<"File was successfully verified as sorted"<<endl;
    } else {
        cout<<"File is not sorted."<<endl;
    }
    close(fdVerify);
    cout<<"Finished"<<endl;
    return 0;
}
