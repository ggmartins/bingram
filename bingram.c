
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

#ifdef DEBUG
# define DPRINT(x, y) if (y & MODE_VERBOSE ) printf x 
#else
# define DPRINT(x, y) do {} while (0)
#endif

#define BG_DEFAULT_GRAMSIZE 3
#define BG_DEFAULT_MAXFILES 200
#define BG_DEFAULT_BUFFERSIZE 1500
#define BG_LIMIT_BUFFERSIZE 8000
#define BG_LIMIT_MAXFILES 18000

typedef enum { 
  MODE_DEFAULT = 0,
  MODE_VERBOSE = 0x01,
  MODE_BYTECNT = 0x02,
  MODE_STRINGS = 0x04,
} opt_mask_t;

typedef struct {
  int size;
  unsigned char *buf;
  char *filename;
} bg_file_t;

typdef struct {

} gram_t;

typedef struct {
  unsigned int maxfiles;
  unsigned int buffersize;
  unsigned int gramsize;
  opt_mask_t opt_mask;
  int ind;
  bg_file_t **bg_file;
  gram_t gram_hash[sizeof(unsigned char)];
} bg_mem_t;


int bg_mem_init(bg_mem_t *bg_mem, opt_mask_t opt_mask, int maxfiles, int buffersize, int gramsize);
int bg_mem_show(bg_mem_t *bg_mem);
int bg_file_init(bg_file_t *bg_file, FILE *f, char *filename, opt_mask_t opt_mask);
int bg_file_show(bg_file_t *bg_file);

void usage(char *cmdname)
{
    fprintf(stderr, "Usage: %s [-svcbf] [file ...]\n", cmdname);
    fprintf(stderr, "Show common sequences of bytes (grams) across multiple binary files.\n");
    fprintf(stderr, "\t -v,--verbose\tenable debug and verbose prints\n");
    fprintf(stderr, "\t -s,--strings\tprocess \"strings\" command output (alphanumeric) files\n");
    fprintf(stderr, "\t -i,--histogram\tsummary most common bytes found across files\n");
   	fprintf(stderr, "\t -g,--gramsize\tminimum size of a gram used in comparisons\n");
    fprintf(stderr, "\t -b,--buffersize\tchange max size for a file (default %d bytes)\n", BG_DEFAULT_BUFFERSIZE);
    fprintf(stderr, "\t -f,--maxfiles\tprocess up to maxfiles (default %d)\n\n", BG_DEFAULT_MAXFILES);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int opt;
	int bg_mem_buffersize = BG_DEFAULT_BUFFERSIZE;
	int bg_mem_maxfiles = BG_DEFAULT_MAXFILES;
	int bg_mem_gramsize = BG_DEFAULT_GRAMSIZE;
	opt_mask_t opt_mask = MODE_DEFAULT;  // Default set
	int option_index=0;

	bg_mem_t bg_mem;
	
	static struct option long_options[] =
	{
		/* These options set a flag. */
		{"verbose",   no_argument,      0, 'v'},
		{"strings",   no_argument,      0, 's'},
		{"histogram", no_argument,      0, 'i'},
		{"gramsize",  required_argument,0, 'g'},
		{"buffersize",required_argument,0, 'b'},
		{"maxfiles",  required_argument,0, 'f'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "vscg:b:f:",
                      long_options, &option_index)) != -1)
	{
	    switch (opt)
	    {

	    case 0:
		    if (long_options[option_index].flag != 0)
	           break;
	        DPRINT(("option %s", long_options[option_index].name), 1);
	        if (optarg)
	           DPRINT(("arg %s\n", optarg), 1);
	        printf ("\n");
            break; 
        case 's': opt_mask|=MODE_STRINGS; break;
	    case 'v': opt_mask|=MODE_VERBOSE; break;
	    case 'i': opt_mask|=MODE_BYTECNT; break;
	    case 'g':
	    	bg_mem_gramsize=atoi(optarg);
	    	if(bg_mem_gramsize>0 && bg_mem_gramsize<bg_mem_buffersize)
	    		printf("Using gram size of [%d] bytes\n", bg_mem_gramsize);
	    	else
	    	{
	    		fprintf(stderr, "main: invalid gram size, try any positive integer between 0 to %d\n", bg_mem_gramsize);
	    		return 1;
	    	}
	    	break;
	    case 'b':
	    	bg_mem_buffersize=atoi(optarg);
	    	if(bg_mem_buffersize>0 && bg_mem_buffersize<BG_LIMIT_BUFFERSIZE )
	    		printf("Using buffer size of [%d] bytes\n", bg_mem_buffersize);
	    	else
	    	{
	    		fprintf(stderr, "main: invalid buffersize, try any positive integer between 0 to %d\n", BG_LIMIT_BUFFERSIZE);
	    		return 1;
	    	}
	        break;
	    case 'f':
	    	bg_mem_maxfiles=atoi(optarg);
			if(bg_mem_maxfiles>0 && bg_mem_maxfiles<BG_LIMIT_MAXFILES )
				printf("Allocating data for processing up to [%d] (maxfiles)\n", bg_mem_maxfiles);
			else
			{
				fprintf(stderr, "main: invalid maxfiles, try any positive integer between 0 to %d\n", BG_LIMIT_MAXFILES);
				return 1;
			}
			DPRINT(("arg %s\n", optarg), 1);
	    	//printf
	        break;

	    default:
	    	usage(argv[0]);
	    }
	}

	DPRINT(("verbose mode on "), opt_mask);

	/* Process file names or stdin */
	if (optind >= argc)
	{
	//	bg_file_init(stdin, "(standard input)", opt_mask);
		usage(argv[0]);
	}
	else
	{
	    int i;
	    bg_mem_init(&bg_mem, opt_mask, bg_mem_maxfiles, bg_mem_buffersize, bg_mem_gramsize);
	    for (i = optind; i < argc; i++)
	    {

	        FILE *fp = fopen(argv[i], "r");
	        if (fp == 0)
	            fprintf(stderr, "%s: failed to open %s (%d %s)\n",
	                    argv[0], argv[i], errno, strerror(errno));
	        else
	        {
	        	bg_file_t *bg_file;
	        	bg_file = (bg_file_t *)malloc(sizeof(bg_file_t));
	        	if(!bg_file)
	        	{
	    			fprintf(stderr, "main: malloc failed\n");
	        		return 1;
	        	}
	       		printf("loading %s ...\n", argv[i]);
	            bg_file_init(bg_file,fp, argv[i], opt_mask);
	            if((bg_file->size > 0) && (bg_file->size <= bg_mem_buffersize))
	            	bg_mem_add(&bg_mem, bg_file);
	            else
	            {
	            	fprintf(stderr, "main: invalid file size of %d, allowed: %d. Check -b option.\n", bg_file->size, 
	            		bg_mem_buffersize);
	            	return 1;
	            }
	            fclose(fp);
	        }
	    }
	    bg_mem_show(&bg_mem);
	    bg_mem_process(&bg_mem);
	}
	bg_mem_close(&bg_mem);
	return 0;
}

int bg_mem_init(bg_mem_t *bg_mem, opt_mask_t opt_mask, int maxfiles, int buffersize, int gramsize)
{
	memset(bg_mem, 0, sizeof(bg_mem_t));
	bg_mem->bg_file=(bg_file_t **)malloc(sizeof(bg_file_t *)*maxfiles);
	if(!bg_mem->bg_file)
	{
		fprintf(stderr, "bg_mem_init: malloc failed\n");
		return 1;
	}
	memset(bg_mem->bg_file, 0, sizeof(bg_file_t *)*maxfiles);
	bg_mem->maxfiles=maxfiles;
	bg_mem->buffersize=buffersize;
	bg_mem->gramsize=gramsize;
	bg_mem->opt_mask=opt_mask;
	return 0;
}

int bg_mem_show(bg_mem_t *bg_mem)
{
	int i;
	printf("bf_mem->maxfiles: %d\n", bg_mem->maxfiles);
	printf("bf_mem->buffersize: %d\n", bg_mem->buffersize);
	printf("bf_mem->ind: %d\n", bg_mem->ind);
	for(i=0; i<bg_mem->ind; i++)
		bg_file_show(bg_mem->bg_file[i]);
	return 0;
}

int bg_mem_add(bg_mem_t *bg_mem, bg_file_t *bg_file)
{
	if( bg_mem->ind <= bg_mem->maxfiles )
		bg_mem->bg_file[bg_mem->ind++]=bg_file;
	return 0;
}

int bg_mem_process(bg_mem_t *bg_mem)
{
	bg_file_t *f1, *f2;
	int i,j,k,ind1, ind2;
	for(i=0; i<bg_mem->ind; i++)
		for(j=i+1; j<bg_mem->ind; j++)
	{
		if( bg_mem->bg_file[i]->size >= bg_mem->bg_file[j]->size )
		{
			f1=bg_mem->bg_file[i];
			f2=bg_mem->bg_file[j];
		}
		else
		{
			f1=bg_mem->bg_file[j];
			f2=bg_mem->bg_file[i];
		}

		int sequence=0;
		for(k=1; k < f1->size + f2->size; k++)
		{
			int x =  (k <= f2->size)? 0 : k - f2->size;          //shifting x for ind1
			int y =  (k >= f2->size)? 0 : abs( k - f2->size );   //shifting x for ind2
			
			for (ind1=x; (ind1 < k) && (ind1 < f1->size); ind1++) 
			{
				ind2=y++;
				if(f1->buf[ind1] == f2->buf[ind2])
				{
				  DPRINT(("f1[%d]=%02X == %02X=f2[%d] \n", ind1, f1->buf[ind1], f2->buf[ind2], ind2), 
				  																   bg_mem->opt_mask);
				  sequence++;
				}
				else //end of sequence
				{
				  if(sequence > 0)
				  {
                    DPRINT(("end of sequence, size: %d\n", sequence), bg_mem->opt_mask);
                    if(sequence >= bg_mem->gramsize)
                    {

                    }
                  }
				  sequence=0;
				}

			}
			

		}

	}
		
	return 0;
}

int bg_mem_close(bg_mem_t *bg_mem)
{
	int i;
	for(i=0; i<bg_mem->ind; i++)
		free(bg_mem->bg_file[i]);
	return 0;
}

int bg_file_init(bg_file_t *bg_file, FILE *f, char *filename, opt_mask_t opt_mask)
{
	if (opt_mask & MODE_BYTECNT)
	{
	printf("bytecount\n");	
	}
	if (opt_mask & MODE_STRINGS)
	{
		printf("strings\n");
	}
	struct stat st;
	stat(filename, &st);
	if( st.st_mode & S_IFDIR )
    {
        fprintf(stderr, "bg_file_init: directories not yet supported: %s\n", filename);
    }
    else if( st.st_mode & S_IFREG )
    {
    	printf("filename: %s, %d\n", filename, (int)st.st_size);
		bg_file->size=(int)st.st_size;
		bg_file->filename=filename;//strdup(filename);
		bg_file->buf=(char *)malloc(sizeof(char)*bg_file->size);
		if(!bg_file) 
		{
			fprintf(stderr, "bg_file_init: unable to allocate %d bytes for %s %d\n", bg_file->size, filename);
			return 1;
		}
		fread(bg_file->buf, sizeof(char), bg_file->size, f);
    } 
    else
    {
    	fprintf(stderr, "bg_file_init: not a file of directory: %s\n", bg_file->size, filename);
    	return 1;
    }

	
	return 0;
}

int bg_file_show(bg_file_t *bg_file)
{
	int i;
	if(!bg_file) return 1;
	printf("bf_file->name: %s\n", bg_file->filename);
	printf("bf_file->size: %d\n", bg_file->size);
	printf("bf_file->buf:");
	for(i=0; i< ((bg_file->size<10)?bg_file->size:10); i++)
	  printf("%02X", bg_file->buf[i]);
	printf("\n");
	return 0;
}

