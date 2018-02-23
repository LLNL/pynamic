/* Allow ten differently named large functions to be created and put into a function pointer array. -JCG LLNL 1 March 2010
*/

#ifdef BUILD_PYNAMIC_BIGEXE

 #include <stdio.h>

#ifndef INDEX
#define INDEX 1
#endif

#define xstr(s) str(s)
#define str(s) #s
#define fn(ID,ID2) func ## ID ## ID2
#define fn2(ID,ID2) fn (ID, ID2)
#define funcname(ID) fn2 ( INDEX , ID)
#define N0 funcname(0)
#define N1 funcname(1)
#define N2 funcname(2)
#define N3 funcname(3)
#define N4 funcname(4)
#define N5 funcname(5)
#define N6 funcname(6)
#define N7 funcname(7)
#define N8 funcname(8)
#define N9 funcname(9)

#define an(NAME) NAME ## _Array
#define an2(NAME) an(NAME)
#define array_name an2(N0)

#define a   i=0; i=0; i=0; i=0; i=0; i=0; i=0; i=0; i=0; i=0;
#define b   a  a a a a a a a a a
#define c   b  b b b b b b b b b
#define d   c c c c c c c c c c
#define e   d d d d d d d d d d
#define big_body   e e e

typedef int (*funcPtr)();

extern funcPtr array_name [];

int N0 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N0) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N1 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N1) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N2 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N2) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N3 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N3) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N4 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N4) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N5 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N5) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N6 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N6) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N7 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N7) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N8 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N8) );
 if (i == 0) return 0;
 big_body
 return 0;
}

int N9 ()
{
 volatile int i = 0;
 printf ("In %s\n", xstr(N9) );
 if (i == 0) return 0;
 big_body
 return 0;
}


funcPtr array_name [] = {N0, N1, N2, N3, N4, N5, N6, N7, N8, N9};

#define cname(MID) check ## MID
#define cname2(MID) cname(MID)
#define checkname cname2(INDEX)


void checkname ()
{
	printf ("In %s\n", xstr(checkname));

        {
		funcPtr fptr = N0;
		printf ("%s: d %p i %p\n", xstr(N0), N0, array_name[0]);
                if (fptr != N0)
		   printf ("***Mismatch for %s\n", xstr(N0));
                array_name[0]();
	}

        {
		funcPtr fptr = N1;
		printf ("%s: d %p i %p\n", xstr(N1), N1, array_name[1]);
                if (fptr != N1)
		   printf ("***Mismatch for %s\n", xstr(N1));
                array_name[1]();
	}

        {
		funcPtr fptr = N2;
		printf ("%s: d %p i %p\n", xstr(N2), N2, array_name[2]);
                if (fptr != N2)
		   printf ("***Mismatch for %s\n", xstr(N2));
                array_name[2]();
	}

        {
		funcPtr fptr = N3;
		printf ("%s: d %p i %p\n", xstr(N3), N3, array_name[3]);
                if (fptr != N3)
		   printf ("***Mismatch for %s\n", xstr(N3));
                array_name[3]();
	}


        {
		funcPtr fptr = N4;
		printf ("%s: d %p i %p\n", xstr(N4), N4, array_name[4]);
                if (fptr != N4)
		   printf ("***Mismatch for %s\n", xstr(N4));
                array_name[4]();
	}

        {
		funcPtr fptr = N5;
		printf ("%s: d %p i %p\n", xstr(N5), N5, array_name[5]);
                if (fptr != N5)
		   printf ("***Mismatch for %s\n", xstr(N5));
                array_name[5]();
	}

        {
		funcPtr fptr = N6;
		printf ("%s: d %p i %p\n", xstr(N6), N6, array_name[6]);
                if (fptr != N6)
		   printf ("***Mismatch for %s\n", xstr(N6));
                array_name[6]();
	}

        {
		funcPtr fptr = N7;
		printf ("%s: d %p i %p\n", xstr(N7), N7, array_name[7]);
                if (fptr != N7)
		   printf ("***Mismatch for %s\n", xstr(N7));
                array_name[7]();
	}

        {
		funcPtr fptr = N8;
		printf ("%s: d %p i %p\n", xstr(N8), N8, array_name[8]);
                if (fptr != N8)
		   printf ("***Mismatch for %s\n", xstr(N8));
                array_name[8]();
	}

        {
		funcPtr fptr = N9;
		printf ("%s: d %p i %p\n", xstr(N9), N9, array_name[9]);
                if (fptr != N9)
		   printf ("***Mismatch for %s\n", xstr(N9));
                array_name[9]();
	}

}

/*
int main()
{
        checkname();
	return (0);

}
*/

#endif
