#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>

#include "ffsearch.h"

using namespace std;
using namespace std::chrono;
using namespace ffsearch;

int get_time()
{
    milliseconds ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    return ms.count();
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cout << "Usage: ffsearch <input_string> <string_file>" << endl << endl;
        return 0;
    }

    FFSearch ts;

    int start_time = get_time();

    cout << "Building index...";
    ts.CreateIndexFromFile(argv[2]);
    cout << ts.GetSize() << " strings loaded, " << get_time() - start_time << " ms used" << endl;

    string test = argv[1];
    cout << "Press any key to process input:" << test << endl;
    getchar();
    
    start_time = get_time();
    std::vector<SearchResult> extract_results;
    extract_results.reserve(128);
    int iterations = 100000;
    for (int i = 0; i < iterations; ++i)
    {
        extract_results.clear();
        ts.Search(test, 2, extract_results);
    }
    
    int time_spent = get_time() -start_time;
    
    cout << "Search results:" << endl;
    for (size_t i = 0 ; i < extract_results.size(); i++) {
        cout << "id:"
             << extract_results[i].id
             << " string:\""
             << extract_results[i].name
             << "\" dist:" 
             << extract_results[i].dist << endl; 
    }
    
    cout << "Search done. Totally " << extract_results.size() << " results returned. " <<  (double)time_spent / iterations << " ms each query" << endl;
    return 0;
}
