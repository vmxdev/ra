#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "inih/ini.h"

#define PLANET_NAME_MAX 128

struct vector
{
	double x, y, z;
};

struct body
{
	char name[PLANET_NAME_MAX];

	struct vector c; /* coordinates */
	struct vector v; /* velocity */

	double M;
};

struct nbody
{
	size_t n;
	struct body *b;
	struct body *bnext;
};

static struct vector
vector_add(struct vector a, struct vector b)
{
	struct vector r;

	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;

	return r;
}

static struct vector
vector_sub(struct vector a, struct vector b)
{
	struct vector r;

	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;

	return r;
}

static double
vector_mod(struct vector a)
{
	double mod;

	mod = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);

	return mod;
}

static struct vector
vector_mul(struct vector a, double k)
{
	struct vector r;

	r.x = a.x * k;
	r.y = a.y * k;
	r.z = a.z * k;

	return r;
}


static struct vector
acceleration(struct nbody *nb, size_t i)
{
	struct vector a = {0.0, 0.0, 0.0};
	size_t j;

	for (j=0; j<nb->n; j++) {
		struct vector r;
		double rmod;

		if (i == j) {
			continue;
		}

		r = vector_sub(nb->b[j].c, nb->b[i].c);

		rmod = vector_mod(r);
		if ((rmod > DBL_EPSILON) || (rmod < -DBL_EPSILON)) {
			struct vector da;

			da = vector_mul(r, nb->b[j].M / (rmod * rmod * rmod));
			a = vector_add(a, da);
		}
	}

	return a;
}


static void
calc_next_step(struct nbody *nb, size_t i, double dt)
{
	struct vector a;

	a = acceleration(nb, i);

	nb->bnext[i].c = vector_add(nb->b[i].c, vector_mul(nb->b[i].v, dt));
	nb->bnext[i].v = vector_add(nb->b[i].v, vector_mul(a, dt));
}


static void
nbody_nextstep(struct nbody *nb, double delta)
{
	size_t i;

	for (i=0; i<nb->n; i++) {
		calc_next_step(nb, i, delta);
	}

	for (i=0; i<nb->n; i++) {
		nb->b[i].v = nb->bnext[i].v;
		nb->b[i].c = nb->bnext[i].c;
	}
}

static void
ra_ini_key(struct body *b, const char *name, const char *value)
{
#define KEY(STR, V)                    \
	if (strcmp(name, STR) == 0) { \
		V = atof(value);  \
		return;                \
	}
	KEY("cx", b->c.x);
	KEY("cy", b->c.y);
	KEY("cz", b->c.z);

	KEY("vx", b->v.x);
	KEY("vy", b->v.y);
	KEY("vz", b->v.z);

	KEY("M", b->M);
#undef KEY
}

static int
ra_ini_handler(void *user, const char *section, const char *name,
		const char *value)
{
	struct nbody *nb = (struct nbody *)user;
	const char *planet_pfx = "planet_";
	size_t i;

	if (strncmp(section, planet_pfx, strlen(planet_pfx)) == 0) {
		int found = 0;
		const char *planet_name = section + strlen(planet_pfx);

		for (i=0; i<nb->n; i++) {
			if (strcmp(nb->b[i].name, planet_name) == 0) {
				/* existing planet */
				found = 1;
				break;
			}
		}
		if (!found) {
			struct body *tmp;

			tmp = realloc(nb->b, sizeof(struct body)*(nb->n + 1));
			if (!tmp) {
				free(nb->b);
				fprintf(stderr, "Insufficient memory\n");
				return 0;
			}
			nb->b = tmp;
			strcpy(nb->b[i].name, planet_name);
			nb->n++;
		}
		ra_ini_key(&nb->b[i], name, value);
	} else {
		fprintf(stderr, "Unknown section '%s'\n", section);
	}

	return 1;
}

static void
usage(char *progname, double duration, double delta, double samplerate)
{
	fprintf(stderr, "Usage: %s [-d duration] [-t delta] [-r samplerate] planets.ini\n",
		progname);
	fprintf(stderr, " where:\n");
	fprintf(stderr, "  -d | --duration  time (in seconds) of simulation, default %f\n",
		duration);
	fprintf(stderr, "  -t | --delta  step (in seconds) between simulation steps, default %f\n",
		delta);
	fprintf(stderr, "  -r | --samplerate  results will be printed each samplerate second, default %f\n",
		samplerate);
	fprintf(stderr, "  planets.ini  INI-file with planets description\n");
}


int
main(int argc, char *argv[])
{
	struct nbody solar_system;
	char *ini_file = NULL;

	/* parameters with some reasonable defaults */
	double duration = 60.0 * 60.0 * 24.0 * 365.256; /* one year */
	double delta = 1.0; /* one second */
	double samplerate = 60.0 * 60.0 * 24.0; /* one day */

	double passed = 0.0;
	uint64_t prev_sr = 1;

	/* command-line options */
	while (1) {
		int c;

		int option_index = 0;
		static struct option long_options[] = {
			{"duration",    required_argument, 0, 'd'},
			{"delta",       required_argument, 0, 't'},
			{"samplerate",  required_argument, 0, 'r'},
			{0,             0,                 0,  0 }
		};

		c = getopt_long(argc, argv, "d:r:t:",
			long_options, &option_index);
		if (c == -1) {
			break;
		} else if (c == 0) {
			c = long_options[option_index].val;
		}

		switch (c) {
			case 'd':
				duration = atof(optarg);
				break;
			case 't':
				delta = atof(optarg);
				break;
			case 'r':
				samplerate = atof(optarg);
				break;
		}
	}

	if (optind != (argc - 1)) {
		usage(argv[0], duration, delta, samplerate);
		return EXIT_FAILURE;
	}

	memset(&solar_system, 0, sizeof(struct nbody));

	/* parse INI file with planets description */
	ini_file = argv[optind];
	if (ini_parse(ini_file, &ra_ini_handler, &solar_system) < 0) {
		fprintf(stderr, "Can't parse '%s'\n", ini_file);
		return EXIT_FAILURE;
	}

	solar_system.bnext = malloc(solar_system.n * sizeof(struct body));
	if (!solar_system.bnext) {
		fprintf(stderr, "Insufficient memory\n");
		return EXIT_FAILURE;
	}

	while (passed <= duration) {
		nbody_nextstep(&solar_system, delta);

		if (prev_sr != (uint64_t)(passed / samplerate)) {
			size_t i;

			printf("%lu\n", (unsigned long)prev_sr);
			for (i=0; i<solar_system.n; i++) {
				printf("%s: x = %f, y = %f, z = %f\n",
					solar_system.b[i].name,
					solar_system.b[i].c.x,
					solar_system.b[i].c.y,
					solar_system.b[i].c.z);
			}
			printf("\n");

			prev_sr = (uint64_t)(passed / samplerate);
		}

		passed += delta;
	}

	free(solar_system.bnext);
	free(solar_system.b);

	return 0;
}

