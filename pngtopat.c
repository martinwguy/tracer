/*
 *	Convert PNG images file to Fritzz format pattern file
 *
 *	Usage: pngtopat [file]
 *
 *	With no filename, reads stdin and writes to stdout
 *	With a filename, converts *.png to *.pat
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
		fp = fopen(argv[1], "rb");
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

convert(ifp, filename)
FILE *ifp;
char *filename;
{
    char header[8];		/* PNG file magic number */
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    png_byte **image;	/* Array of pointer to line */
    unsigned width, height;	/* of image */
    FILE *ofp;		/* output file pointer */
    int y;			/* for indexing into image */

    if (fread(header, sizeof(char), sizeof(header), ifp) != sizeof(header) ||
	png_sig_cmp(header, 0, sizeof(header)) != 0) {
	    fputs("Not a PNG file.\n", stderr);
	    exit(1);
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr) {
	info_ptr = png_create_info_struct(png_ptr);
	end_info = png_create_info_struct(png_ptr);
    }
    if (!png_ptr || !info_ptr || !end_info) {
	fputs("Cannot creat PNG structure.<n", stderr);
	exit(1);
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        exit(1);
    }

    png_init_io(png_ptr, ifp);

    png_set_sig_bytes(png_ptr, sizeof(header));

    /* Use the low-level interface so that we can have it convert
     * any image tupe to grayscale */
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);

    /*
     *  Convert all color formats to 8-bit gray
     */
    if (png_get_bit_depth(png_ptr, info_ptr) == 16)
	png_set_strip_16(png_ptr);

    if (png_get_color_type(png_ptr, info_ptr) & PNG_COLOR_MASK_ALPHA)
	png_set_strip_alpha(png_ptr);

    if (png_get_color_type(png_ptr, info_ptr) & PNG_COLOR_MASK_PALETTE)
	png_set_palette_to_rgb(png_ptr);

    switch (png_get_color_type(png_ptr, info_ptr)) {
	case PNG_COLOR_TYPE_RGB:
	case PNG_COLOR_TYPE_RGB_ALPHA:
	    png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);
	    break;
	case PNG_COLOR_TYPE_GRAY:
	    if (png_get_bit_depth(png_ptr, info_ptr) < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	    break;
    }
    
    /* Have libpng handle deinterlacing if present */
    (void) png_set_interlace_handling(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    /*
     * Allocate storage area for image
     */
    
    if ((image = malloc(height * sizeof(*image))) == NULL) goto oom;
    for (y=0; y<height; y++) {
        if ( (image[y] = malloc(width * sizeof(**image))) == NULL) {
oom:	    fputs("Out of memory.\n", stderr); exit(1);
	    exit(1);
	}
    }

    png_read_image(png_ptr, image);
    
    /* Create output file */
    if (ifp == stdin) ofp = stdout;
    else {
	char outputfilename[256];
	char *p;
	strcpy(outputfilename, filename);

	/* Replace trailing ".png" with ".pat" */
	if ( (p = strrchr(outputfilename, '/')) == NULL ) p = filename;
	if ( (p = strrchr(outputfilename, '.')) == NULL) p += strlen(p);
	strcpy(p, ".pat");
	    
	ofp = fopen(outputfilename, "w");
	if (ofp == NULL) {
	    fputs("Cannot create output file.\n", stderr);
	    exit(1);
	}
    }

    write_pat(image, width, height, ofp);

    exit(0);
}

/*
 * Write the pattern file, which is a bare text file where each
 * character represents one pixel. Character values are in the
 * range '@' (64) to '~' (126).
 */
write_pat(image, width, height, ofp)
png_byte *image[];
int width, height;
FILE *ofp;
{
    int x, y;

    for (y = 0; y<height; y++) {
	png_byte *line = image[y];
	for (x=0; x<width; x++)
	    putc('@' + (line[x] * ('~' - '@')) / 255, ofp);
	putc('\n', ofp);
    }
}
