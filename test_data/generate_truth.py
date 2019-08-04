import editdistance


text_file = "frequency_dictionary_en_500_000.txt"
query_file = "noisy_query_en_1000.txt"

text_list = []
with open(text_file, "r") as in_file:
    for line in in_file:
        text_list.append(line.rstrip("\n"))

query_list = []
with open(query_file, "r") as in_file:
    for line in in_file:
        query_list.append(line.rstrip("\n"))


for q in query_list:
    for t in text_list:
        ed = editdistance.eval(q, t)
        if ed <= 2:
            print("%s\t%s\t%d" % (q, t, ed))