
# download entity data from S3 and generate two example queries
wget https://handsomezebra.s3-us-west-1.amazonaws.com/public/entity_name_unique.txt.gz
gunzip entity_name_unique.txt.gz
echo "NewYork" > noisy_entity.txt && echo "Loma Linda Estate Coloni" >> noisy_entity.txt

# download spell check data from SymSpell github, remove BOM header,
# and only keep the first column
curl -s https://raw.githubusercontent.com/wolfgarbe/SymSpell/master/SymSpell.Benchmark/test_data/frequency_dictionary_en_500_000.txt | tail -c +4 | cut -d " " -f 1 > frequency_dictionary_en_500_000.txt
curl -s https://raw.githubusercontent.com/wolfgarbe/SymSpell/master/SymSpell.Benchmark/test_data/noisy_query_en_1000.txt | cut -d " " -f 1 > noisy_query_en_1000.txt