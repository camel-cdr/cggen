#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RANDOM_H_IMPLEMENTATION
#include "vendor/random.h"
#include "vendor/stretchy-buffer.h"

#define PI 3.141592653589793238462643383279502884
#define LENGTH(a) (sizeof (a) / sizeof *(a))


struct Lable {
	char *name;
	Sb(size_t) chords;
	uint32_t col;
};

struct Chord {
	size_t cnt, l, r;
	size_t lbeg, lend, rbeg, rend;
};

static size_t lable_intern(char *name);
static void load_chords_row(size_t cnt, char *l, char *r);
static void load_csv(char *filename);

static void shuf_lables(void);

static void print_chords(void);

static Sb(struct Lable) lables;
static Sb(struct Chord) chords;
static char *file;
static size_t total;

#define prng64_next prng64_romu_duo_jr
#define prng64_randomize prng64_romu_duo_randomize
static PRNG64RomuDuo prng64;

void
calc_angles(double *x, double *y, size_t total, size_t cur)
{
	double t = (2.0 * PI * cur) / total;
	*x = 100.0 * (sin(t) + 1.0);
	*y = 100.0 * (cos(t) + 1.0);
}

int
comp_chords(const void *lhs, const void *rhs, void *arg)
{
	const size_t *li = lhs, *ri = rhs, *a = arg;

	size_t l = *a == chords.at[*li].l ? chords.at[*li].r : chords.at[*li].l;
	size_t r = *a == chords.at[*ri].l ? chords.at[*ri].r : chords.at[*ri].l;

	if (l < *a && r > *a) {
		return -1;
	} else if (l > *a && r < *a) {
		return 1;
	}

	return r - l;
}

int
main(void)
{
	size_t i, j, k;

	prng64_randomize(&prng64);
	load_csv("data/worm.csv");
	shuf_lables();

	size_t sum = 0;
	for (i = 0; i < sb_len(lables); ++i) {
		struct Lable *l = &lables.at[i];
		for (j = 0; j < sb_len(l->chords); ++j) {
			struct Chord *c = &chords.at[l->chords.at[j]];
			sum += c->cnt;
		}
	}

	size_t scale = (sum * 0.25 / sb_len(lables)) + 1;

	for (i = 0; i < sb_len(lables); ++i) {
		struct Lable *l = &lables.at[i];
		qsort_r(l->chords.at, sb_len(l->chords), sizeof *l->chords.at,
		         comp_chords, &i);

		for (j = 0; j < sb_len(l->chords); ++j) {
			struct Chord *c = &chords.at[l->chords.at[j]];
			if (i == c->l) {
				c->lbeg = total;
				c->lend = (total += c->cnt);
			} else {
				c->rbeg = total;
				c->rend = (total += c->cnt);
			}
		}
		total += scale;
	}

	print_chords();

	return EXIT_SUCCESS;
}

static uint32_t
HSVtoRGB(float H, float S, float V)
{
	const unsigned idx = H * 6;
	const float C = V * S;
	const float X = C * (1 - fabsf(fmodf(H*6, 2) - 1));
	const float m = V - C;
	float r=0, g=0, b=0;

	switch (idx) {
	case 0: r=C; g=X; b=0; break;
	case 1: r=X; g=C; b=0; break;
	case 2: r=0; g=C; b=X; break;
	case 3: r=0; g=X; b=C; break;
	case 4: r=X; g=0; b=C; break;
	case 5: r=C; g=0; b=X; break;
	}

	uint8_t R = (r+m)*255;
	uint8_t G = (g+m)*255;
	uint8_t B = (b+m)*255;
	return (R<<16) + (G<<8) + B;
}

size_t
lable_intern(char *name)
{
	size_t i;
	for (i = 0; i != sb_len(lables); ++i)
		if (strcmp(name, lables.at[i].name) == 0)
			break;
	if (i == sb_len(lables)) {
		struct Lable lbl = { name };
		while (!lbl.col) {
			lbl.col = HSVtoRGB(
				dist_uniformf(prng64_next(&prng64)),
				dist_uniformf(prng64_next(&prng64))*0.8 + 0.2,
				dist_uniformf(prng64_next(&prng64))*0.4 + 0.6);
#if 0
			/* try to force more diverse colors */
			for (i = 0; i != sb_len(lables); ++i) {
				uint32_t x = lbl.col, y = lables.at[i].col;
				uint32_t a =(x&0xff)-(y&0xff); x >>= 8; y >>= 8;
				uint32_t b =(x&0xff)-(y&0xff); x >>= 8; y >>= 8;
				uint32_t c =(x&0xff)-(y&0xff);
				if (a*a+b*b+c*c < 80*80)
					lbl.col = 0;
			}
#endif
		}
		sb_push(lables, lbl);
	}
	return i;
}

void
load_chords_row(size_t cnt, char *l, char *r)
{
	size_t i, j, k;
	struct Chord *c, *cend;

	struct Chord chord = { cnt };

	chord.l = lable_intern(l);
	chord.r = lable_intern(r);

	sb_push(lables.at[chord.l].chords, sb_len(chords));
	sb_push(lables.at[chord.r].chords, sb_len(chords));

	sb_push(chords, chord);
}

void
load_csv(char *filename)
{
	char *it;

	long fsize;
	FILE *f = fopen(filename, "r");

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	file = malloc(fsize + 1);
	fread(file, 1, fsize, f);
	fclose(f);

	it = file;
	while (1) {
		char *beg, *l, *r;
		size_t cnt;

		beg = it;

		if (!(it = strchr(it, ',')))
			break;
		*it++ = 0;
		l = it;

		cnt = atol(beg);

		if (!(it = strchr(it, ',')))
			break;
		*it++ = 0;
		r = it;

		if (!(it = strchr(it, '\n')))
			break;
		*it++ = 0;

		if (cnt > 4)
			load_chords_row(cnt, l, r);
	}
}

void
shuf_lables(void)
{
	size_t n = sb_len(lables);
	for (; n >= 2; n--) {
		size_t li = n-1;
		size_t ri = dist_uniform_u64(n, prng64_next, &prng64);
		struct Lable *l = &lables.at[li];
		struct Lable *r = &lables.at[ri];

		/* swap */
		struct Lable tmp = *r;
		*r = *l;
		*l = tmp;

		/* swap indexes */
		for (size_t i = 0; i < sb_len(chords); ++i) {
			/**/ if (chords.at[i].l == ri)
				chords.at[i].l = li;
			else if (chords.at[i].l == li)
				chords.at[i].l = ri;


			/**/ if (chords.at[i].r == ri) {
				chords.at[i].r = li;
			}
			else if (chords.at[i].r == li)
				chords.at[i].r = ri;
		}
	}
}

void
print_chords(void)
{
	size_t i;
	puts("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	puts("<svg version=\"1.1\" width=\"200\" height=\"200\">");
	puts("<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>");

	puts("<g transform=\"translate(40,40) scale(0.60,0.60)\">");

	for (i = 0; i < sb_len(chords); ++i) {
		double l1x, l1y, l2x, l2y;
		double r1x, r1y, r2x, r2y;

		struct Chord *c = &chords.at[i];
		struct Lable *l = &lables.at[c->l];
		struct Lable *r = &lables.at[c->r];

		calc_angles(&l1x, &l1y, total, c->lbeg);
		calc_angles(&l2x, &l2y, total, c->lend);
		calc_angles(&r1x, &r1y, total, c->rbeg);
		calc_angles(&r2x, &r2y, total, c->rend);

		double lx, ly, rx, ry;
		if (pow(l1x - r2x, 2) + pow(l1y - r2y, 2) >
		    pow(l2x - r1x, 2) + pow(l2y - r1y, 2)) {
			lx = l1x;
			ly = l1y;
			rx = r2x;
			ry = r2y;
		} else {
			lx = l2x;
			ly = l2y;
			rx = r1x;
			ry = r1y;
		}

		printf("<defs>\n"
		       "\t<linearGradient id=\"grad%zu\" "
			"gradientUnits=\"userSpaceOnUse\" "
		       "x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\">\n"

		       "\t\t<stop offset=\"5%%\" stop-color=\"#%06X\"/>\n"
		       "\t\t<stop offset=\"95%%\" stop-color=\"#%06X\"/>\n"
		       "\t</linearGradient>\n"
		       "</defs>\n",
			i, lx, ly, rx, ry, l->col, r->col);


		printf("<path fill-opacity=\"0.75\" fill=\"url(#grad%zu)\" d=\"\n"
				"M%f,%f\n"
				"Q100,100 %f,%f\n"
				"A100,100,0,0,1, %f,%f\n"
				"Q100,100 %f,%f\n"
				"A100,100,0,0,1, %f,%f Z\n"
			"\"/>\n",
			i,
			l1x, l1y,
			r2x, r2y,
			r1x, r1y,
			l2x, l2y,
			l1x, l1y
		);

		// This needs to be cleaned up manually
		printf("<g transform=\"translate(%f,%f) scale(0.4,0.4)\">",
		(l1x + l2x) / 2, (l1y + l2y) / 2);
		printf("<text font-weight=\"bold\">%s</text>", l->name);
		printf("</g>");

		printf("<g transform=\"translate(%f,%f) scale(0.4,0.4)\">",
		(r1x + r2x) / 2, (r1y + r2y) / 2);
		printf("<text font-weight=\"bold\">%s</text>", r->name);
		printf("</g>");
	}



	puts("</g>");
	puts("</svg>");
}

