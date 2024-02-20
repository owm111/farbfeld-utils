#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define MAX_RECTANGLES 16

struct rectangle {
	int w, h, x, y;
};

static int width, height;
static unsigned int dims[2];
static struct rectangle rectangles[MAX_RECTANGLES];
static int nrectangle;
static int showgaps;
static const unsigned short int fg[4]  = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static const unsigned short int bg[4]  = {0x0000, 0x0000, 0x0000, 0xFFFF};
static const unsigned short int gap[4] = {0x7777, 0x7777, 0x7777, 0xFFFF};

/* does this rectangle have nonnegative dimensions and offsets? */
static int
isvalid(const struct rectangle *r)
{
	return r->x >= 0 && r->y >= 0 && r->w >= 0 && r->h >= 0;
}

/* does *r contain (x, y)? */
static int
containspixel(const struct rectangle *r, int x, int y)
{
	return r->x <= x && x <= r->x + r->w && r->y <= y && y <= r->y + r->h;
}

/* does anything in rectangles contain (x, y)? */
static int
anycontainspixel(int x, int y)
{
	int i;

	for (i = 0; i < nrectangle; i++)
		if (containspixel(&rectangles[i], x, y))
			return 1;
	return 0;
}

/* put the horizontal space between the right edge of *s and the left
edge of *t into *r */
static void
horizgap(struct rectangle *r, const struct rectangle *s,
		const struct rectangle *t)
{
	r->x = s->x + s->w;
	r->w = t->x - r->x;
	r->y = MAX(s->y, t->y);
	r->h = MIN(s->y + s->h, t->y + t->h) - r->y;
}

/* put the vertical space between the bottom edge of *s and the top
edge of *t into *r */
static void
vertgap(struct rectangle *r, const struct rectangle *s,
		const struct rectangle *t)
{
	r->y = s->y + s->h;
	r->h = t->y - r->y;
	r->x = MAX(s->x, t->x);
	r->w = MIN(s->x + s->w, t->x + t->w) - r->x;
}

/* store the gap between *s and *t in *r, or zero it if there is no
such valid gap */
static void
gapbetween(struct rectangle *r, const struct rectangle *s,
		const struct rectangle *t)
{
	horizgap(r, s, t);
	if (isvalid(r))
		return;
	horizgap(r, t, s);
	if (isvalid(r))
		return;
	vertgap(r, s, t);
	if (isvalid(r))
		return;
	vertgap(r, t, s);
	if (isvalid(r))
		return;
	r->w = r->h = r->x = r->y = 0;
}

/* is (x, y) inside any gap between any rectangles? */
static int
inanygap(int x, int y)
{
	int i, j;
	struct rectangle r;

	for (i = 0; i < nrectangle; i++) {
		for (j = i + 1; j < nrectangle; j++) {
			gapbetween(&r, &rectangles[i], &rectangles[j]);
			if (containspixel(&r, x, y))
				return 1;
		}
	}
	return 0;
}

static void
usage(void)
{
	fputs("usage: rectangles [-gv] wxh+x+y ...\n", stderr);
	exit(EXIT_FAILURE);
}

static void
processargs(int argc, char **argv)
{
	int rc;
	int i;
	int opt;

	while (opt = getopt(argc, argv, "gv"), opt != -1) switch (opt) {
	case 'g':
		showgaps = 1;
		break;
	case 'v':
		fputs("rectangles-0.0\n", stderr);
		exit(EXIT_SUCCESS);
		break;
	default:
		usage();
		break;
	}
	if (optind == argc)
		usage();
	if (argc - optind - 1 > MAX_RECTANGLES) {
		fprintf(stderr, "%s: Too many rectangles\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	rc = sscanf(argv[optind], "%dx%d+%*d+%*d", &width, &height);
	if (rc != 2)
		usage();
	dims[0] = htonl(width);
	dims[1] = htonl(height);
	for (i = optind + 1; i < argc; i++) {
		rc = sscanf(argv[i], "%dx%d+%d+%d", &rectangles[nrectangle].w,
				&rectangles[nrectangle].h,
				&rectangles[nrectangle].x,
				&rectangles[nrectangle].y);
		if (rc != 4)
			usage();
		nrectangle++;
	}
}

int
main(int argc, char **argv)
{
	int x, y;
	const unsigned short int *color;

	processargs(argc, argv);
	fwrite("farbfeld", 1, 8, stdout);
	fwrite(dims, 4, 2, stdout);
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			color = bg;
			if (anycontainspixel(x, y)) {
				color = fg;
			} else if (showgaps && inanygap(x, y)) {
				color = gap;
			}
			fwrite(color, 2, 4, stdout);
		}
	}
	return EXIT_SUCCESS;
}
