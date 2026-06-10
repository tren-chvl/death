#include "death.h"

static const unsigned char ELF_MAGIC[] = {0x7F, 'E', 'L', 'F'};
static const unsigned char PNG_MAGIC[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
static const unsigned char JPG_MAGIC[] = {0xFF, 0xD8, 0xFF};
static const unsigned char GIF87_MAGIC[] = "GIF87a";
static const unsigned char GIF89_MAGIC[] = "GIF89a";
static const unsigned char PDF_MAGIC[] = "%PDF-";
static const unsigned char ZIP_MAGIC[] = {'P', 'K', 0x03, 0x04};

static const magic_rule_t rules[] = {
	{FT_ELF, 0, ELF_MAGIC, 4},
	{FT_PNG, 0, PNG_MAGIC, 8},
	{FT_JPEG, 0, JPG_MAGIC, 3},
	{FT_GIF, 0, GIF87_MAGIC, 6},
	{FT_GIF, 0, GIF89_MAGIC, 6},
	{FT_PDF, 0, PDF_MAGIC, 5},
	{FT_ZIP, 0, ZIP_MAGIC, 4},
};

file_type_t detect_type(const unsigned char *buf, size_t size)
{
	for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++)
	{
		const magic_rule_t *r = &rules[i];

		if (r->offset + r->length > size)
			continue;

		if (memcmp(buf + r->offset, r->magic, r->length) == 0)
			return r->type;
	}

	return FT_UNKNOWN;
}

file_type_t detect_file(const char *path)
{
	unsigned char buf[512];

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return FT_UNKNOWN;

	ssize_t n = read(fd, buf, sizeof(buf));
	close(fd);

	if (n <= 0)
		return FT_UNKNOWN;

	return detect_type(buf, n);
}