#include <stdio.h>
#include <stdlib.h>
#include "codec.h"

FILE *fileopen(char *filename, char *mode)
{
  FILE *fp;
  fp = fopen(filename, mode);
  if (fp == NULL) {
    fprintf(stderr, "Can\'t open %s!\n", filename);
    exit(1);
  }
  return (fp);
}

// 動的配列を生成
void *alloc_mem(size_t size)
{
  void *ptr;
  if ((ptr = (void *)malloc(size)) == NULL)
  {
    fprintf(stderr, "Can't allocate memory (size = %u)!\n", (int)size);
    exit(1);
  }
  return (ptr);
}
// 動的二次元配列を生成
void **alloc_2d_array(int height, int width, size_t size)
{
  void **mat;
  void *ptr;
  int k;

  mat = (void **)alloc_mem(sizeof(*mat) * height + height * width * size);
  ptr = (void *)(mat + height);
  for (k = 0; k < height; k++)
  {
    mat[k] = ptr;
    ptr += width * size;
  }
  return (mat);
}
// 動的三次元配列を生成
void ***alloc_3d_array(int height, int width, int depth, size_t size)
{
  void ***mat, **ptr2;
  void *ptr;
  int t, k, l;
  t = size * depth;

  mat = (void ***)alloc_mem((sizeof(*mat) + sizeof(**mat) * width + t * width) * height);
  ptr2 = (void **)(mat + height);
  ptr = (void *)(ptr2 + height * width);

  for (k = 0; k < height; k++)
  {
    mat[k] = ptr2;
    for (l = 0; l < width; l++)
    {
      ptr2[l] = ptr;
      ptr += t;
    }
    ptr2 += width;
  }
  return (mat);
}

// IMAGEオブジェクトを生成
IMAGE *alloc_image(int width, int height, int maxval)
{
  int y,x;
  IMAGE *img;
  img = (IMAGE *)alloc_mem(sizeof(IMAGE));
  img->width = width;
  img->height = height;
  img->maxval = maxval;
  img->val = (img_t **)alloc_2d_array(img->height, img->width, sizeof(img_t));
  for(y = 0; y < img->height; y++){
    for(x = 0; x < img->width; x++){
      img->val[y][x] = 0;
    }
  }
  return (img);
}

// 画像を読み込む
IMAGE *read_pgm(char *filename)
{
  int i, j, width, height, maxval;
  char tmp[256];
  IMAGE *img;
  FILE *fp;

  fp = fileopen(filename, "rb");
  fgets(tmp, 256, fp);
  if (tmp[0] != 'P' || tmp[1] != '5')
  {
    fprintf(stderr, "Not a PGM file!\n");
    exit(1);
  }
  while (*(fgets(tmp, 256, fp)) == '#')
    ;
  sscanf(tmp, "%d %d", &width, &height);
  while (*(fgets(tmp, 256, fp)) == '#')
    ;
  sscanf(tmp, "%d", &maxval);
  if ((width % BASE_BSIZE) || (height % BASE_BSIZE))
  {
    fprintf(stderr, "Image width and height must be multiples of %d!\n",
            BASE_BSIZE);
    exit(1);
  }
  if (maxval > 255)
  {
    fprintf(stderr, "Sorry, this version only supports 8bpp images!\n");
    exit(1);
  }
  img = alloc_image(width, height, maxval);
  for (i = 0; i < img->height; i++)
  {
    for (j = 0; j < img->width; j++)
    {
      img->val[i][j] = (img_t)fgetc(fp);
    }
  }
  fclose(fp);
  return (img);
}

// 画像を出力する
void write_pgm(IMAGE *img, char *filename)
{
  printf("*** write pgm ***\n");
  int i, j;
  FILE *fp;
  fp = fileopen(filename, "wb");
  fprintf(fp, "P5\n%d %d\n%d\n", img->width, img->height, img->maxval);
  for (i = 0; i < img->height; i++)
  {
    for (j = 0; j < img->width; j++)
    {
      putc(img->val[i][j], fp);
    }
  }
  fclose(fp);
  printf("write pgm success.\n");
  return;
}