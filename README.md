# External Merge Sort

## Code Compilation & Running

### Testbed

1. CPU Platform
   - Tested on `AMD EPYC 7763 64-Core Processor`, `Intel(R) Core(TM) i5-10600T CPU @ 2.40GHz` and `
2. System
   - Tested on `Ubuntu 22.04.4 LTS`, `Ubuntu 20.04.6 LTS`, `Debian 12`
3. Compiler
   - Tested on `gcc-9`, `gcc-11`, `gcc-12`
  
### Compilation

```bash
make -j
```

### Running

**With** *Duplicate Elimination*

```bash
DISTINCT=1 ./ExternalSort.exe -c n_records -s record_size -o trace_file
```

**Without** *Duplicate Elimination*

```bash
DISTINCT=0 ./ExternalSort.exe -c n_records -s record_size -o trace_file
```

## Code Structure

(For Ranjitha)

## Contribution

### Xincheng

1. sort
2. ...

### Ranjitha

...


## Expected Time

1. **50MB** input, **1KB** record size (*51200* records)

*Command*

```bash
time DISTINCT=1 ./ExternalSort.exe -c 51200 -s 1024
```

*Output*
```
real    0m1.171s
user    0m0.347s
sys     0m0.125s
```

2. **125MB** input, **1KB** record size (*128000* records)

*Command*

```bash
time DISTINCT=1 ./ExternalSort.exe -c 128000 -s 1024
```

*Output*
```
real    0m4.097s
user    0m0.879s
sys     0m0.373s
```

3. **12GB** input, **1KB** record size (*12582912* records)

*Command*

```bash
time DISTINCT=1 ./ExternalSort.exe -c 12582912 -s 1024
```

*Output*
```
real    6m42.670s
user    1m29.719s
sys     0m45.369s
```

4. **120GB** input, **1KB** record size (*125829120* records)

*Command*

```bash
time DISTINCT=1 ./ExternalSort.exe -c 125829120 -s 1024
```

*Output*
```

```
