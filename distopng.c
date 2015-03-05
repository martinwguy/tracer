/*
 *	Convert Fritzz format display files to PNG image file
 *
 *	Usage: distopng [file]
 *
 *	With no filename, reads stdin and writes to stdout
 *	With a filename, converts *.image to *.png
 *
 *	Martin Guy <martinwguy@gmail.com>, March 2015
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
	png_byte **image;		/* Array of pointer to line */
	int width, height;	/* of image */
	FILE *ofp;		/* output file pointer */
	int y;			/* for indexing into image */

	if (fscanf(fp, "%d %d\n", &width, &height) != 2) {
		fputs("Cannot read image width and height.\n", stderr);
		exit(1);
	}

	/* Allocate the image buffer */
	image = malloc(height * sizeof (*image));
	for (y=0; y<height; y++)
		if( (image[y] = malloc(width*sizeof(**image))) == NULL) {
			fputs("Out of memory.\n", stderr);
			exit(1);
		}

	/* Read the image in */
	for (y = 0; y<height; y++) {
		if (fread (image[y], 1, width, fp) != width) {
			fputs("Premature EOF reading input file.\n", stderr);
			exit(1);
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

	write_png(image, width, height, ofp);

	exit(0);
}

write_png(image, width, height, ofp)
png_byte *image[];
int width, height;
FILE *ofp;
{
    png_structp png_ptr;
    png_infop info_ptr;

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

    png_write_image(png_ptr, image);

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
}
