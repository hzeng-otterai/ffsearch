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

vector<string> get_lines(const char * file_name)
{
    vector<string> lines;
    ifstream file(file_name);

    string str; 
    while (std::getline(file, str))
    {
        lines.push_back(str);
    }

    return lines;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4)
    {
        cout << "Usage: ffsearch <query_file> <dictionary_file> <iterations>" << endl << endl;
        return 0;
    }

    vector<string> query_lines = get_lines(argv[1]);
    vector<string> dictionary_lines = get_lines(argv[2]);

    int iterations = 1000;
    if (argc == 4)
        iterations = stoi(argv[3]);

    FFSearch ts;

    int start_time = get_time();

    cout << "Building index...";
    ts.CreateIndex(move(dictionary_lines));
    cout << ts.GetSize() << " strings loaded, " << get_time() - start_time << " ms used" << endl;

    cout << "Press any key to process input." << endl;
    getchar();
    
    start_time = get_time();
    size_t nb_queries = query_lines.size();
    std::vector<std::vector<SearchResult>> extract_results(nb_queries);

    for (size_t i = 0; i < nb_queries; i++)
    {
        extract_results[i].reserve(128);
        for (int j = 0; j < iterations; j++)
        {
            extract_results[i].clear();
            const char * test_query = query_lines[i].c_str();
            ts.Search(test_query, 2, extract_results[i]);
        }
    }
    
    int time_spent = get_time() - start_time;

    cout << "Search done. " <<  (double)time_spent / iterations / query_lines.size() << " ms each query" << endl;

    for(size_t i = 0; i < nb_queries; ++i)
    {
        cout << extract_results[i].size() << " results for query: " << query_lines[i] << endl;
        for(size_t j = 0; j < extract_results[i].size(); j++)
        {
            cout << "  " << extract_results[i][j].name << "\t" << extract_results[i][j].dist << endl;
        }
    }

    return 0;
}
