
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
#include "bingram.h"


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
    fprintf(stderr, "\t -e,--editdist\tchange edit distance subtraction tolerance (default %d, max %d)\n", 
                                                                                      BG_DEFAULT_EDITDIST,
                                                                                      BG_LIMIT_EDITDIST);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int opt;
	int bg_mem_buffersize = BG_DEFAULT_BUFFERSIZE;
	int bg_mem_maxfiles = BG_DEFAULT_MAXFILES;
	int bg_mem_gramsize = BG_DEFAULT_GRAMSIZE;
	int bg_mem_editdist = BG_DEFAULT_EDITDIST;
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

	while ((opt = getopt_long(argc, argv, "ivsce:g:b:f:",
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
	    case 'e':
	    	bg_mem_editdist=atoi(optarg);
			if(bg_mem_editdist>0 && bg_mem_editdist<BG_LIMIT_EDITDIST )
				printf("Changing edit distance default to [%d] of subtraction tolerance\n", bg_mem_editdist);
			else
			{
				fprintf(stderr, "main: invalid editdist, try any positive integer between 0 to %d\n", BG_LIMIT_EDITDIST);
				return 1;
			}
			DPRINT(("arg %s\n", optarg), 1);
	    	//printf
	        break;


	    default:
	    	usage(argv[0]);
	    }
	}

	DPRINT(("verbose mode on \n"), opt_mask);

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
			bg_mem_addfile(&bg_mem, argv[i]);
		}

	    bg_mem_process(&bg_mem);
	    bg_mem_show(&bg_mem);
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
	bg_mem->out_json=bg_mem_out;
	return 0;
}


int bg_mem_addfile(bg_mem_t *bg_mem, char *filename)
{
    struct stat st;
    stat(filename, &st);
    if( st.st_mode & S_IFDIR )
    {
        DIR *dir;
        struct dirent *ent;
        char fullpath[BG_LIMIT_FULLPATH];
		DPRINT(("loading dir %s ...\n", filename), bg_mem->opt_mask);
		if ((dir = opendir (filename)) != NULL) 
		{
			int ret=1;
			while ((ent = readdir (dir)) != NULL) 
			if(ent->d_name[0]!='.')
			{
				snprintf(fullpath, BG_LIMIT_FULLPATH, "%s/%s", filename, ent->d_name);
				DPRINT(("%s\n", fullpath), bg_mem->opt_mask);
				ret=bg_mem_addfile(bg_mem, fullpath);
			}
			if(!ret) closedir(dir);
		}
	}
	else if( st.st_mode & S_IFREG )
    {
	    FILE *fp = fopen(filename, "r");
	    if (fp == 0)
	        fprintf(stderr, "bg_mem_addfile: failed to open %s (%d %s)\n",
	                                     filename, errno, strerror(errno));
	    else
	    {
	    	bg_file_t *bg_file;
	    	bg_file = (bg_file_t *)malloc(sizeof(bg_file_t));
	    	if(!bg_file)
	    	{
				fprintf(stderr, "main: malloc failed\n");
	    		return 1;
	    	}
	   		DPRINT(("loading file %s ...\n", filename), bg_mem->opt_mask);
	        bg_file_init(bg_file,fp, filename, bg_mem->opt_mask);
	        if((bg_file->size > 0) && (bg_file->size <= bg_mem->buffersize))
	        {
	        	if( bg_mem->ind <= bg_mem->maxfiles )
					bg_mem->bg_file[bg_mem->ind++]=bg_file;
	        }
	        else
	        {
	        	fprintf(stderr, "main: invalid file size of %d, allowed: %d. Check -b option.\n", 
	        		bg_file->size, 
	        		bg_mem->buffersize);
	        	return 1;
	        }
	        fclose(fp);
	    }
	}

	return 0;
}


int bg_mem_addgram(bg_mem_t *bg_mem, unsigned char *buf, int addr, int offs)
{
	int ind=0;
	int hashind=(int)buf[addr];
	int match, nmatch;

	while( (bg_mem->gramdata[hashind][ind].buf) )
	{
		int i;
		match=1; 
		nmatch=0;
		for(i=1; (i < bg_mem->gramdata[hashind][ind].offs) && (i < offs) && (nmatch <= bg_mem->editdist); i++)
		{
			int gdind=bg_mem->gramdata[hashind][ind].addr;
			//printf("--->%02X\n", bg_mem->gramdata[hashind][ind].buf[gdind + i]);
			//TODO: change for "byte aligned" hamming dist here (abs(buf1[a] - buf2[a]) < threshold)
			if(bg_mem->gramdata[hashind][ind].buf[gdind + i] == buf[addr+i])
				match++;
			else
				nmatch++;
		}
		//perfect match, nothing more to do
		if((match == offs) && 
		   (match == bg_mem->gramdata[hashind][ind].offs))
		{
			// return on overlapping with existing gram 
			//printf("## %d %d\n", addr, bg_mem->gramdata[hashind][ind].addr);
			//printf("## %d %d\n", addr, bg_mem->gramdata[hashind][ind].addr-bg_mem->gramdata[hashind][ind].offs);
			if(addr < bg_mem->gramdata[hashind][ind].addr )
				if( addr > bg_mem->gramdata[hashind][ind].addr-bg_mem->gramdata[hashind][ind].offs ) return 1;
			bg_mem->gramdata[hashind][ind].buf=buf;
			bg_mem->gramdata[hashind][ind].addr=addr;
			bg_mem->gramdata[hashind][ind].offs=offs;
			bg_mem->gramdata[hashind][ind].count++;
			bg_mem->gramdata[hashind][ind].out_json=gram_out;
			return 0;
		}
		
		//if((nmatch > bg_mem->editdist) && (match > bg_mem->gramsize)) //still good, move along..

		ind++;
		if(ind == BG_LIMIT_GRAMDATA_DEPTH) return 1;
	}
	DPRINT(("adding gram at %d, %02X\n", addr, buf[addr]), bg_mem->opt_mask);
	bg_mem->gramdata[hashind][ind].buf=buf;
	bg_mem->gramdata[hashind][ind].addr=addr;
	bg_mem->gramdata[hashind][ind].offs=offs;
	bg_mem->gramdata[hashind][ind].count++;
	bg_mem->gramdata[hashind][ind].out_json=gram_out;
	return 0;
}

int bg_file_addgram(bg_file_t *bg_file, unsigned char *buf, int addr, int offs)
{
	//TODO replace ones with fewer matches
	bg_file->gram[bg_file->hit].buf=buf;
	bg_file->gram[bg_file->hit].addr=addr;
	bg_file->gram[bg_file->hit].offs=offs;
	bg_file->gram[bg_file->hit].out_json=gram_out;
	bg_file->gram[bg_file->hit].count=1;
	if(bg_file->hit < BG_LIMIT_FILEHIT-1) bg_file->hit++;
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
				//printf(".%d %d\n", ind1, ind2);
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
            			  int ret;
            			  DPRINT(("Adding gram\n"), bg_mem->opt_mask);
            			  //bg_mem_addgram(bg_mem, f1->buf, ind1 - sequence, sequence);
            			  if(!ind2) ind2=f2->size;

            			  ret=bg_mem_addgram(bg_mem, f2->buf, ind2 - sequence, sequence);
            			  if(!ret)
            			  {
            			  	int addr1=((ind1 - sequence) < 0)?0:(ind1 - sequence);
            			  	int addr2=((ind2 - sequence) < 0)?0:(ind2 - sequence);
            				bg_file_addgram(f1, f1->buf, addr1, sequence);
            				bg_file_addgram(f2, f2->buf, addr2, sequence);	
            			  }
            			}
                  }
				  sequence=0;
				} //if
			} //fir ind
		} //for k
	} //for j
		
	return 0;
}

int bg_mem_close(bg_mem_t *bg_mem)
{
	int i;
	for(i=0; i<bg_mem->ind; i++)
	{
		free(bg_mem->bg_file[i]->filename);
		free(bg_mem->bg_file[i]->buf);
		free(bg_mem->bg_file[i]);
	}
	return 0;
}

int bg_file_init(bg_file_t *bg_file, FILE *f, char *filename, opt_mask_t opt_mask)
{
    struct stat st;
    stat(filename, &st);

    /*if (opt_mask & MODE_STRINGS)
    {
	printf("strings\n");
    }*/

    if( st.st_mode & S_IFREG )
    {
    	DPRINT(("filename: %s, %d\n", filename, (int)st.st_size), opt_mask);
		bg_file->size=(int)st.st_size;
		bg_file->filename=strdup(filename);
		bg_file->buf=(unsigned char *)malloc(sizeof(unsigned char)*bg_file->size);
		bg_file->out_json=bg_file_out;
		if(!bg_file->buf) 
		{
			fprintf(stderr, "bg_file_init: unable to allocate %d bytes for %s\n", bg_file->size, filename);
			return 1;
		}
		fread(bg_file->buf, sizeof(char), bg_file->size, f);
		if (opt_mask & MODE_BYTECNT)
		{
			int i;
			DPRINT(("computing histogram... \n"), opt_mask);
			for(i=0; i<bg_file->size; i++)
			{
				int ind=(int)bg_file->buf[i];
				bg_file->histogram[ind]++;
			}
		}
    } 
    else
    {
    	fprintf(stderr, "bg_file_init: not a file or directory: %s\n", filename);
    	return 1;
    }

	
    return 0;
}

int bg_mem_show(bg_mem_t *bg_mem)
{
	int i,j,k, size=BG_LIMIT_OUTBUF, offs=0;
	char outbuf[BG_LIMIT_OUTBUF];
	char jsonbuf[BG_LIMIT_OUTBUF];
	char gramdata[BG_LIMIT_OUTBUF];
	char gram[BG_LIMIT_BUFFERSIZE];
	

	if(bg_mem->opt_mask & MODE_VERBOSE)
	{	
		printf("bg_mem->maxfiles: %d\n", bg_mem->maxfiles);
		printf("bg_mem->buffersize: %d\n", bg_mem->buffersize);
		printf("bg_mem->ind: %d\n", bg_mem->ind); 
		printf("bg_mem->grams:\n");
		for(i=0; i < BG_LIMIT_GRAMDATA; i++)
			for(j=0; j < BG_LIMIT_GRAMDATA_DEPTH; j++)
		{
			gram_t *g=&bg_mem->gramdata[i][j];
			if(g->buf) //continue;
			{
				printf(" bg_gram->addr(%d),offs(%d),cnt(%d),buf:", g->addr, g->offs, g->count);
				for(k=0; k < g->offs; k++)
					printf("%02X", g->buf[g->addr + k]);
				printf("\n"); 
			}
		}
	}
	
	for(i=0; (i < bg_mem->ind) && (offs < BG_LIMIT_OUTBUF); i++)
	{
		if(bg_file_show(bg_mem->bg_file[i], bg_mem->opt_mask, outbuf+offs, &size)) continue;

		offs=size;
		size=BG_LIMIT_OUTBUF;
	}
	if(outbuf[strlen(outbuf)-1] == ',') outbuf[strlen(outbuf)-1] = 0;
	
	for(i=0; i < BG_LIMIT_GRAMDATA; i++)
		for(j=0; j < BG_LIMIT_GRAMDATA_DEPTH; j++)
		{
			gram_t *g=&bg_mem->gramdata[i][j];
			if(g->buf) //continue;
			{
				gram_show(g, bg_mem->opt_mask, gram, BG_LIMIT_BUFFERSIZE);
				snprintf(gramdata, BG_LIMIT_OUTBUF, "{%s},", gram);
			}
		}
	if(gramdata[strlen(gramdata)-1] == ',') gramdata[strlen(gramdata)-1] = 0;

	snprintf(jsonbuf, BG_LIMIT_OUTBUF, bg_mem->out_json, outbuf, gramdata);
	printf("%s\n", jsonbuf);
	return 0;
}

int bg_file_show(bg_file_t *bg_file, opt_mask_t opt_mask, char *outbuf, int *size)
{
	int i; 
	char gram[BG_LIMIT_BUFFERSIZE];
	char gramdata[BG_LIMIT_BUFFERSIZE*2];
	char histdata[BG_LIMIT_HISTBUF];
	if(!bg_file) return 1;
	if(!bg_file->hit)
	{
		//*size=0;
		return 1;
	}
	
	if(opt_mask & MODE_VERBOSE)
	{
		printf("bg_file->name: %s\n", bg_file->filename);
		printf("bg_file->hit: %d\n", bg_file->hit);
		printf("bg_file->size: %d\n", bg_file->size);
		printf("bg_file->buf:");
		for(i=0; i< ((bg_file->size<10)?bg_file->size:10); i++)
			printf("%02X", bg_file->buf[i]);
		if( i==10 ) printf("...");
		printf("\n"); 

		if( opt_mask & MODE_BYTECNT )
		{
			printf("bg_file->histogram:");
			for(i=0; i< BG_LIMIT_HISTOGRAM; i++) 
				printf("%d,",bg_file->histogram[i]);
			printf("\n");	
		}
	}

	for(i=0; i< bg_file->hit; i++)
	{
		gram_show(&bg_file->gram[i], opt_mask, gram, BG_LIMIT_BUFFERSIZE);
		snprintf(gramdata, BG_LIMIT_BUFFERSIZE, "{%s},", gram);

	}
	if(gramdata[strlen(gramdata)-1] == ',') gramdata[strlen(gramdata)-1] = 0;
	
	if( opt_mask & MODE_BYTECNT )
	{
		int ind=0;
		for(i=0; i< BG_LIMIT_HISTOGRAM; i++)
			ind+=snprintf(histdata+ind, BG_LIMIT_HISTBUF, "%d,", bg_file->histogram[i]);
		if(histdata[strlen(histdata)-1]==',') histdata[strlen(histdata)-1] = 0; 
	}

	snprintf(outbuf, *size, bg_file->out_json, bg_file->filename, bg_file->size, bg_file->hit, gramdata, histdata);
	
	*size=strlen(outbuf);

	return 0;
}

int gram_show(gram_t *g, opt_mask_t opt_mask, char *outbuf, int size)
{
	int i, ind=0;
	if(!g) return 1;
	if(!g->buf) return 1;
	if(!outbuf || size <= 0) return 1;
	if(opt_mask & MODE_VERBOSE)
	{
		printf("bg_gram->addr(%d),offs(%d),cnt(%d),buf:", g->addr, g->offs, g->count);
		for(i=0; i < g->offs; i++)
			printf("%02X", g->buf[g->addr + i]);
		printf("\n"); 
	}
    ind+=snprintf(outbuf+ind, size, "\"");

	for(i=0; (i < g->offs) && (size > 0) ; i++)
	{
		ind+=snprintf(outbuf+ind, size, "%02X", g->buf[g->addr + i]);
		size-=ind;
	}
	ind+=snprintf(outbuf+ind, size, "\"");
	if(size < 0) return 1;
	snprintf(outbuf+ind, size, g->out_json, g->addr, g->offs, g->count);

	return 0;
}

