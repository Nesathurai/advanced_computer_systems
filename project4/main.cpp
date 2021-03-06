#include <iostream>       // For I/O
#include <fstream>        // For I/O
#include <iomanip>        // For setw
#include <stdio.h>        // std
#include <stdlib.h>       // std
#include <ctime>          // Timekeeping
#include <ratio>          // Timekeeping
#include <chrono>         // Timekeeping
#include <vector>         // For vectors
#include <cmath>          // For floor
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>
#include <cstddef>         // std::size_t
#include <unordered_map>
#include <algorithm>
#include <pthread.h>


//namespace fs = std::filesystem;
using std::chrono::high_resolution_clock;
using std::chrono::time_point;
using std::chrono::duration;

// struct to contain data for pthreads
typedef struct myStruct
{
    std::unordered_map<std::size_t, std::string> dict;
    std::unordered_map<std::string, std::size_t> dict_rev;
    int threadId;
    std::string inPartName;
    std::string outPartName;
    int debug;
} compress_args_t;

// update the declarations
duration<double, std::milli> delta(std::string msg);
float get_size(std::string file);
void print_dict(std::unordered_map<std::size_t, std::string> &umap);
void print_dict(std::unordered_map<std::string, std::size_t> &umap);
void write_dict(std::unordered_map<std::size_t, std::string> &umap, std::string &out_file, int debug);
void parse_dict(std::unordered_map<std::size_t, std::string> &umap, std::string &compressed_file);
int is_equal(std::string &input_file, std::string &decompressed_file);
void check_file(std::ifstream &file, std::string &file_name);
void check_file(std::ofstream &file, std::string &file_name);
void construct_params(std::ifstream &params, int &debug, std::string &input_file, std::string &output_file, int &mode, int &optimization, int &numThreads);
void construct_decompress_output_file(std::string &input_file, std::string &decomp_output_file);
void compress(std::ifstream &input, std::ofstream &output, std::unordered_map<std::size_t, std::string> &dict, int debug);
void decompress(std::ifstream &compressed, std::ofstream &decompressed, std::unordered_map<std::size_t, std::string> &dict, int debug);

using std::chrono::high_resolution_clock;
using std::chrono::time_point;
using std::chrono::system_clock;
using std::chrono::duration;
high_resolution_clock::time_point start_time = high_resolution_clock::now();
int time_first = 1;

duration<double, std::milli> delta(std::string msg = ""){
    //duration<double, std::milli> t2 = (end2 - start2) / 1000;
    duration<double, std::milli> del;
    //time_point<system_clock,duration<double>> zero_{};
    int silent = 0;
    if(msg == ""){silent = 1;}
    if(time_first){
        start_time = high_resolution_clock::now();
        time_first = 0;
        del = (high_resolution_clock::now() - high_resolution_clock::now()) / 1000;
    }
    else{
        del = (high_resolution_clock::now() - start_time) / 1000;
        if(!silent){std::cout << "  " << msg << ": " << del.count() << " s" << std::endl;}
        start_time = high_resolution_clock::now();
    }
    return del;
}

float get_size(std::string file){
    // https://www.tutorialspoint.com/how-can-i-get-a-file-s-size-in-cplusplus
    std::ifstream in_file(file, std::ios::binary);
    in_file.seekg(0, std::ios::end);
    float file_size = in_file.tellg();
    return file_size;
}

void print_dict(std::unordered_map<std::size_t, std::string> &umap){
    // may want to make a slower sorted version
    std::unordered_map<std::size_t, std::string>::iterator itr;
    for (itr = umap.begin(); itr != umap.end(); itr++)
    {
        // itr works as a pointer to pair<string, double>
        // type itr->first stores the key part  and
        // itr->second stores the value part
        std::cout << itr->first << "  " << itr->second << std::endl;
    }
}

void print_dict(std::unordered_map<std::string, std::size_t> &umap){
    // may want to make a slower sorted version
    std::unordered_map<std::string, std::size_t>::iterator itr;
    for (itr = umap.begin(); itr != umap.end(); itr++)
    {
        // itr works as a pointer to pair<string, double>
        // type itr->first stores the key part  and
        // itr->second stores the value part
        std::cout << itr->first << "  " << itr->second << std::endl;
    }
}

void print_vec(std::vector<std::pair<std::string, size_t>> &pair_vec){
    // may want to make a slower sorted version
    std::vector<std::pair<std::string, size_t>>::iterator itr;
    for (itr = pair_vec.begin(); itr != pair_vec.end(); itr++)
    {
        // itr works as a pointer to pair<string, double>
        // type itr->first stores the key part  and
        // itr->second stores the value part
        std::cout << itr->first << "  " << itr->second << std::endl;
    }
}

void write_dict(std::unordered_map<std::size_t, std::string> &umap, std::string &out_file, int debug){
    // output is reopened and will append to end of file
    // may want to make a slower sorted version
    // https://www.delftstack.com/howto/cpp/how-to-append-text-to-a-file-in-cpp/
    
    std::ofstream out;
    out.open(out_file, std::ios_base::app);
    out << "__dictbegin__" << std::endl;
    std::unordered_map<std::size_t, std::string>::iterator itr;
    for (itr = umap.begin(); itr != umap.end(); itr++)
    {
        // itr works as a pointer to pair<string, double>
        // type itr->first stores the key part  and
        // itr->second stores the value part
        out << itr->first << ";" << itr->second << std::endl;
    }
    out.close();
    if(debug){std::cout << "Dictionary writing has completed!" << std::endl;}
}

void parse_dict(std::unordered_map<std::size_t, std::string> &umap, std::string &compressed_file){
    // want to skip to the beginning of the dictionary then create the dictionary 
    std::ifstream compressed(compressed_file);
    std::string line;
    
    std::string cword;
    std::string ckey_str;
    std::size_t ckey;
    int found = 0; 
    while(!compressed.eof()){
        std::getline(compressed,line);
        // the dictionary has started
        if(line == "__dictbegin__"){
            found = 1;
        }
        if(found){
            // split by semicolon delineator 
            // write to dictionary; left of find is key, right is value
            std::size_t found = line.find_last_of(";");
            ckey_str = line.substr(0,found);
            int ckey_int = std::stoi(ckey_str);
            ckey = ckey_int;
            cword = line.substr(found+1);
            umap.insert({ ckey, cword });
        }
    }
    compressed.close();
}

int is_equal(std::string &input_file, std::string &decompressed_file){
    // fix this with length differences
    // open both files in read mode
    std::ifstream input(input_file);
    std::ifstream decompressed(decompressed_file);

    std::string cword;
    std::string dword;
    // files SHOULD be equal length... if not then bad
    // may need to implement size checking
    while(!decompressed.eof()){
        getline(input,cword);
        getline(decompressed,dword);
        if((cword != "") && (dword != "")){
            if(cword != dword){
                return 0;
            }
        }
    }

    return 1;
}

void check_file(std::ifstream &file, std::string &file_name){
    // check that the parameters are loaded
    if (!file.good())
    { // pulled from submitty resources, cs1200, on helpful c++ programming information
      // https://www.cs.rpi.edu/academics/courses/spring20/csci1200/programming_information.php
        std::cerr << "Can't open the file: " << file_name << " ; please try again" << std::endl;
        exit(true);
    }
}

void check_file(std::ofstream &file, std::string &file_name){
    // check that the parameters are loaded
    if (!file.good())
    { // pulled from submitty resources, cs1200, on helpful c++ programming information
      // https://www.cs.rpi.edu/academics/courses/spring20/csci1200/programming_information.php
        std::cerr << "Can't open the file: " << file_name << " ; please try again" << std::endl;
        exit(true);
    }
}

void construct_params(std::ifstream &params, int &debug, std::string &input_file, std::string &output_file,  int &optimization, int &numThreads){
    // read in debug
    std::string line;
    getline(params,line);
    getline(params,line);
    debug = stoi(line);
    if( !((debug == 0) || (debug == 1)) ){
        std::cerr << "Wrong input for debug; must be either 0 (disabled) or 1 (enabled); recieved value: " << debug << std::endl;
        std::cout << "Wrong input for debug; must be either 0 (disabled) or 1 (enabled); recieved value: " << debug << std::endl;
    }
    
    // read in input file
    getline(params,line);
    getline(params,line);
    input_file = line;
    
    // construct output file
    std::string output_prefix = "compressed_";
    //https://www.cplusplus.com/reference/string/string/find_last_of/
    std::size_t found = input_file.find_last_of("/\\");
    output_file = input_file.substr(0,found) + "/" + output_prefix + input_file.substr(found+1);

    // read in optimization 
    getline(params,line);
    getline(params,line);
    optimization = stoi(line);

    // read in number of threads
    getline(params,line);
    getline(params,line);
    numThreads = stoi(line);

    if(debug == 1) std::cout << "Debug: " << debug << std::endl;
    std::cout << "Input File: " << input_file << std::endl;
    if(debug){std::cout << "  Size in bytes: " << get_size(input_file) << std::endl;}
    std::cout << "Output File: " << output_file << std::endl;
}

void construct_decompress_output_file(std::string &input_file, std::string &decomp_output_file){
    // create decompression output file 
    std::string decomp_output_prefix = "decompressed_";
    //https://www.cplusplus.com/reference/string/string/find_last_of/
    std::size_t decomp_found = input_file.find_last_of("/\\");
    decomp_output_file = input_file.substr(0,decomp_found) + "/" + decomp_output_prefix + input_file.substr(decomp_found+1);
}

void create_dictionary_and_compress(std::ifstream &input, std::ofstream &output, std::unordered_map<std::size_t, std::string> &dict, int debug){
    // compress file and construct dictionary 
    std::size_t dict_key = 0;
    std::string word;
    std::size_t key;
    std::unordered_map<std::string, std::size_t> dict_rev;
    //duration<double, std::milli> find_time;
    //duration<double, std::milli> insert_time;
    while(!input.eof()){
        // read in value
        getline(input,word);
        if(word != ""){
            int found = 0;
            //delta("  compression; time to find key");
            //find_time = delta();
            
            // using a reverse map to search for values since iterating is extremely slow
            std::unordered_map<std::string, std::size_t>::iterator citr;
            citr = dict_rev.find(word);
            if(citr != dict_rev.end()){
                found = 1;
                key = citr->second;
            }

            if(!found){
                // if not found, add key to dict
                //insert_time = delta();
                dict.insert(make_pair(dict_key,word));   
                dict_rev.insert(make_pair(word,dict_key));
                //delta("  compression; time to insert key pair");
                key = dict_key;
                dict_key += 1;
            }
            // now, create output 
            output << key << std::endl;
            if(debug){std::cout << "Key: " << key << " Value: " << word << std::endl;}
        
        }
        //std::cout << find_time.count() / insert_time.count() << std::endl;
        //delta("  compression; time to compress line");
    }
    if(debug){print_dict(dict);}
}
// might want to copy dictionary multiple times instead of passing by reference. may be performance issues with pthreads. 
void compress_better(std::ifstream &input, std::ofstream &output, std::unordered_map<std::size_t, std::string> &dict, std::unordered_map<std::string, std::size_t> &dict_rev, int debug){
    // compress file  
    
    std::string word;
    std::size_t key;
    
    // take dictionary, then assign new values
    std::string ckey_str;
    std::size_t ckey;
    while(!input.eof()){
        // read in value
        getline(input,word);
        if(word != ""){
            // find value using key
            std::unordered_map<std::string, std::size_t>::iterator citr;
            citr = dict_rev.find(word);
            //delta("time to find word");
            // now, create output 
            output << citr->second << std::endl;
            if(debug){std::cout << "Key: " << key << " Value: " << word << std::endl;}
        }
    }
    if(debug){print_dict(dict);}
}
void compress_best(std::string &inPartName, std::string &outPartName, std::unordered_map<std::size_t, std::string> &dict, std::unordered_map<std::string, std::size_t> &dict_rev, int debug){
    // compress file 
    // now using FILE instead of streams (must faster)
    
    // convert string to char array
    char inName[inPartName.length() + 1];
    strcpy(inName, inPartName.c_str());
    char outName[outPartName.length() + 1];
    strcpy(outName, outPartName.c_str());

    FILE *inPart; 
    FILE *outPart;
    char *line = NULL;
    size_t len = 0;

    inPart = fopen(inName, "r");
    if (inPart == NULL) perror ("Error inPart");
    outPart = fopen(outName, "w");
    if (outPart == NULL) perror ("Error outPart");
    
    std::string word;
    
    // take dictionary, then assign new values
    std::string ckey_str;
    std::size_t ckey;

    while(getline(&line, &len, inPart) != -1){
        // read in value
        word = strtok(line, "\n");        
        if(word != ""){
            // find value using key
            std::unordered_map<std::string, std::size_t>::iterator citr;
            citr = dict_rev.find(word);
            int size = word.length();
            char val[size+1+2];
            std::sprintf(val,"%d",(int)citr->second);
            fputs ( val, outPart);
            fputc ( '\n', outPart);
            if(debug){std::cout << "Key: " << citr->first << " Value: " << word << std::endl;}
        }
    }
    fclose(inPart);
    fclose(outPart);
    if(debug){print_dict(dict);}
}

void decompress(std::ifstream &compressed, std::ofstream &decompressed, std::unordered_map<std::size_t, std::string> &dict, int debug){
    // decompress compressed file and write output to decompressed
    // stop decompressing once the dictionary has been hit
    std::string ckey_str;
    std::size_t ckey;
    int end = 0;
    while(!compressed.eof()){
        // read in value
        getline(compressed,ckey_str);
        if(ckey_str == "__dictbegin__"){
            end = 1;
        }
        // if key not empty, and not at the end 
        if( (ckey_str != "") && (end == 0)){
            // read input string, convert to int then cast to ckey for size_t
            int ckey_int = std::stoi(ckey_str);
            ckey = ckey_int;
            //std::cout << word << std::endl;
            // search dictionary for key
            // https://www.cplusplus.com/reference/map/map/find/
            
            int found = 0;
            std::unordered_map<std::size_t, std::string>::iterator citr;
            citr = dict.find(ckey);
            if(debug){std::cout << "Key: " << ckey << " Value: " << citr->second << std::endl;}
            // if key found
            if(citr != dict.end()){
                // want to output the value to the output file (decompressed)
                decompressed << citr->second << std::endl;
            }
            // if key not found 
            else{
                std::cerr << "decompression failed; no value for the key: " << ckey << std::endl;
            }
        }
    }
}

bool comp(std::pair<std::string, std::size_t> a, std::pair<std::string, std::size_t> b) {
    return a.second > b.second;
}

void construct_dict_better(std::string input_file, std::unordered_map<std::size_t, std::string> &dict, std::unordered_map<std::string, std::size_t> &dict_rev, int &numElems, int debug){
    std::unordered_map<std::string, std::size_t> dict_rev_freq;
    std::ifstream input(input_file);
    // go through entire file 
    // compress file and construct dictionary 
    std::string word;
    while(!input.eof()){
        // read in value
        getline(input,word);
        if(word != ""){
            numElems++;
            int found = 0;
            // using a reverse map to search for values since iterating is extremely slow
            std::unordered_map<std::string, std::size_t>::iterator citr;
            citr = dict_rev_freq.find(word);
            if(citr != dict_rev_freq.end()){
                found = 1;
                citr->second += 1;
            }
            if(!found){
                // if not found, add key to dict
                dict_rev_freq.insert(make_pair(word,0));
            }
        }
        else{break;}
        //std::cout << find_time.count() / insert_time.count() << std::endl;
        //delta("  compression; time to compress line");
    }
    //print_dict(dict_rev_freq);
    // https://stackoverflow.com/questions/31323135/sort-an-unordered-map-using-sort
    std::vector<std::pair<std::string, size_t>> sorted_pair(dict_rev_freq.begin(), dict_rev_freq.end());
    sort(sorted_pair.begin(), sorted_pair.end(), comp);
    //print_vec(sorted_pair);
    input.close();

    // construct dictionary with low to high frequency corresponding to large to small numbers
    std::size_t dict_key = 0;

    std::vector<std::pair<std::string, size_t>>::iterator itr;
    for (itr = sorted_pair.begin(); itr != sorted_pair.end(); itr++)
    {
        dict.insert(make_pair(dict_key,itr->first));
        dict_rev.insert(make_pair(itr->first,dict_key));
        dict_key += 1;
    }
    //print_dict(dict);

}

void construct_dict_best(std::string input_file, std::unordered_map<std::size_t, std::string> &dict, std::unordered_map<std::string, std::size_t> &dict_rev, int &numElems, int debug){
    // replace ifstream with FILE*
    std::unordered_map<std::string, std::size_t> dict_rev_freq;
    
    // convert string to char array
    char inName[input_file.length() + 1];
    strcpy(inName, input_file.c_str());

    FILE *inFile; 
    char *line = NULL;
    size_t len = 0;

    inFile = fopen(inName, "r");
    if (inFile == NULL) perror ("Error opening file");
    // go through entire file 
    // compress file and construct dictionary 
    std::string word;
    while(getline(&line, &len, inFile) != -1){
        // read in value
        word = strtok(line, "\n");
        if(word != ""){
            numElems++;
            int found = 0;
            // using a reverse map to search for values since iterating is extremely slow
            std::unordered_map<std::string, std::size_t>::iterator citr;
            
            citr = dict_rev_freq.find(word);
            if(citr != dict_rev_freq.end()){
                found = 1;
                citr->second += 1;
            }
            if(!found){
                // if not found, add key to dict
                dict_rev_freq.insert(make_pair(word,0));
            }
        }
        else{break;}
    }
    // https://stackoverflow.com/questions/31323135/sort-an-unordered-map-using-sort
    std::vector<std::pair<std::string, size_t>> sorted_pair(dict_rev_freq.begin(), dict_rev_freq.end());
    sort(sorted_pair.begin(), sorted_pair.end(), comp);
    fclose(inFile);

    // construct dictionary with low to high frequency corresponding to large to small numbers
    std::size_t dict_key = 0;

    std::vector<std::pair<std::string, size_t>>::iterator itr;
    for (itr = sorted_pair.begin(); itr != sorted_pair.end(); itr++)
    {
        dict.insert(make_pair(dict_key,itr->first));
        dict_rev.insert(make_pair(itr->first,dict_key));
        dict_key += 1;
    }
    //print_dict(dict);

}

static void *compress_multithread(void *data){

    // simply call compress_better for each thread   
    
    compress_args_t *args = (compress_args_t *)data;
    // std::cout << "in thread: " << args->threadId << std::endl;
    // std::ifstream inPart(args->inPartName);
    // std::ofstream outPart(args->outPartName);
    // compress_best(inPart, outPart, args->dict, args->dict_rev, args->debug);
    // inPart.close();
    // outPart.close();
    
    compress_best(args->inPartName, args->outPartName, args->dict, args->dict_rev, args->debug);
}

int main(int argc, char *argv[])
{
    delta();
    // check input parameters and open input file 
    std::ifstream params(argv[1]); // input file
    std::string params_file = argv[1];

    check_file(params, params_file);
    int debug;
    std::string input_file;
    std::string output_file;
    
    int optimization;
    int numThreads; 
    construct_params(params, debug, input_file, output_file, optimization, numThreads);
    
    // open input and output, check that the operation succedded 
    std::ifstream input(input_file); // input file
    check_file(input,input_file);
    std::ofstream output(output_file); // output file
    check_file(output,output_file);
    params.close();
    
    
    if(debug){std::cout << "Input parameters accepted!" << std::endl; delta("time to construct parameters");}
    std::cout << "  Compression has started!" << std::endl;

    // https://www.geeksforgeeks.org/unordered_map-in-cpp-stl/
    // will use this built in hash map. might reimplement with b tree later
    std::unordered_map<std::size_t, std::string> dict;
    std::unordered_map<std::string, std::size_t> dict_rev;

    // compress file and construct dictionary
    // add optimization where most frequent words use smallest key 
    //   most number of keys is less than 200,000
    int numElems = 0;
    if(optimization == 0){
        create_dictionary_and_compress(input, output, dict, debug);
        input.close();
        output.close();
    }
    else if(optimization == 1){
        construct_dict_better(input_file, dict, dict_rev, numElems, debug);
        compress_better(input, output, dict, dict_rev, debug);
        input.close();
        output.close();
    }
    else if(optimization == 2){
        
        //construct_dict_better(input_file, dict, dict_rev, numElems, debug);
        construct_dict_best(input_file, dict, dict_rev, numElems, debug);
        if(debug){delta("time to construct dictionary");}
        // std::cout << "num of elems " << numElems << std::endl;
        // same strategy as in project 2
        // split up input file into pieces, find out sizes
        
        int equalPartSize;
        int numParts;
        int lastPartSize; 
        equalPartSize = floor(numElems / numThreads);
        numParts = floor(numElems / equalPartSize);
        lastPartSize = numElems - numParts*equalPartSize + equalPartSize;
        if(debug){
            std::cout << "equal size: " << equalPartSize << std::endl;
            std::cout << "num parts: " << numParts << std::endl;
            std::cout << "last part size: " << lastPartSize << std::endl;}

        char inName[input_file.length() + 1];
        strcpy(inName, input_file.c_str());

        FILE *inFile;
        char buffer [10000];
        std::string line;
        
        inFile = fopen(inName, "r");
        if (inFile == NULL) perror ("Error opening file");
        
        int lastPart = 0;
        int numRead = 0;
        
        // create multiple input files 
        for(int i = 0; i < numParts; i++){
            if(i == numParts-1){lastPart = 1;};
            // create input part file name 
            std::string inPartName = "temp/inPart" + std::to_string(i) + ".txt";
            char inPartNameChar[inPartName.length() + 1];
            strcpy(inPartNameChar, inPartName.c_str());
            FILE *inPart;
            inPart = fopen(inPartNameChar, "w");

            char *line = NULL;
            size_t len = 0;

            // read in first part
            if(!lastPart){
                while( getline(&line, &len, inFile) != -1) {
                    fputs (line , inPart);
                    numRead++;
                    if(numRead < equalPartSize){
                        // write to temp output 
                        continue;
                    }
                    else{break;}
                }
                numRead = 0; 
            }
            else if(lastPart){
                while( getline(&line, &len, inFile) != -1) {
                    fputs (line , inPart);
                    numRead++;
                    if(numRead < lastPartSize){
                        // write to temp output 
                        continue;
                    }
                    else{break;}
                }
                numRead = 0; 
            }
            fclose(inPart);
        }
        if(debug){delta("time to create temp input files");}
        pthread_t threads[numThreads];
        compress_args_t args[numThreads];
        // dispatch threads
        for(int i = 0; i < numParts; i++){
            args[i].debug = debug;
            args[i].dict_rev = dict_rev;
            args[i].inPartName = "temp/inPart" + std::to_string(i) + ".txt";
            args[i].outPartName = "temp/outPart" + std::to_string(i) + ".txt";
            args[i].threadId = i; 
            pthread_create (&threads[i], NULL, compress_multithread, &args[i]);
        }

        for (unsigned i = 0; i < numParts; i++){
            pthread_join (threads[i], NULL);
        }

        // now reconstruct the correct output
        for (unsigned i = 0; i < numThreads; i++){
            // file to open
            std::string outPartName = "temp/outPart" + std::to_string(i) + ".txt";
            std::ifstream outPart(outPartName);
            while(!outPart.eof()){
                getline(outPart,line);
                // write to output 
                output << line << std::endl;
            }
            outPart.close();
        }
        input.close();
        output.close();
    }

    // now, write dictionary to the end of the file 
    write_dict(dict, output_file, debug);
    delta("    time to compress");
    
    std::cout << "  Compression has completed!" << std::endl;
    std::cout << "  Decompression has started!" << std::endl;
    
    // decompress 
    // open the output as an input now
    std::ifstream compressed(output_file); 
    // create decompressed file output name
    std::string decomp_output_file;
    construct_decompress_output_file(input_file,decomp_output_file);
    // open the output file
    std::ofstream decompressed(decomp_output_file); // output file
    // run the actual decompression algorithm
    decompress(compressed, decompressed, dict, debug);

    compressed.close();
    decompressed.close();
    std::cout << "  Deompression has completed!" << std::endl;
    delta("    time to decompress");
    std::cout << std::endl;
    // check if compression and decompression was successful
    int passed = is_equal(decomp_output_file,input_file);
    std::cout << "Original file and compressed file are identical: " << passed << std::endl;
    if(debug){delta("time to check that the files are identical");}

    // add statistics based on compression ratio 
    std::cout << "Compression ratio (larger over smaller): " << get_size(input_file) << "/" << get_size(output_file);
    std::cout << " = " << get_size(input_file) / get_size(output_file) << std::endl;
    return 0;
};
