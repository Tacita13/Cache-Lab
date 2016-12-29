//******************************************************************
//
//   Programador : Isamar López Rodríguez CCOM 4086-002
//   Num. Est.   : 801-08-3174            Primer Semestre 2016-2017
//   Laboratorio 3                        Prof. Corrada
//   Archivo     : csim.c                 Fecha : 16/11/16
//
//******************************************************************
//    Propósito :This file that contains a cache Simulation
//    It recieves four to five arguments (v, s, E, b, t) and by
//    reading the valgrind it executes the different instructions
//    and gives the results in hits, misses and evictions.
//
//******************************************************************

#include "cachelab.h"
#include "getopt.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include <strings.h>
#include <math.h>
#include <assert.h>


// long long int is used given that it will hold memory address
typedef unsigned long long int dir;

//in main will be initilized to 1
int ver;

char buf[1000];

//4 STRUCTS
//Holds the key variables in the cache (parameters and results)
typedef struct{

    int s;//gives the wanted line
    int S; //2^s
    int b;//b for bytes
    int B;// 2^b 
    int E;//cachelines in a set
    //holds the results
    int hit;
    int miss;
    int eviction;

    }CacheVar;

//Struct lines=> data members: valid, tag, lru
typedef struct{
    int valid;
    int tag;
    int age;

   //the tag from the address
    dir tags;
    char *block;

    }Line;

//Struct de set=> struct line
typedef struct{
    Line * ptrLines;

    }Set;

//Struct cache => set
typedef struct{
    Set * ptrSet;

    }Cache;


//Prototypes: 7 functions
Cache createCache(long long, int, long long);
void clearCache(Cache, long long, int, long long);
int emptyLine(Set, CacheVar);
int evictionLine(Set, CacheVar , int *);
long long bitPow(int);
CacheVar execute(Cache, CacheVar, dir);
void printHelp(char * argv[]);
void printSummary(int, int, int);


//main function: holds two switch cases: the arguments given and the instructions found in the traces file ====================================================MAIN=======================================================
int main(int argc, char **argv){

    Cache cacheSimul;
    CacheVar par;
    bzero(&par, sizeof(par));

    long long numberSets;
    long long blockSize;

    //to read the trace files as argument -t
    FILE *readTrace;
    char traceCmd;
    dir address;
    int size;

    char *traceFile;
    //evaluates the given arguments converts using atoi, if not help message is given
    char c;
    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
        case 's':
            par.s = atoi(optarg);
            break;
        case 'E':
            par.E = atoi(optarg);
            break;
        case 'b':
            par.b = atoi(optarg);
            break;
        case 't':
            traceFile = optarg;
            break;
        case 'v':
            ver = 1;
            break;
        case 'h':
            printHelp(argv);
            exit(0);
        default:
            printHelp(argv);
            exit(1);
        }
    }
    //Input validation
    if (par.s == 0 || par.E == 0 || par.b == 0 || traceFile == NULL){
        printf("%s: Missing required command line argument\n", argv[0]);
        printHelp(argv);
        exit(1);
    }

    
    numberSets = 1<<(par.s);//2^s
    blockSize = bitPow(par.b);
    par.hit = 0;
    par.miss = 0;
    par.eviction = 0;

    cacheSimul = createCache(numberSets, par.E, blockSize);

    // opens the file to read it
    readTrace  = fopen(traceFile, "r");

    if (readTrace != NULL){
        while (fscanf(readTrace, " %c %llx,%d", &traceCmd, &address, &size) == 3){

            //prints the trace information (instruction address, size)
            printf(" %c %llx,%d \n", traceCmd, address, size);

	   //Instructions found in the trace file executed accordingly
            switch(traceCmd){
                case 'I'://Instruction store (unclear)
                    break;
                case 'L': //Load
                    par = execute(cacheSimul, par, address);
                    break;
                case 'S': //Store
                    par = execute(cacheSimul, par, address);
                    break;
                case 'M': //Modify
                    par = execute(cacheSimul, par, address);
                    par = execute(cacheSimul, par, address);
                    break;
                default:
                    printHelp(argv);//Help message
                    break;
            }
        }
    }
    //gives the expected results
    printSummary(par.hit, par.miss, par.eviction);

    //cleans the cache
    clearCache(cacheSimul, numberSets, par.E, blockSize);
    
    //closes the traces file
    fclose(readTrace);

    return 0;
}


//function 1: creates the cache, initilizes the variables and takes the number
//of parameters to iterate
Cache createCache(long long numberSets, int numberLines, long long blockSize){

    Cache newCache;
    Set sets;
    Line line;
    int setIndex;
    int lineIndex;

    newCache.ptrSet = (Set *) malloc(sizeof(Set) * numberSets);

    for (setIndex = 0; setIndex < numberSets; setIndex ++){

        sets.ptrLines =  (Line *) malloc(sizeof(Line) * numberLines);
        newCache.ptrSet[setIndex] = sets;

        for (lineIndex = 0; lineIndex < numberLines; lineIndex ++){

            line.age = 0;//what is last_used, lru?
            line.valid = 0;
            line.tags = 0;
            sets.ptrLines[lineIndex] = line;
        }
    }
    return newCache;
}

//function 2: it frees the cache memory from what was previously stored
void clearCache(Cache cacheSimul, long long numberSets, int numberLines, long long blocksize){
    int setIndex;

    for (setIndex = 0; setIndex < numberSets; setIndex ++) {
        Set set = cacheSimul.ptrSet[setIndex];

        if (set.ptrLines != NULL){
            free(set.ptrLines);
        }

    }
    if (cacheSimul.ptrSet != NULL){
        free(cacheSimul.ptrSet);
    }
}

//function 3: Search for an empty line, in which case it is not full
int emptyLine(Set querySet, CacheVar par){
    int numberLines = par.E;
    int index;
    Line line;

    for (index = 0; index < numberLines; index ++){
        line = querySet.ptrLines[index];
        if (line.valid == 0) {
            return index;
        }
    }
    return -1;
}

//function 4: This function returns the postion(index) of the least recently used line(lrul)
int evictionLine(Set querySet, CacheVar par, int * usedLines){

    int numberLines = par.E;
    int maxUsed = querySet.ptrLines[0].age;
    int minUsed = querySet.ptrLines[0].age;
    int minUsedIndex = 0;

    Line line;
    int lineIndex;

    for (lineIndex = 1; lineIndex < numberLines; lineIndex ++){
        line = querySet.ptrLines[lineIndex];

        if (minUsed > line.age){
            minUsedIndex = lineIndex;
            minUsed = line.age;
        }

        if (maxUsed < line.age){
            maxUsed = line.age;
        }
    }

    usedLines[0] = minUsed;
    usedLines[1] = maxUsed;
    return minUsedIndex;
}

//function 5: elevate to the power
long long bitPow(int exp){
    long long result = 1;
    result = result << exp;
    return result;
}

//function 6: Runs the cache Simulation using functions 3, 4 and 5
CacheVar execute(Cache cacheSimul, CacheVar par, dir address){

        int lineIndex;
        int cacheFull = 1;

        int numberLines = par.E;
        int prevHits = par.hit;

        int tagSize = (64 - (par.s + par.b));
        dir inputTag = address >> (par.s + par.b);
        unsigned long long temp = address << (tagSize);
        unsigned long long setIndex = temp >> (tagSize + par.b);

        Set querySet = cacheSimul.ptrSet[setIndex];

        for (lineIndex = 0; lineIndex < numberLines; lineIndex ++){

            Line line = querySet.ptrLines[lineIndex];

            if (line.valid){

                if (line.tags == inputTag){

                    line.age ++;
                    par.hit ++;
                    querySet.ptrLines[lineIndex] = line;
                }
            }
            else if (!(line.valid) && (cacheFull)){
                //Empty line found
                cacheFull = 0;
            }

        }

        if (prevHits == par.hit){
            //Data not found in cache
            par.miss++;
        }
        else{
            //Data found in cache
            return par;
        }

        //Miss, either evict or write data into cache.

        int *usedLines = (int*) malloc(sizeof(int) * 2);
        int minUsedIndex = evictionLine(querySet, par, usedLines);

        if (cacheFull){
            par.eviction++;

            //Finds lru line
            querySet.ptrLines[minUsedIndex].tags = inputTag;
            querySet.ptrLines[minUsedIndex].age = usedLines[1] + 1;
        }
        else{
            int emptyIndex = emptyLine(querySet, par);

            //Finds the first empty line
            querySet.ptrLines[emptyIndex].tags = inputTag;
            querySet.ptrLines[emptyIndex].valid = 1;
            querySet.ptrLines[emptyIndex].age = usedLines[1] + 1;
        }

        free(usedLines);
        return par;
}

//function 7: Help menu
void printHelp(char* argv[]){
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}


