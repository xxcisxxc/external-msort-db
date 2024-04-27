# External Merge Sort

## Code Compilation & Running

### Testbed

1. CPU Platform
   - Tested on `AMD EPYC 7763 64-Core Processor`, `Intel(R) Core(TM) i5-10600T CPU @ 2.40GHz` and `Intel(R) Xeon(R) CPU @ 2.30GHz`
2. System
   - Tested on `Ubuntu 22.04.4 LTS`, `Ubuntu 20.04.6 LTS` and `Debian GNU/Linux 12 (bookworm)`
3. Compiler
   - Tested on `gcc-9`, `gcc-11` and `gcc-12`

### Use Docker

1. Build Docker Image (_Inside the project directory_)

```bash
docker build -t exmsort-project .
```

2. Running Application

```bash
docker run -v .:/usr/src/exmsort -it --rm --name running-exsort -e DISTINCT=1 exmsort-project -c n_records -s record_size -o trace_file
```

`DISTINCT=0` is _no_ Duplicate Elimination and `DISTINCT=1` is Duplicate Elimination.

3. Open Input, Output, Duplicate Data and Trace File

Files could be directly accessed inside project directory after running the above command.

```bash
cat randin # input data
cat hddout # output data
cat dupout # duplicate data
cat trace_file # trace file
```

4. (_Optional_) If you want to run application directly inside docker

```bash
docker run -it --rm --name running-exsort --entrypoint /bin/bash exmsort-project
```

Then compile & run with the following commands.

### Compilation

```bash
make -j
```

### Running

**With** _Duplicate Elimination_

```bash
DISTINCT=1 ./ExternalSort.exe -c n_records -s record_size -o trace_file
```

**Without** _Duplicate Elimination_

```bash
DISTINCT=0 ./ExternalSort.exe -c n_records -s record_size -o trace_file
```

### Interpret Output Files

1. `randin`: Input Random Data. _No Separator_ between records.
2. `hddout`: Output Unsorted Data. _No Separator_ between records.
3. `dupout`: Duplication Data with Count. For one entry, the first `record_size` bytes data is the duplicate record, the following `sizeof(uint64)t` integer is the count. _No Separator_ between entries.

## Code Structure

### Overall Sort Algorithm

```python
if input <= cache_size:
   quick_sort(input)
else if cache_size < input <= mem_size:
   inmem_merge(input)
else if mem_size < input < 2 * mem_size:
   # graceful degradation (mem -> ssd)
   spill overloaded cache-sized run in mem to ssd
   inmem_merge(runs_in_mem, runs_in_ssd)
else if 2 * mem_size <= input <= ssd_size:
   external_merge(runs_in_ssd)
else if ssd_size < input < 2 * ssd_size:
   # graceful degradation (ssd -> hdd)
   spill overloaded mem-sized run in ssd to hdd
   external_merge(runs_in_ssd, runs_in_hdd)
else: # input >= 2 * ssd_size
   nested_external_merge(runs_in_hdd)
```

(For Ranjitha)
**Main Sort Logic & Graceful Degradation**
Sort.cpp 

- It the main sort logic. The logic here decides when to generate cache_sized mini runs, dram_sized_runs, ssd_sized_runs, spill cache/dram sized runs to make space for graceful degradation etc. 
- It also decides the optimal input and output device page sizes for optimized I/O
- Sorting with graceful degradation. Different cases are handled gracefully
  input<= cache/dram size : sorting happens in memory and finally written out to hdd
  dram < input < 2*dram : the cache sized runs are spilled to ssd to make space for new inputs. At the end, we make space for the runs spilled to ssd and merge all runs at once. 
  input == 2*dram : The existing inmem runs are merged and written to ssd and the spilled inmem runs are read, merged and written back to ssd
  input <= ssd : existing ssd runs are merged and writted to ssd
  ssd < input < 2*ssd : Spill all the inmem sized runs to hdd. In the end, we make space for the runs spilled to hdd and merge all runs at once
  input == 2*ssd : Existing ssd runs are merged and written to hdd. The spilled runs in hdd are read back to ssd, merged and written in bulk to hdd. 
  input > 2*ssd : Doing multilevel merging if the fanin < the runs generated. Can be seen in Sort.cpp:157

SortFunc.cpp 

- It has the utility functions to perform the sorting and merging 
- incache_sort 
  Sorting the cache sized runs using the inbuilt quick sort
- inmem_merge
  To merge the cache sized runs in memory and write to the right output device. It used the Tournament tree of losers to perform merge. 
- inmem_merge_spill
  to merge runs efficiently when dram < input size < 2 * input_size
- external_merge
   to merge dram sized runs in ssd or ssd sized runs in hdd
- external_merge_spill 
  to merge runs efficiently when ssd < input size < 2 * ssd

**Tournamet tree of Losers**
LoserTree.h
Implementation of loser tree. It used the MergeIndex records which maintains runId and the recordId to compare the actual records. 

**Duplicate Removal**
We are doing insort duplicate removal. At each merge step, we check if there are any duplicates and not include them in the output. We also keep record of the duplicate records and the number of times it occured to calculate the witness.

Implementation can be seen in all the merge steps in SortFunc.cpp:251

**Optimal Page Size**
Utils.h:minm_nrecords this defines the optimal page size for hdd. We use this to calculate fanin and do multilevel merging

For ssd we use dram_size/(number_of_dram_sized_runs_in_ssd) this is always greater than the latency*bandwidth for ssd. This ensured the optimal I/O latency and also maximal use of dram. 

**Sort Order**
After the sorting Validate.cpp reads the sorted output and validates the sort order. 

**I/O to external devices**
Device.h maintains the details of the device. It also has utils for read, write, append etc. 

**Random input generation**
Scan.cpp has the details of input generation

**Record Structure**
record.h has the record structure and all the overloaded methods for comparison, initialization, xor etc.

## Contribution

### Xincheng

1. `class Record` implementation and Device Emulation
2. Sort Algorithm Implementation
   - In Cache Sort
   - In Memory Merge Sort
   - External Merge Sort
   - Nested Merge Sort in the Final Merge Step

### Ranjitha
1. LoserTree
2. Duplicate Removal
3. Naive final merge step
4. Witness

## Expected Time

1. **50MB** input, **1KB** record size (_51200_ records)

_Command_

```bash
time DISTINCT=1 ./ExternalSort.exe -c 51200 -s 1024
```

_Output_

```
real    0m1.171s
user    0m0.347s
sys     0m0.125s
```

2. **125MB** input, **1KB** record size (_128000_ records)

_Command_

```bash
time DISTINCT=1 ./ExternalSort.exe -c 128000 -s 1024
```

_Output_

```
real    0m4.097s
user    0m0.879s
sys     0m0.373s
```

3. **12GB** input, **1KB** record size (_12582912_ records)

_Command_

```bash
time DISTINCT=1 ./ExternalSort.exe -c 12582912 -s 1024
```

_Output_

```
real    6m42.670s
user    1m29.719s
sys     0m45.369s
```

4. **120GB** input, **1KB** record size (_125829120_ records)

_Command_

```bash
time DISTINCT=1 ./ExternalSort.exe -c 125829120 -s 1024
```

_Output_

```
real    109m11.856s
user    16m5.668s
sys     10m52.343s
```
