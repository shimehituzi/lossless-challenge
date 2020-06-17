#include <stdio.h>
#include <stdlib.h>
#include "codec.h"

// 指定ビット分順に読み込む
uint getbits(FILE *fp, int n)
{
  static int bitpos = 0;
  static uint bitbuf = 0;
  int x = 0;

  if (n <= 0) return (0);
  while (n > bitpos) {
    n -= bitpos;
    x = (x << bitpos) | bitbuf;
    bitbuf = getc(fp) & 0xff;
    bitpos = 8;
  }
  bitpos -= n;
  x = (x << n) | (bitbuf >> bitpos);
  bitbuf &= ((1 << bitpos) - 1);
  return (x);
}

// デコーダーの設定
DECODER init_decoder(FILE *fp)
{
  DECODER dec;
  if (getbits(fp, 16) != MAGIC_NUMBER)
  {
    fprintf(stderr, "Not a compressed file!\n");
    exit(1);
  }
  dec.width = getbits(fp, 16);
  dec.height = getbits(fp, 16);
  dec.maxval = getbits(fp, 16);
  dec.filtered_val = (img_t **)alloc_2d_array(dec.height, dec.width, sizeof(img_t));
  int y, x;
  for(y = 0; y < dec.height; y++){
    for(x = 0; x < dec.width; x++){
      dec.filtered_val[y][x] = 0;
    }
  }

  dec.pmodel = (uint *)alloc_mem((dec.maxval + 1) * sizeof(int));
  dec.rc = rc_init();

  return (dec);
}

// 確率モデルの復号
void decode_pmodel(FILE *fp, DECODER *dec)
{
  for (int val = 0; val <= dec->maxval; val++)
  {
    dec->pmodel[val] = getbits(fp, 8);
  }
}

// 画像の復号
IMAGE *decode_image(FILE *fp, DECODER *dec)
{
  IMAGE *img = alloc_image(dec->width, dec->height, dec->maxval);
  PMODEL pm;
  pm.freq = dec->pmodel;
  pm.cumfreq = (uint *)alloc_mem((dec->maxval + 1) * sizeof(int));

  // 累積確率を計算
  pm.cumfreq[0] = dec->pmodel[0];
  for (int val = 0; val < dec->maxval + 1; val++)
  {
    pm.cumfreq[val + 1] = pm.cumfreq[val] + pm.freq[val];
  }

  // ラスタスキャン順に１画素ずつ復号
  for (int y = 0; y < dec->height; y++)
  {
    for (int x = 0; x < dec->width; x++)
    {
      dec->filtered_val[y][x] = rc_decode(fp, dec->rc, &pm, 0, dec->maxval + 1);
    }
  }

  return (img);
}

// フィルタを除去
void dpcm_remove(DECODER *dec, IMAGE *img)
{
  int y, x;
  int v1, v2, v3, v4;
  printf("荒木田");
  for(y = 0 ; y < dec->height; y++){
    for(x = 0 ; x < dec->width; x++){
      if(y == 0 && x == 0 ){
        v1 = dec->filtered_val[y][x];
        img->val[y][x] = v1;
      }
      else if(y == 0){
        v2 = dec->filtered_val[y][x] + img->val[y][x-1];
        img->val[y][x] = v2;
      }
      else if(x == 0){
        v3 = dec->filtered_val[y][x] + img->val[y-1][x];
        img->val[y][x] = v3;
      }
      else{
        v4 = dec->filtered_val[y][x] + img->val[y][x-1] + img->val[y-1][x] - img->val[y-1][x-1];
        img->val[y][x] = v4;
      }
    }
  }
}


int main(int argc, char **argv)
{
  IMAGE *img;
  DECODER dec;
  FILE *fp;
  char *infile, *outfile;
  infile = outfile = NULL;

  infile = argv[1];
  outfile = argv[2];

  // 読み込むファイルを設定
  fp = fileopen(infile, "rb");

  printf("*** start decoding ***\n");

  // 符号化済みファイルのヘッダーを読み込みデコーダーを初期化
  dec = init_decoder(fp);

  // 確率モデルを復号
  decode_pmodel(fp, &dec);

  // RangeCoderの準備
  rc_startdec(fp, dec.rc);

  // 画像を復号
  img = decode_image(fp, &dec);
  dpcm_remove(&dec, img);

  printf("decoding success.\n");

  // 復号した画像データをPGM形式で出力
  write_pgm(img, outfile);
  printf("\n");

  fclose(fp);
  free(img);
  free(dec.pmodel);
  free(dec.rc);

  return (0);
}
