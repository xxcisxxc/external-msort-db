# External Merge Sort

## Code Compilation & Running

(For Xincheng)

## Code Structure

(For Ranjitha)

## Contribution

### Xincheng

1. sort
2. ...

### Ranjitha

...


## Expected Time

1. **50MB input, 1KB record size (51200 records)**

*Command*: `time DISTINCT=1 ./ExternalSort.exe -c 51200 -s 1024`

*Output*
```
real    0m1.171s
user    0m0.347s
sys     0m0.125s
```

2. **125MB input, 1KB record size (128000 records)**

*Command*: `time DISTINCT=1 ./ExternalSort.exe -c 128000 -s 1024`

*Output*
```
real    0m4.097s
user    0m0.879s
sys     0m0.373s
```

3. **12GB input, 1KB record size (12582912 records)**

*Command*: `time DISTINCT=1 ./ExternalSort.exe -c 12582912 -s 1024`

*Output*
```
real    6m42.670s
user    1m29.719s
sys     0m45.369s
```

4. 
