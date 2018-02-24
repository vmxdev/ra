#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdlib.h>

struct vector
{
	double x, y, z;
};

struct body
{
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
nbody_nextstep(struct nbody *nb)
{
	size_t i;

	for (i=0; i<nb->n; i++) {
		calc_next_step(nb, i, 1.0);
	}

	for (i=0; i<nb->n; i++) {
		nb->b[i].v = nb->bnext[i].v;
		nb->b[i].c = nb->bnext[i].c;
	}
}



int
main()
{
	struct nbody solar_system;
	size_t i;

	solar_system.n = 2;
	solar_system.b = malloc(solar_system.n * sizeof(struct body));
	solar_system.bnext = malloc(solar_system.n * sizeof(struct body));

	solar_system.b[0].c.x = 0.0;
	solar_system.b[0].c.y = 0.0;
	solar_system.b[0].c.z = 0.0;

	solar_system.b[0].v.x = 0.0;
	solar_system.b[0].v.y = 0.0;
	solar_system.b[0].v.z = 0.0;
	solar_system.b[0].M = 1.32712440018e20;

	solar_system.b[1].c.x = 1.496e11;
	solar_system.b[1].c.y = 0.0;
	solar_system.b[1].c.z = 0.0;

	solar_system.b[1].v.x = 0.0;
	solar_system.b[1].v.y = 29722.0;
	solar_system.b[1].v.z = 0.0;
	solar_system.b[1].M = 3.986004418e14;

	for (i=0; i<60*60*24*365; i++) {
		size_t j;
		const size_t samplerate = 60*60*24;

		nbody_nextstep(&solar_system);

		if (i % samplerate) {
			continue;
		}

		for (j=0; j<solar_system.n; j++) {
			printf("%u: %f/%f/%f\n",
				i / samplerate,
				solar_system.b[j].c.x,
				solar_system.b[j].c.y,
				solar_system.b[j].c.z);
		}
		printf("\n");
	}

	free(solar_system.bnext);
	free(solar_system.b);

	return 0;
}

