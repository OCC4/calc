/*
 * block - fixed, dynamic, fifo and circular memory blocks
 */
/*
 * Copyright (c) 1997 by Landon Curt Noll.  All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright, this permission notice and text
 * this comment, and the disclaimer below appear in all of the following:
 *
 *	supporting documentation
 *	source copies
 *	source works derived from this source
 *	binaries derived from this source or from derived source
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Comments, suggestions, bug fixes and questions about these routines
 * are welcome.  Send EMail to the address given below.
 *
 * Happy bit twiddling,
 *
 *			Landon Curt Noll
 *
 *			chongo@toad.com
 *			...!{pyramid,sun,uunet}!hoptoad!chongo
 *
 * chongo was here	/\../\
 */


#include <stdio.h>
#include "value.h"
#include "zmath.h"
#include "config.h"
#include "block.h"
#include "nametype.h"
#include "string.h"
#include "calcerr.h"

#define NBLOCKCHUNK 16

static long nblockcount = 0;
static long maxnblockcount = 0;
static STRINGHEAD nblocknames;
static NBLOCK **nblocks;


/* forward declarations */
static void blkchk(BLOCK*);


/*
 * blkalloc - allocate a block
 *
 * given:
 *	len	- initial memory length of the block
 *	type	- BLK_TYPE_XXX
 *	chunk	- allocation chunk size
 *
 * returns:
 *	pointer to a newly allocated BLOCK
 */
BLOCK *
blkalloc(int len, int chunk)
{
	BLOCK *new;	/* new block allocated */

	/*
	 * firewall
	 */
	if (len < 0)
		len = 0;
	if (chunk <= 0)
		chunk = BLK_CHUNKSIZE;

	/*
	 * allocate BLOCK
	 */
	new = (BLOCK *)malloc(sizeof(BLOCK));
	if (new == NULL) {
		math_error("cannot allocate block");
		/*NOTREACHED*/
	}

	/*
	 * initialize BLOCK
	 */
	new->blkchunk = chunk;
	new->maxsize = ((len+chunk)/chunk)*chunk;
	new->data = (USB8*)malloc(new->maxsize);
	if (new->data == NULL) {
		math_error("cannot allocate block data storage");
		/*NOTREACHED*/
	}
	memset(new->data, 0, new->maxsize);
	new->datalen = len;

	/*
	 * return BLOCK
	 */
	if (conf->calc_debug > 0) {
		blkchk(new);
	}
	return new;
}


/*
 * blk_free - free a block
 *
 * NOTE: THIS IS NOT THE CALC blktrunc() BUILTIN FUNCTION!!  This
 * is what is called to free block storage.
 *
 * given:
 *	blk	- the block to free
 */
void
blk_free(BLOCK *blk)
{
	/* free if non-NULL */
	if (blk != NULL) {

		/* free data storage */
		if (blk->data != NULL) {
			free(blk->data);
		}

		/* free the block */
		free(blk);
	}
	return;
}


/*
 * blkchk - check the sanity of a block
 *
 * These checks should never fail if calc is working correctly.  During
 * debug time, we plan to call this function often.  Once we are satisfied,
 * we will normally call this code only in a few places.
 *
 * This function is normally called whenever the following builtins are called:
 *
 *	alloc(), realloc(), free()
 *
 * unless the "calc_debug" is set to -1.  If "calc_debug" is > 0, then
 * most blk builtins will call this function.
 *
 * given:
 *	blk	- the BLOCK to check
 *
 * returns:
 *	if all is ok, otherwise math_error() is called and this
 *	function does not return
 */
static void
blkchk(BLOCK *blk)
{

	/*
	 * firewall - general sanity check
	 */
	if (conf->calc_debug == -1) {
		/* do nothing when debugging is disabled */
		return;
	}
	if (blk == NULL) {
		math_error("internal: blk ptr is NULL");
		/*NOTREACHED*/
	}

	/*
	 * pointers must not be NULL
	 */
	if (blk->data == NULL) {
		math_error("internal: blk->data ptr is NULL");
		/*NOTREACHED*/
	}

	/*
	 * check data lengths
	 */
	if (blk->datalen < 0) {
		math_error("internal: blk->datalen < 0");
		/*NOTREACHED*/
	}

	/*
	 * check the datalen and datalen2 values
	 */
	if (blk->datalen < 0) {
		math_error("internal: blk->datalen < 0");
		/*NOTREACHED*/
	}
	return;
}


/*
 * blkrealloc - reallocate a block
 *
 * Reallocation of a block can change several aspects of a block.
 *
 * 	It can change the much data it holds or can hold.
 *
 * 	It can change the memory footprint (in terms of
 * 	how much storage is malloced for current or future use).
 *
 *	It can change the chunk size used to grow malloced size
 * 	as the data size grows.
 *
 * Each of the len and chunksize may be kept the same.
 *
 * given:
 *	blk	- old BLOCK to reallocate
 *	newlen	- how much data the block holds
 *	newchunk - allocation chunk size (<0 ==> no change, 0 == default)
 */
BLOCK *
blkrealloc(BLOCK *blk, int newlen, int newchunk)
{
	USB8 *new;	/* realloced storage */
	int newmax;	/* new maximum stoage size */

	/*
	 * firewall
	 */
	if (conf->calc_debug != -1) {
		blkchk(blk);
	}

	/*
	 * process args
	 */
	/* newlen < 0 means do not change the length */
	if (newlen < 0) {
		newlen = blk->datalen;
	}
	/* newchunk <= 0 means do not change the chunk size */
	if (newchunk < 0) {
		newchunk = blk->blkchunk;
	} else if (newchunk == 0) {
		newchunk = BLK_CHUNKSIZE;
	}

	/*
	 * reallocate storage if we have a different allocation size
	 */
	newmax = ((newlen+newchunk)/newchunk)*newchunk;
	if (newmax != blk->maxsize) {

		/* reallocate new storage */
		new = (USB8*)realloc(blk->data, newmax);
		if (new == NULL) {
			math_error("cannot reallocate block storage");
			/*NOTREACHED*/
		}

		/* clear any new storage */
		if (newmax > blk->maxsize) {
			memset(new + blk->maxsize, 0, (newmax - blk->maxsize));
		}
		blk->maxsize = newmax;

		/* restore the data pointers */
		blk->data = new;
	}

	/*
	 * deal the case of a newlen == 0 early and return
	 */
	if (newlen == 0) {

		/*
		 * setup the empty buffer
		 *
		 * We know that newtype is not circular since we force
		 * newlen to be at least 1 (because circular blocks
		 * always have at least one unused octet).
		 */
		if (blk->datalen < blk->maxsize) {
			memset(blk->data, 0, blk->datalen);
		} else {
			memset(blk->data, 0, blk->maxsize);
		}
		blk->datalen = 0;
		if (conf->calc_debug > 0) {
			blkchk(blk);
		}
		return blk;
	}

	/*
	 * Set the data length
	 *
	 * We also know that the new block is not empty since we have
	 * already dealth with that case above.
	 *
	 * After this section of code, limit and datalen will be
	 * correct in terms of the new type.
	 */
	if (newlen > blk->datalen) {

		/* there is new storage, clear it */
		memset(blk->data + blk->datalen, 0, newlen-blk->datalen);
		/* growing storage for blocks grows the data */
		blk->datalen = newlen;

	} else if (newlen <= blk->datalen) {

		/* the block will be full */
		blk->datalen = newlen;
	}

	/*
	 * return realloced type
	 */
	if (conf->calc_debug > 0) {
		blkchk(blk);
	}
	return blk;
}


/*
 * blktrunc - truncate a BLOCK down to a minimal fixed block
 *
 * NOTE: THIS IS NOT THE INTERNAL CALC FREE FUNCTION!!  This
 * is what blktrunc() builtin calls to reduce storage of a block
 * down to an absolute minimum.
 *
 * This actually forms a zero length fixed block with a chunk of 1.
 *
 * given:
 *	blk	- the BLOCK to shrink
 *
 * returns:
 *	pointer to a newly allocated BLOCK
 */
void
blktrunc(BLOCK *blk)
{
	/*
	 * firewall
	 */
	if (conf->calc_debug != -1) {
		blkchk(blk);
	}

	/*
	 * free the old storage
	 */
	free(blk->data);

	/*
	 * setup as a zero length fixed block
	 */
	blk->blkchunk = 1;
	blk->maxsize = 1;
	blk->datalen = 0;
	blk->data = (USB8*)malloc(1);
	if (blk->data == NULL) {
		math_error("cannot allocate truncated block storage");
		/*NOTREACHED*/
	}
	blk->data[0] = (USB8)0;
	if (conf->calc_debug > 0) {
		blkchk(blk);
	}
	return;
}


/*
 * blk_copy - copy a block
 *
 * given:
 *	blk	- the block to copy
 *
 * returns:
 *	pointer to copy of blk
 */
BLOCK *
blk_copy(BLOCK *blk)
{
	BLOCK *new;		/* copy of blk */

	/*
	 * malloc new block
	 */
	new = (BLOCK *)malloc(sizeof(BLOCK));
	if (new == NULL) {
		math_error("blk_copy: cannot malloc BLOCK");
		/*NOTREACHED*/
	}

	/*
	 * duplicate most of the block
	 */
	*new = *blk;

	/*
	 * duplicate block data
	 */
	new->data = (USB8 *)malloc(blk->maxsize);
	if (new->data == NULL) {
		math_error("blk_copy: cannot duplicate block data");
		/*NOTREACHED*/
	}
	memcpy(new->data, blk->data, blk->maxsize);
	return new;
}


/*
 * blk_cmp - compare blocks
 *
 * given:
 *	a	first BLOCK
 *	b	second BLOCK
 *
 * returns:
 *	TRUE => BLOCKs are different
 *	FALSE => BLOCKs are the same
 */
int
blk_cmp(BLOCK *a, BLOCK *b)
{
	/*
	 * firewall and quick check
	 */
	if (a == b) {
		/* pointers to the same object */
		return FALSE;
	}
	if (a == NULL || b == NULL) {
		/* one pointer is NULL, so they differ */
		return TRUE;
	}

	/*
	 * compare lengths
	 */
	if (a->datalen != b->datalen) {
		/* different lengths are different */
		return TRUE;
	}

	/*
	 * compare the section
	 *
	 * We have the same lengths and types, so compare the data sections.
	 */
	if (memcmp(a->data, b->data, a->datalen) != 0) {
		/* different sections are different */
		return TRUE;
	}

	/*
	 * the blocks are the same
	 */
	return FALSE;
}


/*
 * Print chunksize, maxsize, datalen on line line and if datalen > 0,
 * up to * 30 octets on the following line, with ... if datalen exceeds 30.
 */
/*ARGSUSED*/
void
blk_print(BLOCK *blk)
{
	long i;
	BOOL havetail;
	USB8 *ptr;

	/* XXX - use the config parameters for better print control */

	printf("chunksize = %d, maxsize = %d, datalen = %d\n\t",
		(int)blk->blkchunk, (int)blk->maxsize, (int)blk->datalen);
	i = blk->datalen;
	havetail = (i > 30);
	if (havetail)
		i = 30;
	ptr = blk->data;
	while (i-- > 0)
		printf("%02x", *ptr++);
	if (havetail)
		printf("...");
}


/*
 * Routine to print id and name of a named block and details of its
 * block component.
 */
void
nblock_print(NBLOCK *nblk)
{
	BLOCK *blk;

	/* XXX - use the config parameters for better print control */

	blk = nblk->blk;
	printf("block %d: %s\n\t", nblk->id, nblk->name);
	if (blk->data == NULL) {
		printf("chunksize = %d, maxsize = %d, datalen = %d\n\t",
		(int)blk->blkchunk, (int)blk->maxsize, (int)blk->datalen);
		printf("NULL");
	}
	else
		blk_print(blk);
}


/*
 * realloc a named block specified by its id.  The new datalen and
 * chunksize are specified by len >= 0 and chunk > 0.  If len < 0
 * or chunk <= 0, these values used are the current datalen and
 * chunksize, so there is no point in calling this unless len >= 0
 * and/or chunk > 0.
 * No reallocation occurs if the new maxsize is equal to the old maxsize.
 */
NBLOCK *
reallocnblock(int id, int len, int chunk)
{
	BLOCK *blk;
	int newsize;
	int oldsize;
	USB8* newdata;

	/* Fire wall */
	if (id < 0 || id >= nblockcount) {
		math_error("Bad id in call to reallocnblock");
		/*NOTREACHED*/
	}

	blk = nblocks[id]->blk;
	if (len < 0)
		len = blk->datalen;
	if (chunk < 0)
		chunk = blk->blkchunk;
	else if (chunk == 0)
		chunk = BLK_CHUNKSIZE;
	newsize = (1 + len/chunk) * chunk;
	oldsize = blk->maxsize;
	newdata = blk->data;
	if (newdata == NULL) {
		newdata = malloc(newsize);
		if (newdata == NULL) {
			math_error("Allocation failed");
			/*NOTREACHED*/
		}
	}
	else if (newsize != oldsize) {
		newdata = realloc(blk->data, newsize);
		if (newdata == NULL) {
			math_error("Reallocation failed");
			/*NOTREACHED*/
		}
	}
	memset(newdata + len, 0, newsize - len);

	blk->maxsize = newsize;
	blk->datalen = len;
	blk->blkchunk = chunk;
	blk->data = newdata;
	return nblocks[id];
}


/*
 * Create and return a new namedblock with specified name, len and
 * chunksize.
 */
NBLOCK *
createnblock(char *name, int len, int chunk)
{
	NBLOCK *res;
	char *newname;

	if (nblockcount >= maxnblockcount) {
		if (maxnblockcount <= 0) {
			maxnblockcount = NBLOCKCHUNK;
			nblocks = (NBLOCK **)malloc(NBLOCKCHUNK *
						    sizeof(NBLOCK *));
			if (nblocks == NULL) {
				maxnblockcount = 0;
				math_error("unable to malloc new named blocks");
				/*NOTREACHED*/
			}
		} else {
			maxnblockcount += NBLOCKCHUNK;
			nblocks = (NBLOCK **)realloc(nblocks, maxnblockcount *
						     sizeof(NBLOCK *));
			if (nblocks == NULL) {
				maxnblockcount = 0;
				math_error("cannot malloc more named blocks");
				/*NOTREACHED*/
			}
		}
	}
	if (nblockcount == 0)
		initstr(&nblocknames);
	if (findstr(&nblocknames, name) >= 0) {
		math_error("Named block already exists!!!");
		/*NOTREACHED*/
	}
	newname = addstr(&nblocknames, name);
	if (newname == NULL) {
		math_error("Block name allocation failed");
		/*NOTREACHED*/
	}

	res = (NBLOCK *) malloc(sizeof(NBLOCK));
	if (res == NULL) {
		math_error("Named block allocation failed");
		/*NOTREACHED*/
	}

	nblocks[nblockcount] = res;
	res->name = newname;
	res->subtype = V_NOSUBTYPE;
	res->id = nblockcount++;
	res->blk = blkalloc(len, chunk);
	return res;
}


/*
 * find a named block
 */
int
findnblockid(char * name)
{
	return findstr(&nblocknames, name);
}


/*
 * free data block for named block with specified id
 */
int
removenblock(int id)
{
	NBLOCK *nblk;

	if (id < 0 || id >= nblockcount)
		return E_BLKFREE3;
	nblk = nblocks[id];
	if (nblk->blk->data == NULL)
		return 0;
	if (nblk->subtype & V_NOREALLOC)
		return E_BLKFREE5;
	free(nblk->blk->data);
	nblk->blk->data = NULL;
	nblk->blk->maxsize = 0;
	nblk->blk->datalen = 0;
	return 0;
}


/*
 * count number of current unfreed named blocks
 */
int
countnblocks(void)
{
	int n;
	int id;

	for (n = 0, id = 0; id < nblockcount; id++) {
		if (nblocks[id]->blk->data != NULL)
			n++;
	}
	return n;
}


/*
 * display id and name for each unfreed named block
 */
void
shownblocks(void)
{
	int id;

	if (countnblocks() == 0) {
		printf("No unfreed named blocks\n\n");
		return;
	}
	printf(" id   name\n");
	printf("----  -----\n");
	for (id = 0; id < nblockcount; id++) {
		if (nblocks[id]->blk->data != NULL)
			printf("%3d   %s\n", id, nblocks[id]->name);
	}
	printf("\n");
}


/*
 * Return pointer to nblock with specified id, NULL if never created.
 * The memory for the nblock found may have been freed.
 */
NBLOCK *
findnblock(int id)
{
	if (id < 0 || id >= nblockcount)
		return NULL;
	return nblocks[id];
}


/*
 * Create a new block with specified newlen and new chunksize and copy
 * min(newlen, oldlen) octets to the new block.  The old block is
 * not changed.
 */
BLOCK *
copyrealloc(BLOCK *blk, int newlen, int newchunk)
{
	BLOCK * newblk;
	int oldlen;

	oldlen = blk->datalen;

	if (newlen < 0) 		/* retain length */
		newlen = oldlen;

	if (newchunk < 0)		/* retain chunksize */
		newchunk = blk->blkchunk;
	else if (newchunk == 0)		/* use default chunksize */
		newchunk = BLK_CHUNKSIZE;


	newblk = blkalloc(newlen, newchunk);

	if (newlen < oldlen)
		oldlen = newlen;

	if (newlen > 0)
		memcpy(newblk->data, blk->data, oldlen);

	return newblk;
}
