// remaplut.c : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "getopt.h"


uint8_t	output_width = 8;
char	*input_file = NULL;
char	*output_file = NULL;
char	*lut_name = NULL;

char const * const pos_lut = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/-";
uint8_t	remap[64];


// print number as binary
void fprint_bin(FILE *fout, uint64_t v, uint8_t bits)
{
	for (int i = bits - 1; i >= 0; i--)
		fprintf(fout, "%c", v & (1ULL << i) ? '1' : '0');
}

// print remap settings
void print_remapping(FILE *fout)
{
	fprintf(fout, "//");
	for (int i = output_width - 1; i >= 0; i--)
	{
		if (remap[i] == 0xFF)
			printf("-");
		else
			fprintf(fout, "%c", pos_lut[remap[i]]);
	}
	fprintf(fout, "\n");
}

// scan a string for remapping data
bool load_remapping(char *map, FILE *fout)
{
	int len = strlen(map);
	if (len < 1)
	{
		fprintf(stderr, "Remapping string contains too few characters.\n");
		return false;
	}
	if (len > 64)
	{
		fprintf(stderr, "Remapping string contains >64 characters.\n");
		return false;
	}

	for (int i = 0; i < len-1; i++)
	{
		int pos = -1;
		for (int j = 0; j < 65; j++)
		{
			if (pos_lut[j] == map[i])
			{
				pos = j;
				break;
			}
		}
		if (pos == -1)
		{
			fprintf(stderr, "Invalid map character '%c'\n", map[i]);
			return false;
		}

		if (map[i] == '-')
			remap[len-i-2] = 0xFF;
		else
			remap[len-i-2] = pos;
	}
	
	print_remapping(fout);
	return true;
}

// parse command line
bool parse_cli(int argc, char* argv[])
{
	int c;

	if (argc == 1)
	{
		printf("Usage: remaplut -i <input file> [-o <output file] [-w <bit width>] [-r <remap string>] [-n <lut name>]\n");
		return false;
	}

	while ((c = getopt(argc, argv, "i:o:w:r:n:")) != -1)
	{
		switch (c)
		{
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'n':
			lut_name = optarg;
			break;
		case 'w':
		{
			long b = strtol(optarg, NULL, 10);
			switch (b)
			{
			case 8:
				output_width = 8;
				break;
			case 16:
				output_width = 16;
				break;
			case 32:
				output_width = 32;
				break;
			case 64:
				output_width = 64;
				break;
			default:
				fprintf(stderr, "Invalid output width (-w parameter): 8/16/32/64\n");
				return false;
			}
			break;
		}
		case 'r':
			if (!load_remapping(optarg, stdout))	// prints its own error messages
				return false;
			break;
		default:
			fprintf(stderr, "%s: option '-%c' is invalid.\n", argv[0], optopt);
			return false;
		}
	}
	
	return true;
}

// remap bits according to current map
uint64_t remap64(uint64_t v)
{
	uint64_t o = 0;
	for (int i = 0; i < 64; i++)
	{
		if (remap[i] != 0xFF)
			o |= (v & (1ULL << remap[i]) ? 1ULL : 0ULL) << i;
	}
	return o;
}

// parse input file, produce output
bool parse_input(FILE *fin, FILE *fout)
{
	unsigned int line_count = 1;

	while (!feof(fin))
	{
		char line[256];
		if (fgets(line, 256, fin) != NULL)
		{
			if ((line[0] == '#') && (line[1] == '#'))	// remapping data
			{
				if (!load_remapping(&line[2], fout))
				{
					fprintf(stderr, "Bad remapping data on line %u.\n", line_count);
					return false;
				}
			}

			if ((line[0] == '0') && (line[1] == 'b'))	// input bitfield
			{
				uint64_t v = 0;
				int i = 2;
				for (;;)
				{
					if (line[i] == '1')
					{
						v <<= 1;
						v |= 1;
					}
					else if (line[i] == '0')
						v <<= 1;
					else
						break;
					i++;
				}
				//fprint_bin(stdout, v, 64);
				//fprintf(stdout, "\r\n");
				uint64_t o = remap64(v);
				fprintf(fout, "0b");
				fprint_bin(fout, o, output_width);
				fprintf(fout, ",\n");
			}
		}
		line_count++;
	}

	return true;
}

int main(int argc, char* argv[])
{
	memset(remap, 0xFF, sizeof(remap));	// default to no bits in every position

	// parse command line
	if (!parse_cli(argc, argv))
		return 1;

	FILE *fin = fopen(input_file, "r");
	if (fin == NULL)
	{
		fprintf(stderr, "Unable to open %s.\n", input_file);
		return 1;
	}

	FILE *fout;
	if (output_file == NULL)
		fout = stdout;
	else
	{
		fout = fopen(output_file, "w");
		if (fout == NULL)
		{
			fprintf(stderr, "Unable to open %s.\n", output_file);
			return 1;
		}
	}

	// file header
	if (fout != stdout)
	{
		fprintf(fout, "/* %s\n", output_file);
		fprintf(fout, " *\n * Generated from %s\n", input_file);
		fprintf(fout, "*\n\n");
		if (lut_name == NULL)
			lut_name = "remapped_lut";
		fprintf(fout, "const uint%u_t %s[] = {\n", output_width, lut_name);
	}

	// remap
	if (!parse_input(fin, fout))
		return 1;

	// file footer
	if (fout != stdout)
	{
		fprintf(fout, "};\n");
	}

	return 0;
}
