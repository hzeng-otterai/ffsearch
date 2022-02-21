#ifndef __FFSEARCH_H__
#define __FFSEARCH_H__

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>


namespace ffsearch {

const int SUCCESS = 0;
const int FAILURE = 1;
const size_t MAX_EDIT_DISTANCE = 2;  // currently the code only works for value 2
const size_t SEGMENT_NUM = MAX_EDIT_DISTANCE + 1;
const size_t EDIT_DISTANCE_ARRAY_LEN = 2 * MAX_EDIT_DISTANCE + 3;

// This is the structure to store search results
struct SearchResult
{
    size_t id;
    size_t dist;
    std::string name;
};

// The structure of a text string and and its segmentation positions
struct Text
{
    size_t seg_pos[MAX_EDIT_DISTANCE];
    std::string name;
};

struct TextCandidate
{
    TextCandidate();
    void Add(uint32_t);
    uint32_t GetStart() const;
    uint32_t GetEnd() const;
    std::vector<uint32_t> candidates_;
};

struct TextCandidateDict
{
    std::unordered_map<std::string, TextCandidate> left_;
    std::unordered_map<std::string, TextCandidate> middle_;
    std::unordered_map<std::string, TextCandidate> right_;

    void Update(std::string const& key, size_t start, size_t end, uint32_t text_id, size_t pos);

    // Get the text candidates for a segment of the query string
    TextCandidate const* Get(std::string const& key, size_t start, size_t end, size_t pos) const;
};

class FFSearch 
{
public:
    FFSearch();
    ~FFSearch();

    // create index from text
    int CreateIndex(std::vector<std::string> && lines);

    // Search for a text query in the texts, the edit distance is at most threshold
    int Search(std::string const& query, size_t threshold, std::vector<SearchResult> &result) const;

    // get the number of text
    size_t GetSize() const;
    
private:
    // minimal length of text
    size_t text_min_len_;

    // maximal length of text
    size_t text_max_len_;

    // list of text
    std::vector<Text> text_;

    // the mapping of text segments to the text candidates,
    // each text candidate structure contains the IDs of those texts 
    // containing the text segments as prefix, infix and suffix
    TextCandidateDict da_;
    
private:
    // Get the lowerbound index of text candidates according to text length
    size_t LowerBound(std::vector<uint32_t> const& node, size_t start, size_t end, size_t size_value, size_t threshold) const;
    
    static int CalcEditDistance(std::string const& doc1, int offset1, int len1, std::string const& doc2, int offset2, int len2);
    static void CalcSegPosition(size_t len, size_t *seg_pos);
    static size_t Diff(size_t, size_t);
    static size_t Adjust(size_t, int, size_t, size_t);

    static int const* GetPossiblePrefixPosition(size_t mod);
    static int const* GetPossibleSuffixPosition(size_t mod);
    static int const* GetPossibleInfixStartPosition(size_t mod);
    static int const* GetPossibleInfixEndPosition(size_t mod);
};

} 

#endif
