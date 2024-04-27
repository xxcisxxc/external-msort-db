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

## Contribution

### Xincheng

1. `class Record` implementation and Device Emulation
2. Sort Algorithm Implementation
   - In Cache Sort
   - In Memory Merge Sort
   - External Merge Sort
   - Nested Merge Sort in the Final Merge Step

### Ranjitha

...

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
