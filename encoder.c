#include <stdio.h>
#include <stdlib.h>
#include "codec.h"

// 指定ビットだけ書き込む
int putbits(FILE *fp, int n, uint x)
{
  static int bitpos = 8;
  static uint bitbuf = 0;
  int bits;

  bits = n;
  if (bits <= 0)
    return (0);
  while (n >= bitpos)
  {
    n -= bitpos;
    if (n < 32)
    {
      bitbuf |= ((x >> n) & (0xff >> (8 - bitpos)));
    }
    putc(bitbuf, fp);
    bitbuf = 0;
    bitpos = 8;
  }
  bitpos -= n;
  bitbuf |= ((x & (0xff >> (8 - n))) << bitpos);
  return (bits);
}

// エンコーダーを設定
ENCODER init_encoder(IMAGE *img)
{
  ENCODER enc;
  enc.height = img->height;
  enc.width = img->width;
  enc.maxval = img->maxval;
  enc.histogram = (int *)alloc_mem((img->maxval + 1) * sizeof(int));
  enc.pmodel = (uint *)alloc_mem((img->maxval + 1) * sizeof(int));
  enc.pmodel_precision = PMODEL_PRECISION;
  enc.rc = rc_init();

  for (int val = 0; val <= enc.maxval; ++val)
  {
    enc.histogram[val] = 0;
    enc.pmodel[val] = 0;
  }

  return (enc);
}

// 復号に必要な情報をヘッダーに書き込む
int write_header(ENCODER *enc, FILE *fp)
{
  int bits;
  bits = putbits(fp, 16, MAGIC_NUMBER);
  bits += putbits(fp, 16, enc->width);
  bits += putbits(fp, 16, enc->height);
  bits += putbits(fp, 16, enc->maxval);

  return (bits);
}

// 画像のヒストグラムを計算する
void calc_histogram(ENCODER *enc, IMAGE *img)
{
  for (int h = 0; h < enc->height; h++)
  {
    for (int w = 0; w < enc->width; w++)
    {
      int val = img->val[h][w];
      enc->histogram[val]++;
    }
  }
}

// ヒストグラムから確率モデルを生成
void generate_pmodel(ENCODER *enc)
{
  int max_histogram = 0;

  // ヒストグラムの最大値を計算
  for (int val = 0; val <= enc->maxval; val++)
  {
    int tmp_histogram = enc->histogram[val];
    max_histogram = (max_histogram > tmp_histogram) ? max_histogram : tmp_histogram;
  }

  // hitogramを256段階(8bit)に量子化してpmodelを生成
  for (int val = 0; val <= enc->maxval; val++)
  {
    enc->pmodel[val] = (int)((double)enc->histogram[val] * enc->pmodel_precision / max_histogram);
    if(enc->pmodel[val] > enc->maxval)
    {
      enc->pmodel[val] = enc->maxval;
    }
    if(enc->pmodel[val] < 1)
    {
      enc->pmodel[val] = 1;
    }
  }
}

// 確率モデルを8bitで直接符号化
int encode_pmodel(ENCODER *enc, FILE *fp)
{
  int bits = 0;
  for (int val = 0; val <= enc->maxval; val++)
  {
    bits += putbits(fp, 8, enc->pmodel[val]);
  }

  return (bits);
}

// 画像を符号化
int encode_image(ENCODER *enc, FILE *fp, IMAGE *img)
{
  PMODEL pm;
  pm.freq = enc->pmodel;
  // 累積確率
  pm.cumfreq = (uint *)alloc_mem((enc->maxval + 1) * sizeof(int));
  // 確率の合計値
  int totfreq;
  totfreq = 0;
  int bits = 0;
  int val;

  // 累積確率を計算
  pm.cumfreq[0] = 0;
  for (int val = 0; val <= enc->maxval; val++)
  {
    pm.cumfreq[val + 1] = pm.cumfreq[val] + pm.freq[val] - pm.cumfreq[0];
  }
  totfreq = pm.cumfreq[enc->maxval + 1] - pm.cumfreq[0];

  // ラスタスキャン順に１画素ずつ符号化
  for (int y = 0; y < enc->height; y++)
  {
    for (int x = 0; x < enc->width; x++)
    {
      val = img->val[y][x];
      rc_encode(fp, enc->rc, pm.cumfreq[val], pm.freq[val], totfreq);
    }
  }
  rc_finishenc(fp, enc->rc);
  bits = enc->rc->code;

  free(pm.cumfreq);
  return (bits);
}

int main(int argc, char **argv)
{
  IMAGE *img;
  ENCODER enc;
  FILE *fp;
  int bits, pmodel_rate, image_rate;
  char *infile, *outfile;
  infile = outfile = NULL;

  infile = argv[1];
  outfile = argv[2];

  // 画像を読み込む
  img = read_pgm(infile);

  // 　画像を基にエンコーダーを初期化
  enc = init_encoder(img);

  // 画像のヒストグラムを計算
  calc_histogram(&enc, img);

  // 画像のヒストグラムから確率モデルを作成
  generate_pmodel(&enc);

  // 出力先ファイルの設定
  fp = fileopen(outfile, "wb");

  // ヘッダー書き込み
  bits = write_header(&enc, fp);

  printf("*** start encoding ***\n");

  // 確率モデルを符号化
  bits += pmodel_rate = encode_pmodel(&enc, fp);

  // 画像を符号化
  bits += image_rate = encode_image(&enc, fp, img);

  printf("encoding success.\n");

  printf("pmodel bitrate[bits/pel]: %f\n", (double)pmodel_rate/(enc.height * enc.width));
  printf("image  bitrate[bits/pel]: %f\n", (double)image_rate/(enc.height * enc.width));
  printf("-----------------------------------\n");
  printf("total  bitrate[bits/pel]: %f\n", (double)bits / (enc.height * enc.width));
  printf("\n");

  fclose(fp);
  free(img);
  free(enc.histogram);
  free(enc.pmodel);
  free(enc.rc);

  return (0);
}