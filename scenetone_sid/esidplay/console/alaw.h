/**
 * @file alaw.h Defines the static Alaw utility class.
 */
#if !defined(ALAW_H__)
#define ALAW_H__


/**
 * Static utility class for converting between Alaw and linear PCM sound
 */
class Alaw
	{
public:
	static void conv_u8bit_alaw(unsigned char *src_ptr, unsigned char *dst_ptr, size_t size);
	static void conv_s8bit_alaw(unsigned char *src_ptr, unsigned char *dst_ptr, size_t size);
	static void conv_s16bit_alaw(unsigned short *src_ptr, unsigned char *dst_ptr, size_t size);
	static void conv_u16bit_alaw(unsigned short *src_ptr, unsigned char *dst_ptr, size_t size);
private:
	static inline unsigned char linear2alaw(int pcm_val);
	static inline int search(int val, const short *table, int size);
	};

#endif // ALAW_H__


