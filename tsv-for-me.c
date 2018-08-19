#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION "1.1.0"

struct string {
	char *chars;
	size_t length;
};

void print_help(const char *program_name, FILE *to);
void print_version(const char *program_name, FILE *to);
ssize_t get_columns(FILE *from, struct string **into, size_t *capacity);
void get_widths(const struct string *from, size_t *into, size_t n_columns);
void fit_row(struct string **cells, size_t *capacity, size_t grow_to);
ssize_t get_row(FILE *from, struct string *into, size_t max_columns);
#define MAX(a, b) ( a > b ? a : b )
int print_row(const struct string *row, const size_t *widths, size_t n_columns);
int print_separator(const size_t *widths, size_t n_columns, const char *seg);

int main(int argc, char **argv)
{
	size_t conf_padding = 2;
	bool conf_print_separator = true;
	char *conf_separator = "-";
	char *conf_filename;
	int opt;
	while ((opt = getopt(argc, argv, "p:s:Shv")) != -1) {
		switch (opt) {
		case 'p':
			conf_padding = atoi(optarg);
			break;
		case 's':
			conf_separator = optarg;
			break;
		case 'S':
			conf_print_separator = false;
			break;
		case 'h':
			print_help(argv[0], stdout);
			exit(0);
		case 'v':
			print_version(argv[0], stdout);
			exit(0);
		default:
			print_help(argv[0], stderr);
			exit(1);
		}
	}
	conf_filename = argv[optind];
	FILE *input;
	if (conf_filename) {
		input = fopen(conf_filename, "r");
		if (!input) {
			fprintf(stderr, "%s: Could not open file \"%s\": %s\n",
				argv[0], conf_filename, strerror(errno));
			exit(1);
		}
	} else {
		input = stdin;
	}
	struct string *cells;
	size_t cell_cap;
	ssize_t n_columns = get_columns(input, &cells, &cell_cap);
	if (n_columns < 0) {
		return 1;
	}
	size_t *widths = malloc(n_columns * sizeof(*widths));
	get_widths(cells, widths, n_columns);

	fit_row(&cells, &cell_cap, n_columns * 2);
	struct string empty_string = {"", 0};
	ssize_t rowsize;
	size_t n_cells;
	for (size_t r = n_columns;
	     n_cells = r, (rowsize = get_row(input, &cells[r], n_columns)) >= 0;
	     r += n_columns, fit_row(&cells, &cell_cap, r + n_columns))
	{
		for (size_t i = 0; i < rowsize; ++i) {
			widths[i] = MAX(widths[i], cells[r + i].length);
		}
		while (rowsize < n_columns) {
			cells[r + rowsize] = empty_string;
			++rowsize;
		}
	}

	setvbuf(stdout, NULL, _IOFBF, BUFSIZ);
	for (size_t i = 0; i < n_columns; ++i) {
		widths[i] += conf_padding;
	}
	print_row(cells, widths, n_columns);
	if (conf_print_separator) {
		print_separator(widths, n_columns, conf_separator);
	}
	for (size_t r = n_columns; r < n_cells; r += n_columns) {
		print_row(&cells[r], widths, n_columns);
	}
}

void print_help(const char *program_name, FILE *to)
{
	fprintf(to,
		"Usage: %s [options] [file]\n"
		"\n"
		"  -p <padding>    Sets the minimum number of spaces between\n"
		"                  lines to <padding>. The default is 2.\n"
		"  -s <separator>  Sets the character making up the separator\n"
		"                  line to <separator>. The default is '-'.\n"
		"  -S              Do not print any separator line.\n"
		"  -h              Print this help information and exit.\n"
		"  -v              Print version information and exit.\n"
		"\n"
		"the 'file' parameter indicates which file to read from. If\n"
		"left blank, the program reads from standard in.\n"
		,
		program_name
	);
}

void print_version(const char *program_name, FILE *to)
{
	fprintf(to, "%s "VERSION"\n", program_name);
}

struct item_iter {
	char *line;
	struct string *into;
	size_t length;
	size_t idx;
	size_t base;
	size_t line_idx;
};

#define ITEM_ITER(line_, into_) {					\
	.length = 0,							\
	.idx = 0,							\
	.base = 0,							\
	.line_idx = 0,							\
	.line = (line_),						\
	.into = (into_)							\
}

void item_iter_next(struct item_iter *iter);

ssize_t get_columns(FILE *from, struct string **into, size_t *capacity)
{
	char *line = NULL;
	size_t line_capacity = 0;
	ssize_t line_length = getline(&line, &line_capacity, from);
	if (line_length < 0) {
		free(line);
		return -1;
	}
	*capacity = line_capacity * sizeof(**into);
	*into = malloc(*capacity);
	struct item_iter item = ITEM_ITER(line, *into);
	while (item.line_idx < line_length) {
		item_iter_next(&item);
	}
	return item.idx;
}

void get_widths(const struct string *from, size_t *into, size_t n_columns)
{
	while (n_columns--) {
		into[n_columns] = from[n_columns].length;
	}
}

void fit_row(struct string **cells, size_t *capacity, size_t grow_to)
{
	if (grow_to * sizeof(**cells) <= *capacity) {
		return;
	}
	*capacity *= 2;
	struct string *new_cells = realloc(*cells, *capacity);
	*cells = new_cells;
}

ssize_t get_row(FILE *from, struct string *into, size_t max_columns)
{
	char *line = NULL;
	size_t line_capacity = 0;
	ssize_t line_length = getline(&line, &line_capacity, from);
	if (line_length < 0) {
		free(line);
		return -1;
	}
	struct item_iter item = ITEM_ITER(line, into);
	while (item.line_idx < line_length && item.idx < max_columns) {
		item_iter_next(&item);
	}
	return item.idx;
}

size_t get_char_size(char ch);

void item_iter_next(struct item_iter *iter)
{
	char ch = iter->line[iter->line_idx];
	if (ch == '\t' || ch == '\n') {
		iter->into[iter->idx].chars = &iter->line[iter->base];
		iter->into[iter->idx].length = iter->length;
		iter->length = 0;
		iter->base = iter->line_idx + 1;
		++iter->idx;
		iter->line[iter->line_idx] = '\0';
	} else if (!(ch & 0x80)) {
		++iter->length;
	} else {
		iter->line_idx += get_char_size(ch);
		++iter->length;
		return;
	}
	++iter->line_idx;
}

size_t get_char_size(char ch)
{
	unsigned ch_int = (unsigned)ch << ((sizeof(unsigned) - 1) * CHAR_BIT);
	return __builtin_clz(~ch_int);
}

int print_row(const struct string *row, const size_t *widths, size_t n_columns)
{
	for (size_t i = 0; i < n_columns; ++i) {
		printf("%s%*s",
			row[i].chars, (int)(widths[i] - row[i].length), "");
	}
	printf("\n");
	return 0;
}

int print_separator(const size_t *widths, size_t n_columns, const char *segment)
{
	size_t total_width = 0;
	for (size_t i = 0; i < n_columns; ++i) {
		total_width += widths[i];
	}
	size_t seg_length = strlen(segment);
	char *buf;
	if (seg_length <= 1) {
		buf = malloc(total_width + 1);
		memset(buf, segment[0], total_width);
	} else if (total_width > 0) {
		total_width *= seg_length;
		buf = malloc(total_width + 1);
		memcpy(buf, segment, seg_length);
		size_t write_head = seg_length;
		while (write_head * 2 < total_width) {
			memcpy(buf + write_head, buf, write_head);
			write_head *= 2;
		}
		memcpy(buf + write_head, buf, total_width - write_head);
	} else {
		return 0;
	}
	buf[total_width] = '\0';
	printf("%s\n", buf);
	free(buf);
	return 0;
}
