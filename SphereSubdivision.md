#summary David Severwright's concept for how to make tiled spheres of different but **reasonable** sizes.

# Preamble #
I'm assuming you either know all about [making spheres via triangle subdivision](http://student.ulb.ac.be/~claugero/sphere/index.html) or are interested in this for some other reason.  I also assume you know more about geometry than me, which isn't hard.

As you probably know, Plutocracy uses a spherical map.  And they use triangle subdivision to do it, with an icosahedron (20 faces, 12 verts) as a base.  When sub dividing from that, the number of faces goes up a bit too quickly, it snaps straight from too small to fucking huge.  So faced with the prospect of having only one map size, a meeting was called which I decided to gate crash.  The reason this page exists is because a long time was spent on Google, and this stuff wasn't found.  The reason I am writing it is because I thought of it and no one else would do it for me.

# Old way #
![http://www.davidsev.co.uk/stuff/sphere/old_way.png](http://www.davidsev.co.uk/stuff/sphere/old_way.png)

The old way, as documented a lot, is you get an object, and subdivide each side, and keep going until you have enough.  Each round of subdivision turns each face into four, so you get some rapid growth.

This results in 320, 1280, 5120, 20480 and so on.  This sucks.  We needed to fill in the blanks.

# Solution #
The solution is simple.  Don't divide each face into 4.  Pick another number and go for that.

One key requirement is that each triangle is the same size, which limits our choice somewhat.  The easiest way is to pick a number, and split each edge into that many edges, and then fill in the middle.  In the 'old way' each edge is split into two on each iteration.  We can't do less than that, so we do more.

![http://www.davidsev.co.uk/stuff/sphere/new_way.png](http://www.davidsev.co.uk/stuff/sphere/new_way.png)

With each face split into three, the rate at which faces multiply is much grater, 9 as opposed to 4.  (The number of faces made is the number of edges each was split to squared).  Despite this being a faster climb than before, it provides a new set of sphere sizes to chose from, increasing the number within the range suitable for maps.
If we are to split each edge into four, then we would get the same result as splitting into two twice, as shown in the first image on this page.  Similarly splitting into 6 can be done by splitting each edge into 3, and then doing it again but splitting into 2 this time.

If n is the number you want to split each edge into, then splitting by each of its prime factors is a more easy way to the same result.

![http://www.davidsev.co.uk/stuff/sphere/edge6.png](http://www.davidsev.co.uk/stuff/sphere/edge6.png)

It is this mixing that allows us to greatly increase the number of spheres that can be generated.

If you assume that any sphere between 1,000 and 20,000 faces is a valid map, then there are 21 sizes available, of which 15 are under 10k faces.

# Code #
```
#include <stdio.h>
#include <stdlib.h>

//#define MAX_SUBDIV 20  // Only need check primes.
#define MIN_FACES 1000
#define MAX_FACES 20000
#define MAX_DEPTH 50

unsigned short posible_subdivs[] = { 2,3,5,7,9,13,17,19 };
unsigned int subdivs[MAX_DEPTH];
unsigned short int depth = 0;

void loop (const unsigned int currentFaces) 
{
	unsigned int i;
	for (i=0; i<(sizeof(posible_subdivs)/sizeof(posible_subdivs[0])); i++)
	{
		unsigned short int div = posible_subdivs[i];
		
		unsigned int c = currentFaces*div*div;
		if (c > MAX_FACES)
			break;

		subdivs[depth++] = div;

		if(c > MIN_FACES)
		{
			printf("%-5i (", c);
			unsigned int x;
			for(x=0; x<MAX_DEPTH && subdivs[x]; x++)
				printf("%s%i", (x ? ", " : ""), subdivs[x]);	
			printf(")\n");
		}
//		else
//			printf("%-5i\n", c);

		loop(c);
		subdivs[depth--] = 0;
	}
}

int main()
{
	loop(20);
}
```