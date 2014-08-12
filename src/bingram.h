#ifndef BINGRAM_H
#define BINGRAM_H
/*
 *	bingram.c - command for finding common sequences of bytes (grams) across multiple binary files.
 *
 *	Author: Guilherme G. Martins <gmartins at cc gatech dot edu>
 *
 *  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <limits.h>
#include <dirent.h>

#ifdef DEBUG
# define DPRINT(x, y) if (y & MODE_VERBOSE ) printf x 
#else
# define DPRINT(x, y) do {} while (0)
#endif

#define BG_DEFAULT_GRAMSIZE		2
#define BG_DEFAULT_MAXFILES		200
#define BG_DEFAULT_BUFFERSIZE	1500
#define BG_DEFAULT_EDITDIST		0
#define BG_LIMIT_BUFFERSIZE		8000
#define BG_LIMIT_MAXFILES		18000
#define BG_LIMIT_GRAMDATA		(sizeof(unsigned char) << CHAR_BIT)
#define BG_LIMIT_GRAMDATA_DEPTH	500
#define BG_LIMIT_EDITDIST		5
#define BG_LIMIT_FULLPATH		200
#define BG_LIMIT_FILEHIT		200
#define BG_LIMIT_HISTOGRAM		(sizeof(unsigned char) << CHAR_BIT)

/*#define BG_JSON_BINGRAM {
#include "bingram.c.json"
}
#define BG_JSON_FILE files.c.json
#define BG_JSON_GRAM gram.c.json */

typedef enum { 
  MODE_DEFAULT = 0,
  MODE_VERBOSE = 0x01,
  MODE_BYTECNT = 0x02,
  MODE_STRINGS = 0x04,
} opt_mask_t;

typedef struct {
  unsigned char *buf; //null indicates "End of Array"
  int addr,offs; //start address/index,
  int count;     //number of occurances,
} gram_t;

typedef struct {
  int size;
  int hit;
  unsigned char *buf;
  char *filename;
  gram_t gram[BG_LIMIT_FILEHIT];
  int histogram[BG_LIMIT_HISTOGRAM];
} bg_file_t;

typedef struct {
  unsigned int maxfiles;
  unsigned int buffersize;
  unsigned int gramsize;
  unsigned int editdist;
  opt_mask_t opt_mask;
  int ind; //for file
  bg_file_t **bg_file;
  gram_t gramdata[BG_LIMIT_GRAMDATA][BG_LIMIT_GRAMDATA_DEPTH];
} bg_mem_t;


int bg_mem_init(bg_mem_t *bg_mem, opt_mask_t opt_mask, int maxfiles, int buffersize, int gramsize);
int bg_mem_show(bg_mem_t *bg_mem);
int bg_mem_addgram(bg_mem_t *bg_mem, unsigned char *buf, int addr, int offs);
int bg_mem_addfile(bg_mem_t *bg_mem, char *filename);
int bg_mem_process(bg_mem_t *bg_mem);
int bg_mem_close(bg_mem_t *bg_mem);
int bg_file_init(bg_file_t *bg_file, FILE *f, char *filename, opt_mask_t opt_mask);
int bg_file_show(bg_file_t *bg_file, opt_mask_t opt_mask);
int bg_file_addgram(bg_file_t *bg_file, unsigned char *buf, int addr, int offs);
int gram_show(gram_t *g);

#endif
