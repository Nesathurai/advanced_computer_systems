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

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::time_point;
using std::chrono::duration;

int main(int argc, char *argv[])
{
    
    std::string cword;
    std::string ckey_str = " ";
    std::size_t ckey;

    
    int ckey_int = std::stoi(ckey_str);
    ckey = ckey_int;
    std::cout << ckey_str << " " << ckey << std::endl;


    return 0;
};

