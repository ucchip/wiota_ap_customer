#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

/**
*\file
* \brief Implementation specific datatype

 */
typedef enum
{
    FALSE = 0,
    TRUE
} Boolean;
typedef char Octet;
typedef signed char Integer8;
typedef signed short Integer16;
typedef signed int Integer32;
typedef unsigned char UInteger8;
typedef unsigned short UInteger16;
typedef unsigned int UInteger32;
typedef unsigned short Enumeration16;
typedef unsigned char Enumeration8;
typedef unsigned char Enumeration4;
typedef unsigned char UInteger4;
typedef unsigned char Nibble;
typedef unsigned int size_t;

/**
* \brief Implementation specific of UInteger48 type
 */
typedef struct
{
    unsigned int lsb;
    unsigned short msb;
} UInteger48;

/**
* \brief Implementation specific of Integer64 type
 */
typedef struct
{
    unsigned int lsb;
    int msb;
} Integer64;

/**
* \brief Struct used to average the offset from master
*
* The FIR filtering of the offset from master input is a simple, two-sample average
 */
typedef struct
{
    Integer32 nsec_prev, y;
} offset_from_master_filter;

/**
* \brief Struct used to average the one way delay
*
* It is a variable cutoff/delay low-pass, infinite impulse response (IIR) filter.
*
*  The one-way delay filter has the difference equation: s*y[n] - (s-1)*y[n-1] = x[n]/2 + x[n-1]/2, where increasing the stiffness (s) lowers the cutoff and increases the delay.
 */
typedef struct
{
    Integer32 nsec_prev, y;
    Integer32 s_exp;
} one_way_delay_filter;

/**
* \brief Struct used to store network datas
 */
typedef struct
{
    Integer32 eventSock, generalSock, multicastAddr, peerMulticastAddr, unicastAddr;
} NetPath;

struct cmsghdr
{
    socklen_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

struct iovec
{
    /* Starting address (内存起始地址）*/
    void *iov_base;

    /* Number of bytes to transfer（这块内存长度） */
    size_t iov_len;
};

struct msghdr
{

    void *msg_name;

    socklen_t msg_namelen;

    struct iovec *msg_iov;

    int msg_iovlen;

    void *msg_control;

    socklen_t msg_controllen;

    int msg_flags;
};

extern struct cmsghdr *__cmsg_nxthdr(struct msghdr *__mhdr, struct cmsghdr *__cmsg);

#define CMSG_DATA(cmsg) ((unsigned char *)((struct cmsghdr *)(cmsg) + 1))
#define CMSG_NXTHDR(mhdr, cmsg) __cmsg_nxthdr(mhdr, cmsg)
#define CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & (size_t) ~(sizeof(size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN(len) + CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#define CMSG_FIRSTHDR(mhdr) ((size_t)(mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? (struct cmsghdr *)(mhdr)->msg_control : (struct cmsghdr *)0)

#define SO_TIMESTAMP 29
#define SCM_TIMESTAMP SO_TIMESTAMP

#define MSG_CTRUNC 0x08
#define MSG_TRUNC 0x20

typedef long __kernel_long_t;

struct timex
{
    unsigned int modes;        /* mode selector */
    __kernel_long_t offset;    /* time offset (usec) */
    __kernel_long_t freq;      /* frequency offset (scaled ppm) */
    __kernel_long_t maxerror;  /* maximum error (usec) */
    __kernel_long_t esterror;  /* estimated error (usec) */
    int status;                /* clock command/status */
    __kernel_long_t constant;  /* pll time constant */
    __kernel_long_t precision; /* clock precision (usec) (read only) */
    __kernel_long_t tolerance; /* clock frequency tolerance (ppm)
				   * (read only)
				   */
    struct timeval time;       /* (read only, except for ADJ_SETOFFSET) */
    __kernel_long_t tick;      /* (modified) usecs between clock ticks */

    __kernel_long_t ppsfreq; /* pps frequency (scaled ppm) (ro) */
    __kernel_long_t jitter;  /* pps jitter (us) (ro) */
    int shift;               /* interval duration (s) (shift) (ro) */
    __kernel_long_t stabil;  /* pps stability (scaled ppm) (ro) */
    __kernel_long_t jitcnt;  /* jitter limit exceeded (ro) */
    __kernel_long_t calcnt;  /* calibration intervals (ro) */
    __kernel_long_t errcnt;  /* calibration errors (ro) */
    __kernel_long_t stbcnt;  /* stability limit exceeded (ro) */

    int tai; /* TAI offset (ro) */

    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
};

#if 0
struct itimerval {
	struct timeval it_interval;/* timer interval */
	struct timeval it_value;	/* current value */
};
#endif
#endif /*DATATYPES_DEP_H_*/
