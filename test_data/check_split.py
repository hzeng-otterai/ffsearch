import editdistance
from itertools import chain

test_cases = [
    "abcdef",
    "abcdefg",
    "abcdefgh"
]

def CalcSegPosition(length, segment_num):
    pos = [0] * (segment_num + 1)
    seg_pos = [0] * (segment_num - 1)

    mod = length % segment_num

    seglen = length / segment_num

    pos[0] = 0

    for i in range(1, segment_num + 1): # 1, 2, 3

        current_seg_len = seglen

        #if i == 3:
        #    current_seg_len += mod


        #if mod == 1 and i == 3:
        #    current_seg_len = seglen + 1
        #elif mod == 2 and (i == 1 or i == 3):
        #   current_seg_len = seglen + 1

        if i <= mod:
            current_seg_len = seglen + 1

        remaining_seg_len = length - pos[i-1]

        current_seg_len = min(current_seg_len, remaining_seg_len)

        pos[i] = pos[i-1] + current_seg_len

        if i < segment_num:
            seg_pos[i-1] = pos[i]

    return pos, seg_pos

def SegString(s):
    pos, seg_pos = CalcSegPosition(len(s), 3)

    seg = []

    start = 0
    for end in pos[1:]:
        seg.append(s[start:end])
        start = end

    return seg

def EditString(my_str, dist):
    def op1(i, s, x=''):
        return s[:i] + x + s[i:]
    def op2(i, s, x=''):
        return s[:i] + s[i+1:]
    def op3(i, s, x=''):
        return s[:i] + x + s[i+1:]

    ops = [op1, op2, op3]

    # adding extra
    for i in range(len(my_str)+1):
        for op in ops:
            if op != op1 and i == len(my_str):
                continue
            s1 = op(i, my_str, "x")
            if dist == 1:
                yield s1
            else:
                start = i if op == op2 else i + 1
                for j in range(start, len(s1)+1):
                    for op_inner in ops:
                        if op_inner != op1 and j == len(s1):
                            continue
                        s2 = op_inner(j, s1, "y")
                        yield s2


def SegmentDiff(seg1, seg2):
    def diffInSeg(seg, mode, last_char=''):
        if mode == "prefix" and seg[0] != 'a':
            return True

        if mode == "suffix" and seg[-1] != last_char:
            return True

        for i in range(1, len(seg)):
            if ord(seg[i]) - ord(seg[i-1]) != 1:
                return True
        return False

    # return prefix/infix/suffix and the offset
    if not diffInSeg(seg2[0], "prefix"):
        pos = "prefix"
        p = 0
    elif not diffInSeg(seg2[2], "suffix", last_char = seg1[2][-1]):
        pos = "suffix"
        p = 2
    elif not diffInSeg(seg2[1], "infix"):
        pos = "infix"
        p = 1
    else:
        raise ValueError

    pos_diff = ord(seg2[p][0]) - ord(seg1[p][0])
    len_diff = len(seg2[p]) - len(seg1[p])
    pos_end_diff = pos_diff + len_diff
    return pos, seg1[p], seg2[p], pos_diff, pos_end_diff
    

#
#for t in test_cases:
#   print(SegString(t))

for t in test_cases:
    mod_num = len(t) % 3
    print("===test case: " + t + " mod:" + str(mod_num))

    seg_set = set()
    for s in chain(EditString(t, 1), EditString(t, 2)):
        #if 1 != editdistance.eval(s, t):            
        #   print("Wrong edit distance %s %s" % (t, s))
        seg_t = SegString(t)
        seg_s = SegString(s)
        pos, seg_t_seg, seg_s_seg, pos_diff, pos_end_diff = SegmentDiff(seg_t, seg_s)
        #print(seg_t, seg_s, pos, seg_t_seg, seg_s_seg, pos_diff, pos_end_diff)
        seg_set.add((pos, pos_diff, pos_end_diff))

    print(sorted(list(seg_set)))
