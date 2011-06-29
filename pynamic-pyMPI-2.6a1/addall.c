/*
 * Please see COPYRIGHT information at the end of this file
 * File: addall.c
 * Author: Dong H. Ahn and Greg Lee
 *
 * Updates:
 *      4/12/07 print human readable sizes (i.e. KB, MB, GB)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

int main(int argc, char* argv[])
{
	static long int number = 0;
        ssize_t read;
	char* line = NULL;
	size_t len;
	int base;

	if (argc < 3) 
        {
		printf("Usage: addall section_name base [-h]\n");
                printf("       base must be either 10 or 16\n");
                printf("       -h specifies human readable format\n");
	}

	base = atoi(argv[2]);
	assert ( base == 10 || base == 16 );

	while ( (read = getline(&line, &len, stdin)) != -1) 
 	{
		line[read-1] = '\0';
		number+=strtol(line, NULL, base);
	        line = NULL;
	}
	if (argc == 4)
	{
		if (number >= 1000000000)
			printf("Size of %s: %.1fGB\n", argv[1], number/1000000000.0);
		else if (number >= 1000000)
			printf("Size of %s: %.1fMB\n", argv[1], number/1000000.0);
		else if (number >= 1000)
			printf("Size of %s: %.1fKB\n", argv[1], number/1000.0);
		else
			printf("Size of %s: %dB\n", argv[1], number);
	}	
	else	
		printf("Size of %s: %d\n", argv[1], number);

	return EXIT_SUCCESS;
}

/*************************************************
COPYRIGHT

Copyright (c) 2007, The Regents of the University of California. 
Produced at the Lawrence Livermore National Laboratory 
Written by Gregory Lee, Dong Ahn, John Gyllenhaal, Bronis de Supinski. 
UCRL-CODE-228991. 
All rights reserved. 
 
This file is part of Pynamic.   For details contact Greg Lee (lee218@llnl.gov).  Please also read the "ADDITIONAL BSD NOTICE" in pynamic.LICENSE. 
 
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 
 
* Redistributions of source code must retain the above copyright notice, this list of conditions and the disclaimer below.  
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the disclaimer (as noted below) in the documentation and/or other materials provided with the distribution.  
* Neither the name of the UC/LLNL nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OF THE UNIVERSITY OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*************************************************/
