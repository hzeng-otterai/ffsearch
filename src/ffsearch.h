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
const size_t THRESHOLD = 2;
const size_t SEGMENT_NUM = THRESHOLD + 1;
const size_t EDIT_DISTANCE_ARRAY_LEN = 2 * THRESHOLD + 3;
const size_t MIN_SEGMENT_LEN = 1;

// Text extraction result structure
struct SearchResult
{
    size_t id;
    size_t dist;
    std::string name;
};

typedef std::unordered_map<size_t, SearchResult> SearchResultDict;

struct Text
{
    size_t seg_pos[THRESHOLD];
    std::string name;
};

class TextCandidate
{
public:
    TextCandidate();
    void AddLeft(uint32_t);
    void AddMiddle(uint32_t);
    void AddRight(uint32_t);
    uint32_t GetLeftStart() const;
    uint32_t GetMiddleStart() const;
    uint32_t GetRightStart() const;
    uint32_t GetLeftEnd() const;
    uint32_t GetMiddleEnd() const;
    uint32_t GetRightEnd() const;
    std::vector<uint32_t> text_candidates_;
};

class FFSearch 
{
public:
    FFSearch();
    ~FFSearch();

    int CreateIndex(std::vector<std::string> && lines);
    int CreateIndexFromFile(std::string const& file_name);

    int Search(std::string const& query, size_t threshold, std::vector<SearchResult> &result) const;

    size_t GetSize() const;
    
private:
    // minimal length of text
    size_t text_min_len_;

    // maximal length of text
    size_t text_max_len_;

    // list of text
    std::vector<Text> text_;
    
    std::vector<TextCandidate *> data_;
    std::unordered_map<std::string, int> da_;
    
private:
    int Search(std::string const& query, size_t threshold, SearchResultDict &result) const;
        
    void Clear();
    
    TextCandidate * GetTextCandidate(const std::string& key, size_t start, size_t end) const;
    
    void UpdateTextCandidate(const std::string& key, size_t start, size_t end, size_t idx, size_t pos);

    static int CalcEditDistance(std::string const& doc1, int offset1, int len1, std::string const& doc2, int offset2, int len2);
    static void CalcSegPosition(size_t len, size_t *seg_pos);
    static size_t Diff(size_t, size_t);
    static size_t Adjust(size_t, int, size_t, size_t);
};

} 

#endif
