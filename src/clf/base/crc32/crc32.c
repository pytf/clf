#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
 
static unint table[256];
 
//位逆转
static unint bitrev(unint input, int bw)
{
    int i;
    unint var;
    var = 0;
    for(i=0; i<bw; i++)
    {
        if(input & 0x01)
        {
            var |= 1<<(bw - 1 - i);
        }
        input >>= 1;
    }
        
    return var;
}
 
 
//码表生成
void crc32Init(unint poly)
{
    int i;
    int j;
    unint c;
 
    poly = bitrev(poly, 32);
    for(i=0; i<256; i++)
    {
        c = i;
        for (j=0; j<8; j++)
        {
            c = (c & 1) ? (poly ^ (c >> 1)) : (c >> 1);
        }
        table[i] = c;
    }
}
 
 
//计算CRC
unint crc32(unint crc, void* input, int len)
{
    int i;
    unshort index;
    unshort *p;
    p = (unshort*)input;
    for(i=0; i<len; i++)
    {
        index = (*p ^ crc);
        crc = (crc >> 8) ^ table[index];
        p++;
    }
        
    return crc;
}
