#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "header.hpp"
#include "serial.hpp"
#include "merge.hpp"

void usage(char **argv) {
	fprintf(stderr, "Usage: %s -o merged.count file.count ...\n", argv[0]);
}

int main(int argc, char **argv) {
	extern int optind;
	extern char *optarg;

	char *outfile = NULL;
	int zoom = 32;

	int i;
	while ((i = getopt(argc, argv, "o:z:")) != -1) {
		switch (i) {
		case 'z':
			zoom = atoi(optarg);
			break;

		case 'o':
			outfile = optarg;
			break;

		default:
			usage(argv);
			exit(EXIT_FAILURE);
		}
	}

	if (optind == argc) {
		usage(argv);
		exit(EXIT_FAILURE);
	}

	int nmerges = argc - optind;
	struct merge merges[nmerges];
	int fds[nmerges];
	unsigned char *maps[nmerges];
	long long to_sort = 0;

	for (i = 0; i < nmerges; i++) {
		fds[i] = open(argv[optind + i], O_RDONLY);
		if (fds[i] < 0) {
			perror(argv[optind + i]);
			exit(EXIT_FAILURE);
		}

		struct stat st;
		if (fstat(fds[i], &st) != 0) {
			perror("stat");
			exit(EXIT_FAILURE);
		}

		maps[i] = (unsigned char *) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fds[i], 0);

		if (st.st_size < HEADER_LEN || memcmp(maps[i], header_text, HEADER_LEN) != 0) {
			fprintf(stderr, "%s:%s: Not a tile-count file\n", argv[0], argv[i + optind]);
			exit(EXIT_FAILURE);
		}

		merges[i].start = HEADER_LEN;
		merges[i].end = st.st_size;
		merges[i].map = maps[i];
		merges[i].next = NULL;

		to_sort += st.st_size - HEADER_LEN;
	}

	FILE *out = fopen(outfile, "wb");
	if (out == NULL) {
		perror(outfile);
		exit(EXIT_FAILURE);
	}

	if (fwrite(header_text, HEADER_LEN, 1, out) != 1) {
		perror("write header");
		exit(EXIT_FAILURE);
	}

	merge(merges, nmerges, out, RECORD_BYTES, to_sort / RECORD_BYTES, zoom);
	fclose(out);

	return 0;
}