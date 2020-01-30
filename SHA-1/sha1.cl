/* 
   This code was largely inspired by 
   pyrit opencl kernel sha1 routines.
   Copyright 2011 by chucheng 
   zunceng at gmail dot com
   This program comes with ABSOLUTELY NO WARRANTY; express or
   implied .
   This is free software, and you are welcome to redistribute it
   under certain conditions; as expressed here 
   http://www.gnu.org/licenses/gpl-2.0.html
*/

#ifdef cl_khr_byte_addressable_store
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : disable
#endif

#ifdef cl_nv_pragma_unroll
#define NVIDIA
#pragma OPENCL EXTENSION cl_nv_pragma_unroll : enable
#endif

#ifdef NVIDIA
inline uint SWAP32(uint x)
{
	x = rotate(x, 16U);
	return ((x & 0x00FF00FF) << 8) + ((x >> 8) & 0x00FF00FF);
}
#else
#define SWAP32(a)	(as_uint(as_uchar4(a).wzyx))
#endif

#define K0  0x5A827999
#define K1  0x6ED9EBA1
#define K2  0x8F1BBCDC
#define K3  0xCA62C1D6

#define H1 0x67452301
#define H2 0xEFCDAB89
#define H3 0x98BADCFE
#define H4 0x10325476
#define H5 0xC3D2E1F0

#ifndef uint32_t
#define uint32_t unsigned int
#endif

uint32_t SHA1CircularShift(int bits, uint32_t word) {
	return ((word << bits) & 0xFFFFFFFF) | (word) >> (32 - (bits));
}

void calcHash(__global char *plain_key, const uint ulen, uint *digest) {

	int t, msg_pad;
    int stop, mmod;
    uint i, item, total;
    uint W[80], temp, A,B,C,D,E;
	int current_pad;

	msg_pad=0;

	total = ulen%64>=56?2:1 + ulen/64;

	//printf("ulen: %u total:%u\n", ulen, total);

    digest[0] = 0x67452301;
	digest[1] = 0xEFCDAB89;
	digest[2] = 0x98BADCFE;
	digest[3] = 0x10325476;
	digest[4] = 0xC3D2E1F0;
	for(item=0; item<total; item++)
	{

		A = digest[0];
		B = digest[1];
		C = digest[2];
		D = digest[3];
		E = digest[4];

	#pragma unroll
		for (t = 0; t < 80; t++){
		W[t] = 0x00000000;
		}
		msg_pad=item*64;
		if(ulen > msg_pad)
		{
			current_pad = (ulen-msg_pad)>64?64:(ulen-msg_pad);
		}
		else
		{
			current_pad =-1;		
		}

		//printf("current_pad: %d\n",current_pad);
		if(current_pad>0)
		{
			i=current_pad;

			stop =  i/4;
			//printf("i:%d, stop: %d msg_pad:%d\n",i,stop, msg_pad);
			for (t = 0 ; t < stop ; t++){
				W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
				W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 1]) << 16;
				W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 2]) << 8;
				W[t] |= (uchar)  plain_key[msg_pad + t * 4 + 3];
				//printf("W[%u]: %u\n",t,W[t]);
			}
			mmod = i % 4;
			if ( mmod == 3){
				W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
				W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 1]) << 16;
				W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 2]) << 8;
				W[t] |=  ((uchar) 0x80) ;
			} else if (mmod == 2) {
				W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
				W[t] |= ((uchar) plain_key[msg_pad + t * 4 + 1]) << 16;
				W[t] |=  0x8000 ;
			} else if (mmod == 1) {
				W[t] = ((uchar)  plain_key[msg_pad + t * 4]) << 24;
				W[t] |=  0x800000 ;
			} else /*if (mmod == 0)*/ {
				W[t] =  0x80000000 ;
			}
			
			if (current_pad<56)
			{
				W[15] =  ulen*8 ;
				//printf("w[15] :%u\n", W[15]);
			}
		}
		else if(current_pad <0)
		{
			if( ulen%64==0)
				W[0]=0x80000000;
			W[15]=ulen*8;
			//printf("w[15] :%u\n", W[15]);
		}

		for (t = 16; t < 80; t++)
		{
			W[t] = SHA1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
		}

		for (t = 0; t < 20; t++)
		{
			temp = SHA1CircularShift(5, A) +
				((B & C) | ((~B) & D)) + E + W[t] + K0;
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = SHA1CircularShift(30, B);
			B = A;
			A = temp;
		}

		for (t = 20; t < 40; t++)
		{
			temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K1;
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = SHA1CircularShift(30, B);
			B = A;
			A = temp;
		}

		for (t = 40; t < 60; t++)
		{
			temp = SHA1CircularShift(5, A) +
				((B & C) | (B & D) | (C & D)) + E + W[t] + K2;
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = SHA1CircularShift(30, B);
			B = A;
			A = temp;
		}

		for (t = 60; t < 80; t++)
		{
			temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K3;
			temp &= 0xFFFFFFFF;
			E = D;
			D = C;
			C = SHA1CircularShift(30, B);
			B = A;
			A = temp;
		}

		digest[0] = (digest[0] + A) & 0xFFFFFFFF;
		digest[1] = (digest[1] + B) & 0xFFFFFFFF;
		digest[2] = (digest[2] + C) & 0xFFFFFFFF;
		digest[3] = (digest[3] + D) & 0xFFFFFFFF;
		digest[4] = (digest[4] + E) & 0xFFFFFFFF;

		//for(i=0;i<80;i++)
			//printf("W[%u]: %u\n", i,W[i] );

		//printf("%u\n",  digest[0]);
		//printf("%u\n",  digest[1]);
		//printf("%u\n",  digest[2]);
		//printf("%u\n",  digest[3]);
		//printf("%u\n",  digest[4]);
	}
}

__kernel void breakHash(__global const uint* desiredHash, __global const char* passwords, __global const size_t* passwordFileSize, __global const int* passwordNumber, __global const int* passwordSizes, __global const int* passwordIndeces, __global int* retIndex) {
	
	// Get the global id of the current thread.
	int gId = get_global_id(0);
	if (gId >= *passwordNumber)
		return;
	
	// Declare the variables.
	uint result[5];
	int index = passwordIndeces[gId];
	int len = passwordSizes[gId];
	
	// Calculate the hash of the current thread.
	calcHash(passwords + index, (uint)len, result);

	// Check if the hash was found.
	int found = 1;
	for(int i = 0; i < 5; i++) {
		if (result[i] != desiredHash[i]) {
			found = 0;
			break;
		}
	}

	// If it was found, save it to the return variable.
	if (found == 1)
		*retIndex = gId;
}
