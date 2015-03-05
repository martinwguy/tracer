/*
 *	Convert Fritzz format pattern files to PNG image file
 *
 *	Usage: pattoras [file]
 *
 *	To quote Fritzz:
 *	The tiling bitmap can be digitized data, it must be in the form of 
 *	scan lines no longer than 512 bytes followed by newlines.
 *
 *	What he means is that it is a file of ascii characters, where the
 *	pixel values are given by the values of the printable characters
 *	in a line.
 *	It seems to assume that all lines are the same length.
 *	Input lines are guaranteed not to be longer than 512 chars.
 *	You need to take maximum and minimum of input chars to find the range.
 *
 *	Martin Guy, UKC, January 1987
 *	Martin Guy, Catania, March 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

char *progname;	/* argv[0] for error reporting */

main(argc, argv)
char **argv;
{
	FILE *fp;

	progname = argv[0];

	switch (argc) {
	case 1:
		convert(stdin, "stdin");
		break;
	case 2:
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "%s: ", progname);
			perror(argv[1]);
		} else {
			convert(fp, argv[1]);
			fclose(fp);
		}
	}
	exit(0);
}

convert(fp, filename)
FILE *fp;
char *filename;
{
	png_byte *image[512]; /* ugh, but it's easy */
	int x, y;		/* for indexing into pattern */
	int width, height;	/* of image */
	png_byte *line;	/* for quick access to current line in pattern */
	FILE *ofp;		/* output file pointer */

	int	k;
	int	big = 0,
		little = 255;
	int	xsue, ysue;
	char	buf[512];	/* character input buffer */
	int	maxx = 0;	/* longest line found */

	/* Allocate the rows of the image buffer */
	for (k=0; k<512; k++)
		if( (image[k] = malloc(512*sizeof(png_byte))) == NULL) {
			fputs("Out of memory.\n", stderr);
			exit(1);
		}

	/* the following loop was ripped off from fritzz's raytracing program */

	for (ysue = 0;; ysue++) {
		if (fgets (buf, 512, fp) == NULL)
			break;
		xsue = strlen (buf) - 1;/*get rid of EOL tha leaves nasty black dots*/
		if (xsue > maxx) maxx = xsue;
		line = image[ysue];
		for (x = 0; x < xsue; x++) {
			k = buf[x];
			line[x] = k;
			if (big < k)
				big = k;
			if (little > k)
				little = k;
		}
	}

	/* Create output file */
	if (fp == stdin) ofp = stdout;
	else {
		char outputfilename[256];
		char *p;
		strcpy(outputfilename, filename);

		/* Replace trailing ".pat" with ".png" */
		if ( (p = strrchr(outputfilename, '/')) == NULL ) p = filename;
		if ( (p = strrchr(outputfilename, '.')) == NULL) p += strlen(p);
		strcpy(p, ".png");
		
		ofp = fopen(outputfilename, "wb");
		if (ofp == NULL) {
			fputs("Cannot create output file.\n", stderr);
			exit(1);
		}
	}

	write_png(image, maxx, ysue, little, big, ofp);

	exit(0);
}

write_png(image, width, height, min, max, ofp)
png_byte *image[];
int width, height, min, max;
FILE *ofp;
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_color_8p sig_bit;
    int ncolors = max - min + 1;
    int x, y;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	    			      NULL, NULL, NULL);
    if (!png_ptr) exit(1);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	    exit(1);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
	    png_destroy_write_struct(&png_ptr, &info_ptr);
	    fputs("PNG longjmp error.\n", stderr);
	    exit(1);
    }

    png_init_io(png_ptr, ofp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
		 PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    // sig_bit.gray = 8;
    // png_set_sBIT(png_ptr, info_ptr, &sig_bit);
    
    /* Preprocess image to full range of shades of gray */
    for (y = 0; y<height; y++) {
	png_byte *line = image[y];
	for (x=0; x<width; x++) {
	    if (line[x] != 0) /* allow for variable line lengths */
	        /* scale  colors to 0-255 */
	        line[x] = ((line[x] - min) * 256) / ncolors;
	}
    }

    png_write_image(png_ptr, image);

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
}
