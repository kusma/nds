#include <assert.h>
#define ASSERT assert
#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
	
#define FRCP_LUT_LOG2_SIZE 10
#define FRCP_LUT_SIZE (1UL << (FRCP_LUT_LOG2_SIZE))
unsigned short frcp_lut[FRCP_LUT_SIZE];

void frcp_init()
{
	int i;
	int max = 0;
	for (i = 0; i < FRCP_LUT_SIZE; ++i)
	{
		float fmantissa = (float)i / FRCP_LUT_SIZE;
		fmantissa = (1.0f / (fmantissa + 1.0f)) * 2 - 1.0f;
		fmantissa = MIN(fmantissa, 1.0f - 1e-7);
		int mantissa = (int)floor(fmantissa * (1 << 23)) >> 8;
		frcp_lut[i] = mantissa;
		if (mantissa > max) max = mantissa;
	}
	printf("%X\n", max);
}

float frcp(float in)
{
#if 0
	/* naive implementation */
	int exponent = 0;
	float mantissa = frexp(in, &exponent);
	
	/* simulate a 512 element LUT */
	mantissa = floor(mantissa * 512) / 512;
	
	/* adjust mantissa and exponent */
	mantissa = 1.0f / mantissa;
	exponent = -exponent;
	
	return ldexp(mantissa, exponent);
#else
	/* let's decode it ourselves */
	union {
		float f;
		unsigned int u;
	} c;
	c.f = in;
	
	/* extract exponent and mantissa */
	unsigned int exponent = (c.u >> 23) & 0xFF;
	unsigned int mantissa = c.u & ((1 << 23) - 1);

	/* check if we will underflow the exponent, and just return 0.0f */
	if (exponent > 0xFF - 2) return 0.0f;
	
	/* adjust exponent */
	exponent ^= 0xFF;
	exponent -= 2;
	ASSERT(exponent <= 0xFF);
	
#if 0
	/* these four lines should go into a LUT */
	float fmantissa  = (float)mantissa / (1 << 23);
//	fmantissa = floor(fmantissa * 512) / 512; /* simulate a 512 element LUT */
	fmantissa = (1.0f / (fmantissa + 1.0f)) * 2 - 1.0f;
	fmantissa = MIN(fmantissa, 1.0f - 1e-7);
	mantissa = (int)(fmantissa * (1 << 23));
#else
	mantissa = frcp_lut[mantissa >> (23 - FRCP_LUT_LOG2_SIZE)] << 8;
#endif
	
	c.u = c.u & (1 << 31); /* clear everything except the sign-bit */
	c.u |= exponent << 23;
	c.u |= mantissa;
	return c.f;
#endif
}

#define FRSQ_LUT_LOG2_SIZE 10
#define FRSQ_LUT_SIZE (1UL << (FRSQ_LUT_LOG2_SIZE))
unsigned int frsq_lut[FRSQ_LUT_SIZE];

void frsq_init()
{
	int i;
	for (i = 0; i < FRSQ_LUT_SIZE; ++i)
	{
		double fmantissa = (float)i / FRSQ_LUT_SIZE;
		fmantissa = 2.0f / sqrt(fmantissa + 1.0f);
		fmantissa = MIN(fmantissa, 2.0f - 1e-7);
		int mantissa = (int)((fmantissa) * (1 << 23));
		ASSERT(i < ARRAY_SIZE(frsq_lut));
		frsq_lut[i] = mantissa;
	}
}

float frsq(float in)
{
#if 0
	/* naive implementation */
	int exponent = 0;
	float mantissa = frexp(in, &exponent);
//	mantissa = floor(mantissa * 1024) / 1024; /* simulate a 1024 element LUT */
	
	/* adjust mantissa and exponent */
	if (0 != (exponent & 1)) mantissa *= 2;
	mantissa = 1.0f / sqrt(mantissa); /* TODO: LUT */
	exponent = -(exponent >> 1);
	
	return ldexp(mantissa, exponent);
#else

	union {
		float f;
		unsigned int u;
	} c;
	c.f = in;
	ASSERT(0 == (c.u & (1 << 31))); /* sqrt(-x) is no good, booooy */
	
	/* unpack exponent and mantissa */
	int exponent = (c.u >> 23) - 127;
	unsigned int mantissa = (c.u & ((1 << 23) - 1)); // | (1 << 23);

	/* adjust mantissa and exponent - require twice the range! */
#if 0
	if (0 != (exponent & 1)) mantissa *= 2;
	exponent = -(exponent >> 1) - 1;
#endif

#if 0
	mantissa >>= 23 - FRSQ_LUT_LOG2_SIZE;
	mantissa <<= 23 - FRSQ_LUT_LOG2_SIZE;

	float fmantissa = (float)mantissa / (1 << 23);
	fmantissa = 2.0f / sqrt(fmantissa);
	fmantissa = MIN(fmantissa, 2.0f - 1e-7);
	mantissa = (int)((fmantissa) * (1 << 23));
#else
	int index = mantissa >> (23 - FRSQ_LUT_LOG2_SIZE);
	ASSERT(index < ARRAY_SIZE(frsq_lut));
	mantissa = frsq_lut[index];
#endif

#if 1
	/* adjust mantissa and exponent */
	if (0 != (exponent & 1)) mantissa = mantissa * 0xb5 >> 8; /* since we're post-multiplying, we need to adjust for sqrt(0.5) ~= 0.70703125 = 0xb5 */
	exponent = -(exponent >> 1) - 1;
#endif
	/* repack float - discard sign-bit, we don't care about negative values */
	c.u = (exponent + 127) << 23;
	c.u |= mantissa & ((1 << 23) - 1);
	return c.f;
	
#endif
}

float frcp_ref(float in)
{
	return 1.0f / in;
}

float frsq_ref(float in)
{
	return 1.0f / sqrt(in);
}


int values_tested = 0;
int values_failed = 0;
float max_rel_err = 0.0f;

void test_rcp(int exponent, int negative)
{
	double min  = ldexp(1.0f, exponent);
	double max  = ldexp(2.0f, exponent);
	double step = ldexp(1e-5, exponent);
	double f;
	for (f = min; f < max; f += step)
	{
		float val = negative ? -f : f;
		float rcp     = frcp(val);
		float rcp_ref = frcp_ref(val);
		float rel_err = fabs((rcp - rcp_ref) / rcp_ref);
		
		if (rel_err > max_rel_err) max_rel_err = rel_err;
		
		if (rel_err > 1e-3)
		{
			printf("1.0 / %f = %f, %f, rel err: %f\n", val, rcp, rcp_ref, rel_err);
			values_failed++;
		}
		values_tested++;
	}
}

#include <float.h>
int main()
{
	int exponent;
	float f;
	frcp_init();
	frsq_init();
	
	puts("testing frsq()...");
	values_failed = values_tested = 0;
	max_rel_err = 0.0f;
	for (f = 1.0f; f < 100.0f; f += 0.0001f)
	{
		float val = f;
		float rsq     = frsq(val);
		float rsq_ref = frsq_ref(val);

		float rel_err = fabs((rsq - rsq_ref));
		if (rel_err > max_rel_err) max_rel_err = rel_err;
		
		if (rel_err > 1e-3)
		{
			printf("1.0 / sqrt(%f) = %f, %f, rel err: %f\n", val, rsq, rsq_ref, rel_err);
			values_failed++;
		}
		values_tested++;
	}
	printf("max_rel_err: %f\n", max_rel_err);
	printf("%d failed out of %d\n\n", values_failed, values_tested);

//	for (f = 0.0f; f < 0.1f; f += 0.001f)

	puts("testing frcp()...");
	values_failed = values_tested = 0;
	max_rel_err = 0.0f;
	for (exponent = -126; exponent < 31; ++exponent)
	{
		test_rcp(exponent, 0);
		test_rcp(exponent, 1);
	}
	printf("max_rel_err: %f\n", max_rel_err);
	printf("%d failed out of %d\n", values_failed, values_tested);
	return 0;
}
