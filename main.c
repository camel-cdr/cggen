#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include </home/camel/dev/cauldron/cauldron/random.h>
#include </home/camel/git/cauldron/cauldron/stretchy-buffer.h>

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

static Sb(struct Lable) lables;
static Sb(struct Chord) chords;
static char *file;

static void
shuf_lables(uint64_t (*rand64)(void*), void *rng)
{
	size_t n = sb_len(lables);
	for (; n >= 2; n--) {
		size_t li = n-1;
		size_t ri = dist_uniform_u64(n, rand64, rng);
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
	PRNG64RomuQuad prng64;
	ShufLcg lcg;
	prng64_romu_quad_randomize(&prng64);

	sb_init(lables);
	sb_init(chords);

	load_csv("data/hp.csv");
	shuf_lables(prng64_romu_quad, &prng64);

	size_t total = 0;

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
		total += 5000;
	}

	puts("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	puts("<svg version=\"1.1\" width=\"200\" height=\"200\">");
	puts("<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>");

	puts("<g transform=\"translate(10,10) scale(0.9,0.9)\">");

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

		       "\t\t<stop offset=\"0%\" stop-color=\"#%X\"/>\n"
		       "\t\t<stop offset=\"100%\" stop-color=\"#%X\"/>\n"
		       "\t</linearGradient>\n"
		       "</defs>\n",
			i, lx, ly, rx, ry, l->col, r->col);

		printf("<path fill-opacity=\"0.7\" fill=\"url(#grad%zu)\" d=\"\n"
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
	}

#if 0
	printf("<g transform=\"translate(%f,%f) rotate(%f) scale(0.4,0.4)\">",
	       l1x, l1y, (1+atan2(100-l1x, l1y-100) / PI) * 180.0 + 90);
	printf("<text dx=\"-%fem\">%s</text>", strlen(r->name) / 2.0, r->name);
	printf("</g>");
#endif

	puts("</g>");
	puts("</svg>");

	return EXIT_SUCCESS;
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
		trng_write(&lbl.col, sizeof lbl.col);
		lbl.col &= 0xFFFFFF;
		sb_init(lbl.chords);
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

		if (cnt > 500)
			load_chords_row(cnt, l, r);
	}
}
