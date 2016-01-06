#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <dirlist.h>
#include <old_dirlist.h>

const char program[] = "printdirlist";
const char version[] = "0.1";
const char author[] = "Walter Brisken <wbrisken@nrao.edu>";
const char verdate[] = "2015 Dec 25";

int usage(const char *pgm)
{
	fprintf(stderr, "\n%s ver. %s  %s  %s\n\n", program, version, author, verdate);
	fprintf(stderr, "Usage: %s [options] <VSN>\n\n", pgm);
	fprintf(stderr, "Options can include:\n\n");
	fprintf(stderr, "  --help\n");
	fprintf(stderr, "  -h      print this useful help information and quit\n\n");
	fprintf(stderr, ".dir or .dirlist files must be in $MARK5_DIR_PATH\n\n");

	return EXIT_SUCCESS;
}

int printdir(const char *vsn)
{
	DirList D;
	std::stringstream error;
	int v;

	v = mark5LegacyLoad(D, vsn, error);

	if(v < 0)
	{
		std::cerr << error.str() << std::endl;
	}
	else
	{
		D.print(std::cout);
	}

	return 0;
}

int main(int argc, char **argv)
{
	const char *vsn = 0;

	if(argc < 2)
	{
		return usage(argv[0]);
	}

	for(int a = 1; a < argc; ++a)
	{
		if(argv[a][0] == '-')
		{
			if(strcmp(argv[a], "--help") == 0 || strcmp(argv[a], "-h") == 0)
			{
				return usage(argv[0]);
			}
			else
			{
				fprintf(stderr, "\nUnrecongnized option: %s\n\n", argv[a]);

				return EXIT_FAILURE;
			}
		}
		else if(vsn == 0)
		{
			vsn = argv[a];
		}
		else
		{
			fprintf(stderr, "\nUnexpected parameter: %s\n\n", argv[a]);

			return EXIT_FAILURE;
		}
	}

	if(!vsn)
	{
		fprintf(stderr, "\nIncomplete command line.\n\n");

		return EXIT_FAILURE;
	}

	return printdir(vsn);
}
