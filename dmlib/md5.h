#ifndef _MD5_H
#define _MD5_H

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned
#endif

typedef struct
{
    unsigned total[2];
    unsigned state[4];
    unsigned char buffer[64];
}
md5_context;

void md5_starts( md5_context *ctx );
void md5_update( md5_context *ctx, const uint8 *input, uint32 length );
void md5_finish( md5_context *ctx, uint8 digest[16] );

#endif /* md5.h */
