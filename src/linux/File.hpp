#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>

struct file_content
{
    bool isOK;
    uint8_t * bytes;
    int count;
};

file_content ReadFile(char * fileName)
{
    file_content result = {};

    FILE * f = fopen(fileName,"rb");
    if(!f)
    {
        result.isOK = false;
        return result;
    }

    int capacity = 1000;
    result.bytes = (uint8_t*) calloc(1,capacity);

    while(true)
    {
        int c = fgetc(f);
        if(c<0) break;

        if(result.count >= capacity)
        {
            capacity *= 2;
            result.bytes = (uint8_t*) realloc(result.bytes,capacity);
        }

        assert(result.bytes);

        result.bytes[result.count] = (uint8_t) c;
        result.count++;
    }
    result.count++;

    result.isOK = true;
    return result;
}
