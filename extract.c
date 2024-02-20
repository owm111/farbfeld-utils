#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define MAX_RECTANGLES 16

struct rectangle {
	int w, h, x, y;
};

static int width, height;
static struct rectangle rectangles[MAX_RECTANGLES];
static int nrectangle;
static FILE *outputs[MAX_RECTANGLES];

static int
containspixel(const struct rectangle *r, int x, int y)
{
	return r->x <= x && x < r->x + r->w && r->y <= y && y < r->y + r->h;
}

static void
usage(void)
{
	fputs("usage: extract [-v] wxh+x+y ...\n", stderr);
	exit(EXIT_FAILURE);
}

static void
processargs(int argc, char **argv)
{
	int rc;
	int i;
	int opt;

	while (opt = getopt(argc, argv, "v"), opt != -1) switch (opt) {
	case 'v':
		fputs("extract-0.0\n", stderr);
		exit(EXIT_SUCCESS);
		break;
	default:
		usage();
		break;
	}
	if (optind == argc)
		usage();
	if (argc - optind > MAX_RECTANGLES) {
		fprintf(stderr, "%s: Too many rectangles\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	for (i = optind; i < argc; i++) {
		rc = sscanf(argv[i], "%dx%d+%d+%d", &rectangles[nrectangle].w,
				&rectangles[nrectangle].h,
				&rectangles[nrectangle].x,
				&rectangles[nrectangle].y);
		if (rc != 4)
			usage();
		nrectangle++;
	}
}

static void
initinput(void)
{
	char magic[8];
	unsigned int dims[2];

	fread(magic, 1, 8, stdin);
	if (memcmp(magic, "farbfeld", 8) != 0) {
		fprintf(stderr, "Invalid Farbfeld image on the standard "
				"input\n");
		exit(EXIT_FAILURE);
	}
	fread(dims, 4, 2, stdin);
	width = ntohl(dims[0]);
	height = ntohl(dims[1]);
}

static void
initoutputs(void)
{
	int i;
	unsigned int dims[2];
	char path[8];

	for (i = 0; i < nrectangle; i++) {
		snprintf(path, sizeof(path), "%d.ff", (int)(char)i);
		outputs[i] = fopen(path, "w");
		if (outputs[i] == NULL) {
			perror(path);
			exit(EXIT_FAILURE);
		}
		fwrite("farbfeld", 1, 8, outputs[i]);
		dims[0] = htonl(rectangles[i].w);
		dims[1] = htonl(rectangles[i].h);
		fwrite(dims, 4, 2, outputs[i]);
	}
}

static void
closeoutputs(void)
{
	int i;

	for (i = 0; i < nrectangle; i++)
		fclose(outputs[i]);
}

static void
iterate(void)
{
	int x, y, i;
	unsigned short int color[4];

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			fread(color, 2, 4, stdin);
			for (i = 0; i < nrectangle; i++) {
				if (!containspixel(&rectangles[i], x, y))
					continue;
				fwrite(color, 2, 4, outputs[i]);
			}
		}
	}
}

int
main(int argc, char **argv)
{
	processargs(argc, argv);
	initinput();
	initoutputs();
	iterate();
	closeoutputs();
	return EXIT_SUCCESS;
}
