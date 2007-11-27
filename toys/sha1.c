/*
 * Based on the public domain SHA-1 in C by Steve Reid <steve@edmweb.com>
 * from http://www.mirrors.wiretapped.net/security/cryptography/hashes/sha1/
 */

/* #define LITTLE_ENDIAN * This should be #define'd if true. */
/* #define SHA1HANDSOFF * Copies data before messing with it. */
#define	LITTLE_ENDIAN

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

struct sha1 {
	uint32_t state[5];
	uint32_t oldstate[5];
	uint64_t count;
	union {
		unsigned char c[64];
		uint32_t i[16];
	} buffer;
};

void sha1_init(struct sha1 *this);
void sha1_transform(struct sha1 *this);
void sha1_update(struct sha1 *this, unsigned char *data, unsigned int len);
void sha1_final(struct sha1 *this, unsigned char digest[20]);

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#ifdef LITTLE_ENDIAN
#define blk0(i) (block[i] = (rol(block[i],24)&0xFF00FF00) \
	|(rol(block[i],8)&0x00FF00FF))
#else
#define blk0(i) block[i]
#endif
#define blk(i) (block[i&15] = rol(block[(i+13)&15]^block[(i+8)&15] \
	^block[(i+2)&15]^block[i&15],1))

static const uint32_t rconsts[]={0x5A827999,0x6ED9EBA1,0x8F1BBCDC,0xCA62C1D6};

void printy(unsigned char *this)
{
	int i;

	for (i = 0; i < 20; i++) printf("%02x", this[i]);
	putchar('\n');
}

/* Hash a single 512-bit block. This is the core of the algorithm. */

void sha1_transform(struct sha1 *this)
{
	int i, j, k, count;
	uint32_t *block = this->buffer.i;
	uint32_t *rot[5], *temp;

	/* Copy context->state[] to working vars */
	for (i=0; i<5; i++) {
		this->oldstate[i] = this->state[i];
		rot[i] = this->state + i;
	}
	/* 4 rounds of 20 operations each. */
	for (i=count=0; i<4; i++) {
		for (j=0; j<20; j++) {
			uint32_t work;

			work = *rot[2] ^ *rot[3];
			if (!i) work = (work & *rot[1]) ^ *rot[3];
			else {
				if (i==2)
					work = ((*rot[1]|*rot[2])&*rot[3])|(*rot[1]&*rot[2]);
				else work ^= *rot[1];
			}
			if (!i && j<16) work += blk0(count);
			else work += blk(count);
			*rot[4] += work + rol(*rot[0],5) + rconsts[i];
			*rot[1] = rol(*rot[1],30);

			// Rotate by one for next time.
			temp = rot[4];
			for (k=4; k; k--) rot[k] = rot[k-1];
			*rot = temp;
			count++;
		}
	}
	/* Add the previous values of state[] */
	for (i=0; i<5; i++) this->state[i] += this->oldstate[i];
}


/* SHA1Init - Initialize new context */

void sha1_init(struct sha1 *this)
{
	/* SHA1 initialization constants */
	this->state[0] = 0x67452301;
	this->state[1] = 0xEFCDAB89;
	this->state[2] = 0x98BADCFE;
	this->state[3] = 0x10325476;
	this->state[4] = 0xC3D2E1F0;
	this->count = 0;
}

/* Run your data through this function. */

void sha1_update(struct sha1 *this, unsigned char *data, unsigned int len)
{
	unsigned int i, j;

	j = this->count & 63;
	this->count += len;

	// Enough data to process a frame?
	if ((j + len) > 63) {
		i = 64-j;
		memcpy(this->buffer.c + j, data, i);
		sha1_transform(this);
		for ( ; i + 63 < len; i += 64) {
			memcpy(this->buffer.c, data + i, 64);
			sha1_transform(this);
		}
		j = 0;
	} else i = 0;
	// Grab remaining chunk
	memcpy(this->buffer.c + j, data + i, len - i);
}

/* Add padding and return the message digest. */

void sha1_final(struct sha1 *this, unsigned char digest[20])
{
	uint64_t count = this->count << 3;
	unsigned int i;
	unsigned char buf;

	// End the message by appending a "1" bit to the data, ending with the
	// message size (in bits, big endian), and adding enough zero bits in
	// between to pad to the end of the next 64-byte frame.  Since our input
	// up to now has been in whole bytes, we can deal with bytes here too.

	buf = 0x80;
	do {
		sha1_update(this, &buf, 1);
		buf = 0;
	} while ((this->count & 63) != 56);
	for (i = 0; i < 8; i++)
	  this->buffer.c[56+i] = count >> (8*(7-i));
	sha1_transform(this);

	for (i = 0; i < 20; i++) {
		digest[i] = (unsigned char)
		 ((this->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
	}
	/* Wipe variables */
	memset(this, 0, sizeof(struct sha1));
}


/*************************************************************/


int main(int argc, char** argv)
{
	int i, j;
	struct sha1 this;
	unsigned char digest[20], buffer[16384];
	FILE* file;

	if (argc < 2) {
		file = stdin;
	}
	else {
		if (!(file = fopen(argv[1], "rb"))) {
			fputs("Unable to open file.", stderr);
			exit(-1);
		}
	}
	sha1_init(&this);
	while (!feof(file)) {  /* note: what if ferror(file) */
		i = fread(buffer, 1, 16384, file);
		sha1_update(&this, buffer, i);
	}
	sha1_final(&this, digest);
	fclose(file);
	printy(digest);
	exit(0);
}