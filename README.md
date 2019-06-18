# Fast fuzzy string search

## Usage
`cd src`  
`make`  
`./ffsearch query.txt dictionary.txt`   

## Example data
`cd test_data` 
`./download_data.sh` 
`../src/ffsearch noisy_entity.txt entity_name_unique.txt` 
`../src/ffsearch noisy_query_en_1000.txt ../test_data/frequency_dictionary_en_500_000.txt` 


## Benchmark

 - using entity_name_unqiue.txt
 - 4,243,940 strings
 - 92MB file size
 - Machine: Intel(R) Core(TM) i7-8700 CPU @ 3.20GHz

### Our
 - 981 MB memory, 5s for loading
 - "NewYork": 10 results, 0.225 ms
 - "Loma Linda Estate Coloni": 1 result, 0.0018 ms

### SymSpell
 - https://github.com/wolfgarbe/SymSpell
 - 2.1 GB (can have 5GB temporily)
 - "NewYork": 8 results, 0.3 ms
 - "Loma Linda Estate Coloni": 1 result, 0.09 ms


