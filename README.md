# fg2019

## Introduction:

 This is a command line file compression program, implemented using a variation
 of Huffman coding. It (de)compresses a single file, but it can be used with 
 multiple files if they are archived first.  


## Warnings:

 fg2019 is a toy program and some choices that were made for the sake of simplicity
affect compatibility, namely the decision to not account for endianess, which
for example will make a file that was compressed in a big endian system unable to be
decompressed in a little endian system, and the assumption that types
such as int or size_t will have their usual sizes, which could break
some of the code in certain systems.

## How to install
```
make
```

## How to use:
To compress, run with:
```
./fg2019 -C <source-name> <compressed-name>
```

To decompress, run with:
```
./fg2019 -D <source-name> <decompressed-name>
```


## Useful Resources:


 **Huffman Coding:**

    https://brilliant.org/wiki/huffman-encoding/


**Canonical Huffman Coding**

    https://en.wikipedia.org/wiki/Canonical_Huffman_code
 	

 **Length limited huffman code heuristic:**
 	
    http://cbloomrants.blogspot.com/2010/07/07-03-10-length-limitted-huffman-codes.html
 	

 **Kraft's inequality:**
    
    https://en.wikipedia.org/wiki/Kraft%E2%80%93McMillan_inequality


 **Efficient Huffman decoding scheme:**

    https://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art007


 **Files to test the program with:**
 
 	http://corpus.canterbury.ac.nz/descriptions/


## TO DO:

> Fix up some of the comments
