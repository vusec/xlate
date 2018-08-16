#include <stdlib.h>
#include <string.h>

#include <libelf.h>
#include <gelf.h>

#include <xlate/string.h>

int gelf_find_sym(GElf_Sym *ret, Elf *elf, const char *key)
{
	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	GElf_Sym sym;
	Elf_Data *data;
	const char *name;
	size_t i, count;

	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		gelf_getshdr(scn, &shdr);

		if (shdr.sh_type != SHT_SYMTAB)
			continue;

		data = elf_getdata(scn, NULL);
		count = shdr.sh_size / shdr.sh_entsize;

		for (i = 0; i < count; ++i) {
			gelf_getsym(data, i, &sym);
			name = elf_strptr(elf, shdr.sh_link, sym.st_name);

			if (stricmp(name, key) != 0)
				continue;

			if (ret)
				*ret = sym;

			return 0;
		}
	}

	return -1;
}

void *gelf_find_sym_ptr(Elf *elf, const char *key)
{
	GElf_Sym sym;

	if (gelf_find_sym(&sym, elf, key) < 0)
		return NULL;

	return (void *)sym.st_value;
}
