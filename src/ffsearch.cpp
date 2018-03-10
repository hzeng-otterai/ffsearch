#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <time.h>
#include <assert.h>

#include <fstream>

//#include <iostream>

#include "ffsearch.h"


using namespace std;
using namespace ffsearch;


TextCandidate::TextCandidate():
    text_candidates_(3, 3)
{
}

void TextCandidate::AddLeft(uint32_t id)
{
    auto it = text_candidates_.begin() + text_candidates_[0];
    text_candidates_.insert(it, id);
    ++text_candidates_[0];
    ++text_candidates_[1];
    ++text_candidates_[2];
}

void TextCandidate::AddMiddle(uint32_t id)
{
    auto it = text_candidates_.begin() + text_candidates_[1];
    text_candidates_.insert(it, id);
    ++text_candidates_[1];
    ++text_candidates_[2];
}

void TextCandidate::AddRight(uint32_t id)
{
    auto it = text_candidates_.begin() + text_candidates_[2];
    text_candidates_.insert(it, id);
    ++text_candidates_[2];
}

uint32_t TextCandidate::GetLeftStart() const
{
    return 3;
}
uint32_t TextCandidate::GetMiddleStart() const
{
    return text_candidates_[0];
}
uint32_t TextCandidate::GetRightStart() const
{
    return text_candidates_[1];
}
uint32_t TextCandidate::GetLeftEnd() const
{
    return text_candidates_[0];
}
uint32_t TextCandidate::GetMiddleEnd() const
{
    return text_candidates_[1];
}
uint32_t TextCandidate::GetRightEnd() const
{
    return text_candidates_[2];
}


FFSearch::FFSearch() :
    text_min_len_(-1), 
    text_max_len_(0), 
    text_()
{
}

FFSearch::~FFSearch()
{

}

int FFSearch::CreateIndex(vector<string> && lines)
{
    // calculate segment positions
    size_t limit = min(size_t(UINT32_MAX), lines.size());
    for (size_t idx = 0; idx < limit; ++idx)
    {
        Text current_text;
        current_text.name = move(lines[idx]);
        
        if (!current_text.name.empty())
        {
            size_t text_length = current_text.name.size();
            text_min_len_ = min(text_length, text_min_len_);
            text_max_len_ = max(text_length, text_max_len_);

            // calculate segment positions
            CalcSegPosition(text_length, current_text.seg_pos);
            
            // build trie tree
            for (size_t pos = 0; pos < SEGMENT_NUM; ++pos)
            {
                size_t start = ((pos == 0)? 0 : current_text.seg_pos[pos-1]);
                size_t end = ((pos == SEGMENT_NUM-1)? text_length : current_text.seg_pos[pos]);
                if (start == end)
                    continue;
                //cout << "Update name " << start << " " << end << " " << idx << " " << pos << " " << endl;
                UpdateTextCandidate(current_text.name, start, end, idx, pos);
            }
        }

        text_.push_back(current_text);
    }
    
    return SUCCESS;
}

int FFSearch::CreateIndexFromFile(std::string const& file_name)
{
    vector<string> lines;
    ifstream file(file_name);

    string str; 
    while (std::getline(file, str))
    {
        lines.push_back(str);
        //cout << str << endl;
    }

    return CreateIndex(move(lines));
}


int FFSearch::Search(string const& query, size_t threshold, vector<SearchResult> &result) const
{
    if (query.empty())
        return SUCCESS;
    
    if (threshold > THRESHOLD)
        threshold = THRESHOLD;
    
    SearchResultDict resultCandidate;
    resultCandidate.reserve(128);
    
    int ret = Search(query, threshold, resultCandidate);
    
    if (ret == FAILURE)
        return ret;
    
    // copy candidates to result
    for (auto it = resultCandidate.begin(); it != resultCandidate.end(); ++it)
    {
        result.push_back(move(it->second));
    }
    
    return SUCCESS;
}

size_t FFSearch::GetSize() const
{
    return text_.size();
}

int FFSearch::Search(string const& query, size_t threshold, SearchResultDict &resultCandidate) const
{
    size_t query_len = query.size();

    size_t seg_pos[THRESHOLD];
    CalcSegPosition(query_len, seg_pos);
    
    //delta of the middle segment
    static const int delta_start_only[] = {-1, 0, 1};
    
    static const int delta_start[] = {-1,-1,  0, 0, 0, 1, 1};
    static const int delta_len[]   = {-1, 0, -1, 0, 1, 0, 1};
    
    static const int delta_end_only[] = {-1, 0, 1};
    
    for (int i = 0; i < 3; ++i)
    {
        size_t middle_start = Adjust(seg_pos[0], delta_start_only[i], 0, query_len);
        
        TextCandidate const* left_node = GetTextCandidate(query, 0, middle_start);
        
        if (left_node != NULL)
        {
            for (size_t pi = left_node->GetLeftStart() ; pi < left_node->GetLeftEnd() ; pi++)
            {
                size_t textId = left_node->text_candidates_[pi];
                Text const & text = text_[textId];

                size_t text_len = text.name.size();
                size_t text_middle_start = text.seg_pos[0];
                
                if (Diff(text_len, query_len) > threshold)
                    continue;

                auto it = resultCandidate.find(textId);
                
                if (it != resultCandidate.end())
                    continue;
                
                size_t dist = CalcEditDistance(
                    text.name, 
                    text_middle_start, 
                    text_len - text_middle_start, 
                    query, 
                    middle_start, 
                    query_len - middle_start);

                //cout << dist;
                if (dist <= threshold)
                {
                    SearchResult er = SearchResult{
                        textId, 
                        dist,
                        text.name
                    };

                    resultCandidate.insert({textId, er});
                    //cout << start << " " << end << " " << " left:";
                    //cout << query.substr(start, end-start) << " dist:" << dist << endl;
                }
            }

        }
    }
    
    if (threshold == 0)
        return SUCCESS;
    
    for (int i = 0; i < 3; ++i)
    {
        size_t middle_end = Adjust(seg_pos[1], delta_end_only[i], 0, query_len);
        TextCandidate const* right_node = GetTextCandidate(query, middle_end, query_len);
        
        if (right_node != NULL)
        {
            for (size_t pi = right_node->GetRightStart() ; pi < right_node->GetRightEnd() ; pi++)
            {
                size_t textId = right_node->text_candidates_[pi];
                Text const & text = text_[textId];

                size_t text_len = text.name.size();
                size_t text_middle_end = text.seg_pos[1];

                if (Diff(text_len, query_len) > threshold)
                    continue;

                auto it = resultCandidate.find(textId);
                
                if (it != resultCandidate.end())
                    continue;
                
                size_t dist = CalcEditDistance(
                    text.name, 
                    0, 
                    text_middle_end, 
                    query, 
                    0, 
                    middle_end);

                //cout << dist;
                if (dist <= threshold)
                {
                    SearchResult er = SearchResult{
                        textId, 
                        dist,
                        text.name
                    };
                    resultCandidate.insert({textId, er});
                    //cout << start << " " << end << " " << " right:";
                    //cout << query.substr(start, end-start) << " dist:" << dist << endl;
                }
            }
        }
    }
    
    if (threshold == 1)
        return SUCCESS;
    
    for (int i = 0; i < 7; ++i)
    {
        size_t middle_start = Adjust(seg_pos[0], delta_start[i], 0, query_len);
        size_t middle_end = Adjust(seg_pos[1], delta_len[i], 0, query_len);
        TextCandidate const* middle_node = GetTextCandidate(query, middle_start, middle_end);
        
        if (middle_node != NULL)
        {
            for (size_t pi = middle_node->GetMiddleStart() ; pi < middle_node->GetMiddleEnd() ; pi++)
            {
                size_t textId = middle_node->text_candidates_[pi];
                Text const & text = text_[textId];

                size_t text_len = text.name.size();
                size_t text_middle_start = text.seg_pos[0];
                size_t text_middle_end = text.seg_pos[1];
                
                if (Diff(text_len, query_len) > THRESHOLD)
                    continue;

                auto it = resultCandidate.find(textId);
                
                if (it != resultCandidate.end())
                    continue;
                
                size_t dist_bwd = CalcEditDistance(
                    text.name, 
                    0, 
                    text_middle_start, 
                    query, 
                    0, 
                    middle_start);
            
                size_t dist_fwd = CalcEditDistance(
                    text.name, 
                    text_middle_end, 
                    text_len - text_middle_end, 
                    query, 
                    middle_end, 
                    query_len-middle_end);
                                
                if (dist_bwd + dist_fwd <= THRESHOLD)
                {
                    SearchResult er = SearchResult{
                        textId, 
                        dist_bwd + dist_fwd,
                        text.name
                    };
                    resultCandidate.insert({textId, er});
                }
            }
        }
    }
    
    return SUCCESS;
}

void FFSearch::Clear()
{
    text_min_len_ = -1;
    text_max_len_ = 0; 
    text_.clear();
}

TextCandidate * FFSearch::GetTextCandidate(const std::string& key, size_t start, size_t end) const
{
    auto it = da_.find(key.substr(start, end-start));
    if (it != da_.end())
        return data_[it->second];
    else
        return NULL;
}

void FFSearch::UpdateTextCandidate(const std::string& key, size_t start, size_t end, size_t idx, size_t pos)
{
    string subs = key.substr(start, end-start);
    auto it = da_.find(subs);

    //cout << "Got i " << i << endl;
    int i;
    if (it == da_.end())
    {
        i = data_.size();
        da_[subs] = i;
        data_.push_back(new TextCandidate());
    }
    else
    {
        i = it->second;
    }
    
    //cout << "Update node with " << i << " data size" << data.size() << endl;
    if (pos == 0)
        data_[i]->AddLeft(idx);
    else if (pos == SEGMENT_NUM - 1)
        data_[i]->AddRight(idx);
    else
        data_[i]->AddMiddle(idx);
    
    //cout << "Update node done." << endl;
}

int FFSearch::CalcEditDistance(string const& doc1, int offset1, int len1, string const& doc2, int offset2, int len2)
{
    
    //cout << doc1 << ":" << doc1.substr(offset1, len1) << "<==>" << doc2 << ":" << doc2.substr(offset2, len2);

    if (Diff(len1, len2) > THRESHOLD)
        return THRESHOLD+1;

    int bot = 1;
    int top = EDIT_DISTANCE_ARRAY_LEN - 1;
    int vl, vn, vt, l2;

    unsigned edit_distance[EDIT_DISTANCE_ARRAY_LEN];
    edit_distance[0] = THRESHOLD + 1;
    edit_distance[EDIT_DISTANCE_ARRAY_LEN - 1] = THRESHOLD + 1;
    for (size_t i = 1; i < THRESHOLD + 1; ++i) {
        edit_distance[i] = THRESHOLD + 1 - i;
    }
    for (size_t i = THRESHOLD + 1; i < 2 * THRESHOLD + 2; ++i) {
        edit_distance[i] = i - THRESHOLD - 1;
    }

    //update edit distance
    for (int l1 = 1; l1 <= len1; ++l1)
    {
        for (int i = bot; i < top; ++i)
        {
            vl = edit_distance[i-1]+1;
            vt = edit_distance[i+1]+1;
            l2 = i - THRESHOLD + l1 - 2;
            vn = edit_distance[i] +
                 ((l2 >= 0 && l2 < len2) ? (doc1[offset1 + l1 - 1] != doc2[offset2 + l2]) : 1);
            edit_distance[i] = (vl > vt) ? ((vt > vn) ? vn : vt) : ((vl > vn) ? vn : vl);
        }
        for (int i = bot; i < top; ++i) {
            if (edit_distance[bot] > THRESHOLD) bot++;
            else break;
        }
        for (int i = top-1; i >= bot; --i) {
            if (edit_distance[top - 1] > THRESHOLD) top--;
            else break;
        }
        if (bot >= top) break;
    }
    //cout << " dist:" << edit_distance[THRESHOLD + 1 + len2 - len1] << endl;
    return edit_distance[THRESHOLD + 1 + len2 - len1];
}

void FFSearch::CalcSegPosition(size_t len, size_t *seg_pos)
{
    size_t pos[SEGMENT_NUM + 1];
    
    // calculate segment positions
    size_t mod = len % SEGMENT_NUM;
    size_t seglen = len / SEGMENT_NUM;
    pos[0] = 0;
    for (size_t i = 1 ; i <= SEGMENT_NUM ; i++)
    {
        size_t current_seg_len;
        if (i <= mod)
            current_seg_len = max(seglen + 1, MIN_SEGMENT_LEN);
        else
            current_seg_len = max(seglen, MIN_SEGMENT_LEN);
        
        size_t remaining_seg_len = len - pos[i-1];
        current_seg_len = min(current_seg_len, remaining_seg_len);
        
        pos[i] = pos[i - 1] + current_seg_len;
        
        if (i < SEGMENT_NUM)
            seg_pos[i-1] = pos[i];
    }
    
    assert(pos[SEGMENT_NUM] == len);
}

size_t FFSearch::Diff(size_t a, size_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

size_t FFSearch::Adjust(size_t value, int delta, size_t lower, size_t upper)
{
    return min(upper, size_t(max(int(lower), int(value) + delta)));
}
