#include <string.h>
#include "base64.h"

char* Base64Encode(const char* data, int len)
{
	int size;
	register int i;
	char *p, *buf;
	unsigned const char *ucdata;
	unsigned char wp[4];
	
	if(len < 1) return NULL;
	
	ucdata = (unsigned char*)data;
	size = (len + 2) / 3;
	size *= 4;

	if((buf=new char[size+1]) == NULL) return NULL;
	
	memset(wp, 0, sizeof(wp));
	memset(buf, 0, size+1);
	for(i=0,p=buf; i<len; i++) {
		if(i%3 == 0 && i > 0) {
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = base64chars[wp[2]];
			*(p++) = base64chars[wp[3]];
			memset(wp, 0, sizeof(wp));
		}
		switch(i%3) {
			case 0:
				wp[0] =(unsigned char) ucdata[i] >> 2;
				wp[1] =(unsigned char) ucdata[i] << 6;
				wp[1] >>= 2;
			break;
			
			case 1:
				wp[1] |=(unsigned char) ucdata[i] >> 4;
				wp[2] =(unsigned char) ucdata[i] << 4;
				wp[2] >>= 2;
			break;
			
			case 2:
				wp[2] |=(unsigned char) ucdata[i] >> 6;
				wp[3] =(unsigned char) ucdata[i] & 0x3F;
			break;
		}
	}
	switch(len%3) {
		case 0:
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = base64chars[wp[2]];
			*p = base64chars[wp[3]];
		break;
		
		case 1:
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = '=';
			*p = '=';
		break;
		
		case 2:
			*(p++) = base64chars[wp[0]];
			*(p++) = base64chars[wp[1]];
			*(p++) = base64chars[wp[2]];
			*p = '=';
		break;
	}
	
	return buf;
}

int Base64Decode(const char* data, char** decdata)
{
	int size, len, no = 0;
	register int i;
	char *p, *buf, *cp;
	unsigned char wp[3];
	
	len = static_cast<int>(strlen(data));
	
	p = (char*)data + len - 1;
	while(*p == '=' && p > data) {
		p--;
		no++;
	}
	
	if(len < 1) return -1;
	
	size = ((len - no) * 6) / 8;
	
	if((buf=new char[size+no+1]) == NULL) return -1;
	
	memset(buf, 0, size+1);
	memset(wp, 0, sizeof(wp));
	for(i=0,p=buf; i<len; i++) {
		if(i%4 == 0 && i > 0) {
			*(p++) = wp[0];
			*(p++) = wp[1];
			*(p++) = wp[2];
			memset(wp, 0, sizeof(wp));
		}
		if((cp=(char*)strchr(base64chars, data[i])) == NULL) continue;
		switch(i%4) {
			case 0:
				wp[0] = (unsigned char) (cp - base64chars) << 2;
			break;
			
			case 1:
				wp[0] |= (unsigned char) ((cp - base64chars) >> 4);
				wp[1] = (unsigned char) (cp - base64chars) << 4;
			break;
			
			case 2:
				wp[1] |= (unsigned char) ((cp - base64chars) >> 2);
				wp[2] = (unsigned char) (cp - base64chars) << 6;
			break;
			
			case 3:
				wp[2] |= (unsigned char) (cp - base64chars);
			break;
		}
	}
	switch(len%4) {
		case 0:
			*(p++) = wp[0];
			*(p++) = wp[1];
			*p = wp[2];
		break;
		
		case 1:
			*p = wp[0];
		break;
		
		case 2:
			*(p++) = wp[0];
			*p = wp[1];
		break;
	}
	
	*decdata = buf;
	
	return size;

}
