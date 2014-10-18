#include "xml_function.h"

int main(void)
{
	struct node *root = parse_xml(L"config_file.xml");
    if (root == NULL)
        return -1;
    printf("hello world\n");
	return 0;
}