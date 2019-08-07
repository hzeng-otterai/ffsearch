#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <time.h>
#include <assert.h>
#include <unordered_set>

//#include <iostream>
#include "ffsearch.h"


using namespace std;
using namespace ffsearch;

TextCandidate::TextCandidate()
{
}

void TextCandidate::Add(uint32_t id)
{
    candidates_.push_back(id);
}

uint32_t TextCandidate::GetStart() const
{
    return 0;
}

uint32_t TextCandidate::GetEnd() const
{
    return candidates_.size();
}

void TextCandidateDict::Update(string const& key, size_t start, size_t end, uint32_t text_id, size_t pos)
{
    string subs = key.substr(start, end-start);
    if (pos == 0)
    {
        auto it = left_.find(subs);
        if (it == left_.end())
            it = left_.insert(make_pair(subs, TextCandidate())).first;
        
        it->second.Add(text_id);
    }
    else if (pos == MAX_EDIT_DISTANCE)
    {
        auto it = right_.find(subs);
        if (it == right_.end())
            it = right_.insert(make_pair(subs, TextCandidate())).first;
        
        it->second.Add(text_id);
    }
    else
    {
        auto it = middle_.find(subs);
        if (it == middle_.end())
            it = middle_.insert(make_pair(subs, TextCandidate())).first;
        
        it->second.Add(text_id);
    }
}

TextCandidate const* TextCandidateDict::Get(string const& key, size_t start, size_t end, size_t pos) const
{
    string subs = key.substr(start, end-start);
    if (pos == 0)
    {
        auto it = left_.find(subs);
        if (it != left_.end())
            return &(it->second);
    }
    else if (pos == MAX_EDIT_DISTANCE)
    {
        auto it = right_.find(subs);
        if (it != right_.end())
            return &(it->second);
    }
    else
    {
        auto it = middle_.find(subs);
        if (it != middle_.end())
            return &(it->second);
    }

    return NULL;
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
    // limit the size of text lines
    size_t limit = min(size_t(UINT32_MAX), lines.size());

    // insert the lines into text list and update index_size list
    typedef pair<uint32_t, size_t> IndexSize;
    vector<IndexSize> index_and_size_list;
    for (size_t idx = 0; idx < limit; ++idx)
    {
        Text current_text;
        current_text.name = move(lines[idx]);
        
        size_t text_length = current_text.name.size();
        if (text_length > 0)
        {
            text_min_len_ = min(text_length, text_min_len_);
            text_max_len_ = max(text_length, text_max_len_);
            index_and_size_list.push_back(make_pair(uint32_t(idx), text_length));

            // calculate segment positions
            CalcSegPosition(text_length, current_text.seg_pos);
        }

        text_.push_back(current_text);
    }

    // sort the index_size list by size
    sort(index_and_size_list.begin(), index_and_size_list.end(), [ ](IndexSize const& lhs, IndexSize const& rhs)
    {
       return lhs.second < rhs.second;
    });

    for (auto it = index_and_size_list.begin(); it != index_and_size_list.end(); ++it)
    {
        uint32_t id = it->first;
        size_t text_length = it->second;

        Text const& current_text = text_[id];

        // build trie tree
        for (size_t pos = 0; pos < SEGMENT_NUM; ++pos)
        {
            size_t start = ((pos == 0)? 0 : current_text.seg_pos[pos-1]);
            size_t end = ((pos == SEGMENT_NUM - 1)? text_length : current_text.seg_pos[pos]);

            // cout << "Update name " << start << " " << end << " " << id << " " << pos << " " << endl;
            da_.Update(current_text.name, start, end, id, pos);
        }
    }
    
    return SUCCESS;
}

size_t FFSearch::GetSize() const
{
    return text_.size();
}

int FFSearch::Search(string const& query, size_t threshold, vector<SearchResult> &result) const
{
    if (query.empty())
        return SUCCESS;
    
    if (threshold > MAX_EDIT_DISTANCE)
        threshold = MAX_EDIT_DISTANCE;

    size_t query_len = query.size();
    size_t mod = query_len % SEGMENT_NUM;

    size_t seg_pos[MAX_EDIT_DISTANCE];
    CalcSegPosition(query_len, seg_pos);

    int const* delta_start_only = GetPossiblePrefixPosition(mod);
    int const* delta_end_only = GetPossibleSuffixPosition(mod);
    int const* delta_middle_start = GetPossibleInfixStartPosition(mod);
    int const* delta_middle_end = GetPossibleInfixEndPosition(mod);
    
    unordered_set<size_t> processed_id;
    processed_id.reserve(1024);
    for (int i = 0; delta_start_only[i] <= 1; ++i)
    {
        size_t middle_start = Adjust(seg_pos[0], delta_start_only[i], 0, query_len);
        
        TextCandidate const* left_node = da_.Get(query, 0, middle_start, 0);
        if (left_node == NULL) continue;

        size_t start = left_node->GetStart();
        size_t end = left_node->GetEnd();
        if (start == end) continue;

        start = LowerBound(left_node->candidates_, start, end, query_len, threshold);
        //cout << end - start << " candidates found in left node." << endl;
        for (size_t pi = start; pi < end; pi++)
        {
            size_t textId = left_node->candidates_[pi];
            Text const & text = text_[textId];

            size_t text_len = text.name.size();
            
            if (Diff(text_len, query_len) > threshold) continue;

            size_t text_middle_start = text.seg_pos[0];
            size_t dist = CalcEditDistance(
                text.name, 
                text_middle_start, 
                text_len - text_middle_start, 
                query, 
                middle_start, 
                query_len - middle_start);

            if (dist > threshold) continue;

            if (!processed_id.insert(textId).second) continue;

            result.push_back(SearchResult{
                textId, 
                dist,
                text.name
            });
            //cout << start << " " << end << " " << " left:";
            //cout << query.substr(start, end-start) << " dist:" << dist << endl;
        }
    }
    
    if (threshold == 0) return SUCCESS;
    
    for (int i = 0; delta_end_only[i] <= 1; ++i)
    {
        size_t middle_end = Adjust(seg_pos[1], delta_end_only[i], 0, query_len);
        TextCandidate const* right_node = da_.Get(query, middle_end, query_len, 2);
        if (right_node == NULL) continue;

        size_t start = right_node->GetStart();
        size_t end = right_node->GetEnd();
        if (start == end) continue;
        
        start = LowerBound(right_node->candidates_, start, end, query_len, threshold);
        //cout << end - start << " candidates found in right node." << endl;
        for (size_t pi = start; pi < end; pi++)
        {
            size_t textId = right_node->candidates_[pi];
            Text const & text = text_[textId];

            size_t text_len = text.name.size();

            if (Diff(text_len, query_len) > threshold) continue;
            
            size_t text_middle_end = text.seg_pos[1];
            size_t dist = CalcEditDistance(
                text.name, 
                0, 
                text_middle_end, 
                query, 
                0, 
                middle_end);

            //cout << dist;
            if (dist > threshold) continue;

            if (!processed_id.insert(textId).second) continue;

            result.push_back(SearchResult{
                textId, 
                dist,
                text.name
            });
            //cout << start << " " << end << " " << " right:";
            //cout << query.substr(start, end-start) << " dist:" << dist << endl;
        }
    }
    
    if (threshold == 1) return SUCCESS;
    
    for (int i = 0; delta_middle_start[i] <= 1; ++i)
    {
        size_t middle_start = Adjust(seg_pos[0], delta_middle_start[i], 0, query_len);
        size_t middle_end = Adjust(seg_pos[1], delta_middle_end[i], 0, query_len);
        TextCandidate const* middle_node = da_.Get(query, middle_start, middle_end, 1);
        if (middle_node == NULL) continue;

        size_t start = middle_node->GetStart();
        size_t end = middle_node->GetEnd();
        if (start == end) continue;
        
        start = LowerBound(middle_node->candidates_, start, end, query_len, threshold);
        //cout << end - start << " candidates found in middle node." << endl;

        for (size_t pi = start; pi < end; pi++)
        {
            size_t textId = middle_node->candidates_[pi];
            Text const & text = text_[textId];

            size_t text_len = text.name.size();
            
            if (Diff(text_len, query_len) > threshold) continue;

            size_t text_middle_start = text.seg_pos[0];
            size_t text_middle_end = text.seg_pos[1];
            size_t dist_bwd = CalcEditDistance(
                text.name, 
                0, 
                text_middle_start, 
                query, 
                0, 
                middle_start);

            if (dist_bwd > threshold) continue;
        
            size_t dist_fwd = CalcEditDistance(
                text.name, 
                text_middle_end, 
                text_len - text_middle_end, 
                query, 
                middle_end, 
                query_len-middle_end);
                            
            if (dist_bwd + dist_fwd > threshold) continue;

            if (!processed_id.insert(textId).second) continue;

            result.push_back(SearchResult{
                textId, 
                dist_bwd + dist_fwd,
                text.name
            });
        }
    }
    
    return SUCCESS;
}

size_t FFSearch::LowerBound(vector<uint32_t> const& node, size_t start, size_t end, size_t size_value, size_t threshold) const
{
    assert(start < end <= node.size());
    size_t size_value_lower_bound = (size_value > threshold) ? (size_value - threshold) : 0;

    size_t count = end - start;
    size_t current, step;
    while (count > 0) {
        current = start; 
        step = count / 2; 
        current += step;
        if (text_[node[current]].name.size() < size_value_lower_bound)
        {
            start = ++current; 
            count -= step + 1; 
        }
        else
        {
            count = step;
        }
    }

    return start;
}

int FFSearch::CalcEditDistance(string const& doc1, int offset1, int len1, string const& doc2, int offset2, int len2)
{
    
    //cout << doc1 << ":" << doc1.substr(offset1, len1) << "<==>" << doc2 << ":" << doc2.substr(offset2, len2);

    int bot = 1;
    int top = EDIT_DISTANCE_ARRAY_LEN - 1;
    int vl, vn, vt, l2;

    unsigned edit_distance[EDIT_DISTANCE_ARRAY_LEN];
    edit_distance[0] = MAX_EDIT_DISTANCE + 1;
    edit_distance[EDIT_DISTANCE_ARRAY_LEN - 1] = MAX_EDIT_DISTANCE + 1;
    for (size_t i = 1; i < MAX_EDIT_DISTANCE + 1; ++i) {
        edit_distance[i] = MAX_EDIT_DISTANCE + 1 - i;
    }
    for (size_t i = MAX_EDIT_DISTANCE + 1; i < 2 * MAX_EDIT_DISTANCE + 2; ++i) {
        edit_distance[i] = i - MAX_EDIT_DISTANCE - 1;
    }

    //update edit distance
    for (int l1 = 1; l1 <= len1; ++l1)
    {
        for (int i = bot; i < top; ++i)
        {
            vl = edit_distance[i-1]+1;
            vt = edit_distance[i+1]+1;
            l2 = i - MAX_EDIT_DISTANCE + l1 - 2;
            vn = edit_distance[i] +
                 ((l2 >= 0 && l2 < len2) ? (doc1[offset1 + l1 - 1] != doc2[offset2 + l2]) : 1);
            edit_distance[i] = (vl > vt) ? ((vt > vn) ? vn : vt) : ((vl > vn) ? vn : vl);
        }
        for (int i = bot; i < top; ++i) {
            if (edit_distance[bot] > MAX_EDIT_DISTANCE) bot++;
            else break;
        }
        for (int i = top-1; i >= bot; --i) {
            if (edit_distance[top - 1] > MAX_EDIT_DISTANCE) top--;
            else break;
        }
        if (bot >= top) break;
    }
    //cout << " dist:" << edit_distance[MAX_EDIT_DISTANCE + 1 + len2 - len1] << endl;
    return edit_distance[MAX_EDIT_DISTANCE + 1 + len2 - len1];
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
            current_seg_len = seglen + 1;
        else
            current_seg_len = seglen;
        
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

int const* FFSearch::GetPossiblePrefixPosition(size_t mod)
{
    //delta of the first segment
    static const int mod0[] = {0, 1, 9999};
    static const int mod1[] = {-1, 0, 9999};
    static const int mod2[] = {-1, 0, 1, 9999};
    if (mod == 0) return mod0;
    else if (mod == 1) return mod1;
    else return mod2;
}

int const* FFSearch::GetPossibleSuffixPosition(size_t mod)
{
    //delta of the last segment
    static const int mod0[] = {0, 1, 9999};
    static const int mod1[] = {-1, 0, 1, 9999};
    static const int mod2[] = {-1, 0, 9999};
    if (mod == 0) return mod0;
    else if (mod == 1) return mod1;
    else return mod2;
}

int const* FFSearch::GetPossibleInfixStartPosition(size_t mod)
{
    //delta of the middle segment
    static const int mod0[] = {-1, 0, 0, 1, 1, 9999};
    static const int mod1[] = {-1, -1, 0, 0, 1, 9999};
    static const int mod2[] = {-1, 0, 0, 1, 1, 9999};
    if (mod == 0) return mod0;
    else if (mod == 1) return mod1;
    else return mod2;
}

int const* FFSearch::GetPossibleInfixEndPosition(size_t mod)
{
    static const int mod0[] = {-1, 0, 1, 0, 1, 9999};
    static const int mod1[] = {-1, 0, 0, 1, 1, 9999};
    static const int mod2[] = {-1, -1, 0, 0, 1, 9999};
    if (mod == 0) return mod0;
    else if (mod == 1) return mod1;
    else return mod2;
}

