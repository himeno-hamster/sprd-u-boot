#include <common.h>
# define ULONG_MAX	0xffffffffUL	/* max value for an unsigned long */
# define LONG_MAX	0x7fffffffL	/* max value for a long */
# define LONG_MIN	(-0x7fffffffL-1)/* min value for a long */


#define CTYPE_NUM_CHARS       256
#define	_U	0x01
#define	_L	0x02
#define	_N	0x04
#define	_S	0x08
#define	_P	0x10
#define	_C	0x20
#define	_X	0x40
#define	_B	0x80
const char _C_ctype_[1 + CTYPE_NUM_CHARS] = {
	0,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_S|(char)_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N,
	_N,	_N,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P,
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	/* determine printability based on the IS0 8859 8-bit standard */
	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C,

	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, /* 80 */
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, /* 88 */
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, /* 90 */
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, /* 98 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* A0 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* A8 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* B0 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* B8 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* C0 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* C8 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* D0 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* D8 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* E0 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* E8 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, /* F0 */
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P  /* F8 */
};

const char *_ctype_ = _C_ctype_;


#if 1 /* ndef NDEBUG */
int isalnum(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_U|_L|_N)));
}

int isalpha(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_U|_L)));
}

int iscntrl(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _C));
}

int isdigit(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _N));
}

int isgraph(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N)));
}

int islower(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _L));
}

int isprint(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N|_B)));
}

int ispunct(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _P));
}

int isspace(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _S));
}

int isupper(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _U));
}

int isxdigit(int c)
{
		return (c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_N|_X)));
}

#if __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __POSIX_VISIBLE > 200112 \
    || __XPG_VISIBLE > 600
	int isblank(int c)
	{
			return (c == ' ' || c == '\t');
	}
#endif

#if __BSD_VISIBLE || __XPG_VISIBLE
int isascii(int c)
{
		return ((unsigned int)c <= 0177);
}

int toascii(int c)
{
		return (c & 0177);
}

#endif /* __BSD_VISIBLE || __XPG_VISIBLE */

#endif /* !NDBEUG */

/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long
strtol(const char *nptr, char **endptr, int base)
{
	const char *s;
	long acc, cutoff;
	int c;
	int neg, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? LONG_MIN : LONG_MAX;
	cutlim = cutoff % base;
	cutoff /= base;
	if (neg) {
		if (cutlim > 0) {
			cutlim -= base;
			cutoff += 1;
		}
		cutlim = -cutlim;
	}
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (neg) {
			if (acc < cutoff || (acc == cutoff && c > cutlim)) {
				any = -1;
				acc = LONG_MIN;
				//errno = ERANGE;
			} else {
				any = 1;
				acc *= base;
				acc -= c;
			}
		} else {
			if (acc > cutoff || (acc == cutoff && c > cutlim)) {
				any = -1;
				acc = LONG_MAX;
				//errno = ERANGE;
			} else {
				any = 1;
				acc *= base;
				acc += c;
			}
		}
	}
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}


/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
	const char *s;
	unsigned long acc, cutoff;
	int c;
	int neg, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	cutoff = ULONG_MAX / (unsigned long)base;
	cutlim = ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
			acc = ULONG_MAX;
			//errno = ERANGE;
		} else {
			any = 1;
			acc *= (unsigned long)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}


typedef uint32_t  uintmax_t;
#  define UINT32_MAX	(4294967295U)
#  define  UINTMAX_MAX   UINT32_MAX
/*
 * Convert a string to a uintmax_t.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
uintmax_t
strtoumax(const char *nptr, char **endptr, int base)
{
	const char *s;
	uintmax_t acc, cutoff;
	int c;
	int neg, any, cutlim;

	/*
	 * See strtoq for comments as to the logic used.
	 */
	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

        /* BIONIC: avoid division and modulo for common cases */
#define  CASE_BASE(x)                            \
            case x: cutoff = UINTMAX_MAX / x;    \
	            cutlim = UINTMAX_MAX % x;    \
		    break

        switch (base) {
        CASE_BASE(8);
	CASE_BASE(10);
	CASE_BASE(16);
	default:
	    cutoff = UINTMAX_MAX / base;
	    cutlim = UINTMAX_MAX % base;
	}

	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
			acc = UINTMAX_MAX;
			//errno = ERANGE;
		} else {
			any = 1;
			acc *= (uintmax_t)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

unsigned long long
strtoull(const char *nptr, char **endptr, int base)
{
    return (unsigned long long)strtoumax(nptr, endptr, base);
}

