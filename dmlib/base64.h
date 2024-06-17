#ifndef __INC_BASE64__
#define __INC_BASE64__
const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char* Base64Encode(const char* data, int len);
int Base64Decode(const char* data, char** decdata);
inline void Base64Free(void* p)
{
	char *wp = (char*)p;
	delete [] wp;
}

#endif
