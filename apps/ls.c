#include <libc.h>
#include <fs/vfs.h>

int ls_main(int argc, char **argv)
{
	char *file_list;
	char *dir;

	if (argc == 0)
		dir = "/";
	else
		dir = argv[1];

	file_list = get_file_list(dir);
	if (!file_list)
	{
		printf("Error retrieving file list\n");
		return 1;
	}

	for (; file_list[0] != '\0' && file_list[1] != '\0'; file_list += strlen(file_list) + 1)
		printf("%s\n", file_list);

	return 0;
}