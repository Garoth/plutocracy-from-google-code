/* Copyright (C) 1991-2003,2004,2005,2006,2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Modified by Devin Papineau, 2008. */

#include "c_shared.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#define __set_errno(ev) ((errno) = (ev))

struct c_glibc_random_data {
    int32_t *fptr;		/* Front pointer.  */
    int32_t *rptr;		/* Rear pointer.  */
    int32_t *state;		/* Array of state values.  */
    int rand_type;		/* Type of random number generator.  */
    int rand_deg;		/* Degree of random number generator.  */
    int rand_sep;		/* Distance between front and rear.  */
    int32_t *end_ptr;		/* Pointer behind state table.  */
};

/* Linear congruential.  */
#define	TYPE_0		0
#define	BREAK_0		8
#define	DEG_0		0
#define	SEP_0		0

/* x**7 + x**3 + 1.  */
#define	TYPE_1		1
#define	BREAK_1		32
#define	DEG_1		7
#define	SEP_1		3

/* x**15 + x + 1.  */
#define	TYPE_2		2
#define	BREAK_2		64
#define	DEG_2		15
#define	SEP_2		1

/* x**31 + x**3 + 1.  */
#define	TYPE_3		3
#define	BREAK_3		128
#define	DEG_3		31
#define	SEP_3		3

/* x**63 + x + 1.  */
#define	TYPE_4		4
#define	BREAK_4		256
#define	DEG_4		63
#define	SEP_4		1


/* Array versions of the above information to make code run faster.
   Relies on fact that TYPE_i == i.  */

#define	MAX_TYPES	5	/* Max number of types above.  */


/* Initially, everything is set up as if from:
	initstate(1, randtbl, 128);
   Note that this initialization takes advantage of the fact that srandom
   advances the front and rear pointers 10*rand_deg times, and hence the
   rear pointer which starts at 0 will also end up at zero; thus the zeroth
   element of the state information, which contains info about the current
   position of the rear pointer is just
	(MAX_TYPES * (rptr - state)) + TYPE_3 == TYPE_3.  */

static int32_t randtbl[DEG_3 + 1] =
{
        TYPE_3,

        -1726662223, 379960547, 1735697613, 1040273694, 1313901226,
        1627687941, -179304937, -2073333483, 1780058412, -1989503057,
        -615974602, 344556628, 939512070, -1249116260, 1507946756,
        -812545463, 154635395, 1388815473, -1926676823, 525320961,
        -1009028674, 968117788, -123449607, 1284210865, 435012392,
        -2017506339, -911064859, -370259173, 1132637927, 1398500161,
        -205601318,
};

static struct c_glibc_random_data unsafe_state = {
/* FPTR and RPTR are two pointers into the state info, a front and a rear
   pointer.  These two pointers are always rand_sep places aparts, as they
   cycle through the state information.  (Yes, this does mean we could get
   away with just one pointer, but the code for random is more efficient
   this way).  The pointers are left positioned as they would be from the call:
   initstate(1, randtbl, 128);
   (The position of the rear pointer, rptr, is really 0 (as explained above
   in the initialization of randtbl) because the state table pointer is set
   to point to randtbl[1] (as explained below).)  */

        .fptr = &randtbl[SEP_3 + 1],
        .rptr = &randtbl[1],

/* The following things are the pointer to the state information table,
   the type of the current generator, the degree of the current polynomial
   being used, and the separation between the two pointers.
   Note that for efficiency of random, we remember the first location of
   the state information, not the zeroth.  Hence it is valid to access
   state[-1], which is used to store the type of the R.N.G.
   Also, we remember the last location, since this is more efficient than
   indexing every time to find the address of the last element to see if
   the front and rear pointers have wrapped.  */

        .state = &randtbl[1],

        .rand_type = TYPE_3,
        .rand_deg = DEG_3,
        .rand_sep = SEP_3,

        .end_ptr = &randtbl[sizeof (randtbl) / sizeof (randtbl[0])]
};

/* If we are using the trivial TYPE_0 R.N.G., just do the old linear
   congruential bit.  Otherwise, we do our fancy trinomial stuff, which is the
   same in all the other cases due to all the global variables that have been
   set up.  The basic operation is to add the number at the rear pointer into
   the one at the front pointer.  Then both pointers are advanced to the next
   location cyclically in the table.  The value returned is the sum generated,
   reduced to 31 bits by throwing away the "least random" low bit.
   Note: The code takes advantage of the fact that both the front and
   rear pointers can't wrap on the same call by not testing the rear
   pointer if the front one has wrapped.  Returns a 31-bit random number.  */

static int glibc__random_r(struct c_glibc_random_data *buf, int32_t *result)
{
        int32_t *state;

        if (buf == NULL || result == NULL)
                goto fail;

        state = buf->state;

        if (buf->rand_type == TYPE_0) {
                int32_t val = state[0];
                val = ((state[0] * 1103515245) + 12345) & 0x7fffffff;
                state[0] = val;
                *result = val;
        } else {
                int32_t *fptr = buf->fptr;
                int32_t *rptr = buf->rptr;
                int32_t *end_ptr = buf->end_ptr;
                int32_t val;

                val = *fptr += *rptr;
                /* Chucking least random bit.  */
                *result = (val >> 1) & 0x7fffffff;
                ++fptr;
                if (fptr >= end_ptr) {
                        fptr = state;
                        ++rptr;
                } else {
                        ++rptr;
                        if (rptr >= end_ptr)
                                rptr = state;
                }
                buf->fptr = fptr;
                buf->rptr = rptr;
        }
        return 0;

fail:
        __set_errno (EINVAL);
        return -1;
}

/* If we are using the trivial TYPE_0 R.N.G., just do the old linear
   congruential bit.  Otherwise, we do our fancy trinomial stuff, which is the
   same in all the other cases due to all the global variables that have been
   set up.  The basic operation is to add the number at the rear pointer into
   the one at the front pointer.  Then both pointers are advanced to the next
   location cyclically in the table.  The value returned is the sum generated,
   reduced to 31 bits by throwing away the "least random" low bit.
   Note: The code takes advantage of the fact that both the front and
   rear pointers can't wrap on the same call by not testing the rear
   pointer if the front one has wrapped.  Returns a 31-bit random number.  */

long int C_glibc_rand(void)
{
  int32_t retval;

  /*__libc_lock_lock (lock);*/

  (void) glibc__random_r (&unsafe_state, &retval);

  /*__libc_lock_unlock (lock);*/

  return retval;
}

/* Initialize the random number generator based on the given seed.  If the
   type is the trivial no-state-information type, just remember the seed.
   Otherwise, initializes state[] based on the given "seed" via a linear
   congruential generator.  Then, the pointers are set to known locations
   that are exactly rand_sep places apart.  Lastly, it cycles the state
   information a given number of times to get rid of any initial dependencies
   introduced by the L.C.R.N.G.  Note that the initialization of randtbl[]
   for default usage relies on values produced by this routine.  */

static int glibc__srandom_r(unsigned int seed, struct c_glibc_random_data *buf)
{
        int type;
        int32_t *state;
        long int i;
        long int word;
        int32_t *dst;
        int kc;

        if (buf == NULL)
                goto fail;
        type = buf->rand_type;
        if ((unsigned int) type >= MAX_TYPES)
                goto fail;

        state = buf->state;
        /* We must make sure the seed is not 0.  Take arbitrarily 1 in this case.  */
        if (seed == 0)
                seed = 1;
        state[0] = seed;
        if (type == TYPE_0)
                goto done;

        dst = state;
        word = seed;
        kc = buf->rand_deg;
        for (i = 1; i < kc; ++i) {
                /* This does:
                   state[i] = (16807 * state[i - 1]) % 2147483647;
                   but avoids overflowing 31 bits.  */
                long int hi = word / 127773;
                long int lo = word % 127773;
                word = 16807 * lo - 2836 * hi;
                if (word < 0)
                        word += 2147483647;
                *++dst = word;
        }

        buf->fptr = &state[buf->rand_sep];
        buf->rptr = &state[0];
        kc *= 10;
        while (--kc >= 0) {
                int32_t discard;
                (void) glibc__random_r (buf, &discard);
        }

done:
        return 0;

fail:
        return -1;
}

/* Initialize the random number generator based on the given seed.  If the
   type is the trivial no-state-information type, just remember the seed.
   Otherwise, initializes state[] based on the given "seed" via a linear
   congruential generator.  Then, the pointers are set to known locations
   that are exactly rand_sep places apart.  Lastly, it cycles the state
   information a given number of times to get rid of any initial dependencies
   introduced by the L.C.R.N.G.  Note that the initialization of randtbl[]
   for default usage relies on values produced by this routine.  */

void C_glibc_srand(unsigned int x)
{
        /*__libc_lock_lock (lock);*/
        (void) glibc__srandom_r (x, &unsafe_state);
        /*__libc_lock_unlock (lock);*/
}
