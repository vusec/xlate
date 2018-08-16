#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <xlate/string.h>

int stricmp(const char *lhs, const char *rhs)
{
	while (tolower(*lhs) == tolower(*rhs)) {
		if (*lhs == '\0')
			break;

		++lhs;
		++rhs;
	}

	return (int)tolower(*lhs) - (int)tolower(*rhs);
}
