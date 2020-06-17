#include <stdio.h>
#define MAGIC_NUMBER ('M' << 8) + 'R'
#define HAVE_64BIT_INTEGER
typedef unsigned long size_t;
typedef unsigned char img_t;
typedef unsigned int uint;
#define PMODEL_PRECISION 256
#define BASE_BSIZE 8
#ifdef HAVE_64BIT_INTEGER
#define RANGE_SIZE 64
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define range_t unsigned __int64
#else
#define range_t unsigned long long
#endif
#define MAX_TOTFREQ (1 << 20) /* must be < RANGE_BOT */
#else
#define RANGE_SIZE 32
#define range_t unsigned int
#define MAX_TOTFREQ (1 << 14) /* must be < RANGE_BOT */
#endif
#define RANGE_TOP ((range_t)1 << (RANGE_SIZE - 8))
#define RANGE_BOT ((range_t)1 << (RANGE_SIZE - 16))

typedef struct
{
  int height;
  int width;
  int maxval;
  img_t **val;
} IMAGE;

typedef struct
{
  uint *freq;
  uint *cumfreq;
} PMODEL;

typedef struct
{
  range_t low;
  range_t code;
  range_t range;
} RANGECODER;

typedef struct
{
  int width;
  int height;
  int maxval;
  img_t **filtered_val;
  int *histogram;
  uint *pmodel;
  int pmodel_precision;
  RANGECODER *rc;
} ENCODER;

typedef struct
{
  int width;
  int height;
  int maxval;
  uint *pmodel;
  RANGECODER *rc;
} DECODER;

/* encoder.c */
ENCODER init_encoder(IMAGE *);
int putbits(FILE *, int, uint);
int write_header(ENCODER *, FILE *);
void calc_histogram(ENCODER *);
void generate_pmodel(ENCODER *);
int encode_pmodel(ENCODER *, FILE *);
int encode_image(ENCODER *, FILE *);

/* decoder.c */
DECODER init_decoder(FILE *);
uint getbits(FILE *, int);
void decode_pmodel(FILE *, DECODER *);
IMAGE *decode_image(FILE *, DECODER *);

/* common.c */
FILE *fileopen(char *, char *);
void *alloc_mem(size_t);
void **alloc_2d_array(int, int, size_t);
void ***alloc_3d_array(int, int, int, size_t);
IMAGE *alloc_image(int, int, int);
IMAGE *read_pgm(char *);
void write_pgm(IMAGE *, char *);

/* rc.c */
RANGECODER *rc_init(void);
void rc_encode(FILE *, RANGECODER *, uint, uint, uint);
void rc_finishenc(FILE *, RANGECODER *);
int rc_decode(FILE *, RANGECODER *, PMODEL *, int, int);
void rc_startdec(FILE *, RANGECODER *);
