#include "xml_function.h"

#define XML_SPACE_STR	L"\r\n \t"

struct attr
{
	wchar_t *attr_name;        //属性名
	wchar_t *attr_value;       //属性值
};

struct node
{
	wchar_t *node_name;
	wchar_t *node_value;
	struct array *attr_list;
	struct node *father_node;
	struct node *pre_node;
	struct node *next_node;
	struct node *child_node;
	int is_closed;
	enum xml_type type;
};

enum fsm_state {
	XML_STATE_OPEN,
	XML_STATE_NAME,
	XML_STATE_ATTR,
	XML_STATE_VALUE,
	XML_STATE_CLOSE,
	XML_STATE_END,
	XML_STATE_NEXT,
    XML_STATE_NOTE,
};

enum xml_type{
	XML_ROOT,		//根节点
	XML_NOTE,		//注释
	XML_ELEMENT,	//元素
};

struct fsm
{
	struct node *root;
	struct node *cur_node;
	struct node *tem_node;
	const wchar_t *file_point;
	const wchar_t *file_end;
	enum fsm_state cur_fsm_state;
	enum fsm_state last_fsm_state;
    int error;
};


static struct node *new_node(enum xml_type type)
{
    struct node *tem = (struct node *)malloc(sizeof(struct node));
    if(tem == NULL)
        return NULL;

    memset(tem, 0, sizeof(struct node));
	tem->type = type;
	return tem;
}
static void xml_free_node(struct node *node)
{
    assert(node);
    if (node->attr_list)
        array_destroy(node->attr_list);

    if (node->node_name)
    {
        free((_tchar*)node->node_name);
        node->node_name = NULL;
    }

    if (node->node_value)
    {
        free((_tchar*)node->node_value);
        node->node_value = NULL;
    }

    free(node);
    node = NULL;
}

static void xml_free(struct node *node)
{
    struct node *tem;

    assert(node);

    while(node)
    {
        tem = node;
        if(tem->child_node)
            xml_free(tem->child_node);
        node = node->pre_node;
        xml_free_node(tem);
    }
}
static void xml_free_root(struct node *tree)
{
    assert(tree);

    if(tree->child_node)
        xml_free(tree->child_node);

    xml_free_node(tree);
}

static int add_brother(struct node **dst,struct node *src)
{
    struct node *tem = *dst;

    assert(src);
    if(*dst == NULL)
    {
        *dst = src;
        return 0;
    }

    tem->next_node = src;
    src->pre_node = tem;
    src->father_node = tem->father_node;
    tem->father_node->child_node = src;
    return 0;
}

static int add_child(struct node **dst,struct node *src)
{
    struct node *tem = *dst;

    assert(src);
    if(*dst == NULL)
    {
        *dst = src;
        return 0;
    }

    if (tem->child_node == NULL)
    {
        src->father_node = tem;
        tem->child_node = src;
    }
    else
        add_brother(&tem->child_node,src);

    return 0;
}
static int add_node(struct fsm *fsm_point)
{
    int length = 0;
    _tchar *s = NULL;
    struct node *tem = NULL;
    const wchar_t *tem_file_point = fsm_point->file_point;

    assert(fsm_point);

    tem = fsm_point->tem_node;
    if(tem == NULL)             //handle </chip>
    {
        length = wcscspn(tem_file_point, L"<");
        tem_file_point += length;
        if(*(tem_file_point + 1) != '/' || tem_file_point >= fsm_point->file_end)
        {
            fsm_point->cur_fsm_state = XML_STATE_END;
            fsm_point->error = 1;
            return -1;
        }
        tem_file_point += 2;
        length = wcscspn(tem_file_point, L">");
        s = (wchar_t *)alloca(sizeof(wchar_t)*length);
        if(s == NULL)
        {
            fsm_point->cur_fsm_state = XML_STATE_END;
            fsm_point->error = 1;
			return -1;
        }

        _tcsncpy(s, tem_file_point, length);
        if (_tcsncmp(s, fsm_point->cur_node->node_name,length) != 0)
        {
            fsm_point->cur_fsm_state = XML_STATE_END;
            fsm_point->error = 1;
            return -1;
        }
        fsm_point->cur_node->is_closed = 1;
        if(fsm_point->cur_node->father_node)
            fsm_point->cur_node = fsm_point->cur_node->father_node;

        tem_file_point += length + 1;
        fsm_point->file_point = tem_file_point;
        return 0;
    }

    if((fsm_point->cur_node == NULL) || (fsm_point->root == NULL))
    {
        fsm_point->cur_node = fsm_point->root = tem;
        return 0;
    }

    if (fsm_point->cur_node->type == XML_ROOT)
        add_child(&fsm_point->cur_node, tem);
    else if (fsm_point->cur_node->type == XML_NOTE)
        add_brother(&fsm_point->cur_node, tem);
    else if(fsm_point->cur_node->is_closed)
        add_brother(&fsm_point->cur_node,tem);
    else
        add_child(&fsm_point->cur_node,tem);

    fsm_point->cur_node = fsm_point->tem_node;

	if (fsm_point->cur_node->is_closed && fsm_point->cur_node->father_node)
		fsm_point->cur_node = fsm_point->cur_node->father_node;

	fsm_point->tem_node = NULL;

	return 0;
}
static int close_node(struct fsm *fsm_point)
{
    int length = 0;

    assert(fsm_point);

    if (fsm_point->tem_node)
    {
        length = wcscspn(fsm_point->file_point, L">"XML_SPACE_STR);
        if (*(fsm_point->file_point + length - 1) == '/' || *(fsm_point->file_point +                                     length - 1) == '-')
            fsm_point->tem_node->is_closed = 1;
        fsm_point->file_point += length+1;
    }
	return 0;
}

static int fsm_open(struct fsm *fsm_point)
{
    int space_len = 0;
	const wchar_t *tem_file_point = fsm_point->file_point;
	const _tchar *s = XML_SPACE_STR;

	assert(fsm_point);

	tem_file_point = skip_space(tem_file_point, fsm_point->file_end, s);

	if (*tem_file_point == '<')
		tem_file_point++;
	
	if (*tem_file_point == '?')
	{
		tem_file_point++;
		fsm_point->tem_node = new_node(XML_ROOT);
	}
	else if ((*tem_file_point == '!') && (*(tem_file_point + 1) == '-') && (*(tem_file_point + 2) == '-'))
	{
		tem_file_point += 3;
		fsm_point->tem_node = new_node(XML_NOTE);
	}
    else if(*(tem_file_point+1) == '/0')
        return 0;
	else
		fsm_point->tem_node = new_node(XML_ELEMENT);

	if (fsm_point->tem_node == NULL)
	{
		fsm_point->cur_fsm_state = XML_STATE_END;
        fsm_point->error = 1;
		return -1;
	}

	fsm_point->file_point = tem_file_point;
	fsm_point->cur_fsm_state = XML_STATE_NEXT;
	return 0;
}

static int fsm_name(struct fsm *fsm_point)
{
	int name_size = 0;
	_tchar *tem = NULL;
	const _tchar *s = L">"XML_SPACE_STR;
	const _tchar *tem_file_point = fsm_point->file_point;

    assert(fsm_point);

    name_size = _tcscspn(tem_file_point, XML_SPACE_STR);
    tem = (_tchar *)malloc(sizeof(_tchar)*(name_size+1));
    if(tem == NULL)
        goto fsm_name_end;

	memcpy(tem, tem_file_point, sizeof(_tchar)*name_size);
    tem[name_size] = 0;
    fsm_point->tem_node->node_name = tem;

    if (*(tem_file_point + name_size + 1) == '/' || *(tem_file_point + name_size + 1) == '?')
    {
        fsm_point->cur_fsm_state = XML_STATE_OPEN;
        close_node(fsm_point);
        add_node(fsm_point);
    }
    else
	    fsm_point->cur_fsm_state = XML_STATE_NEXT;

	tem_file_point += name_size;
	fsm_point->file_point = tem_file_point;
	return 0;

fsm_name_end:
    fsm_point->cur_fsm_state = XML_STATE_END;
    fsm_point->error = 1;
	return -1;
}

static int fsm_note(struct fsm *fsm_point)
{
    _tchar *tem = NULL;
    const wchar_t *tem_file_point = fsm_point->file_point;
    int length = 0;

    assert(fsm_point);

    length = wcscspn(fsm_point->file_point, L"--");
    tem = (_tchar *)malloc(sizeof(_tchar)*(length+1));
    if (tem == NULL)
        goto fsm_note_end;

    memcpy(tem, tem_file_point, sizeof(_tchar)*length);
    tem[length] = 0;
    fsm_point->tem_node->node_name = tem;
    fsm_point->cur_fsm_state = XML_STATE_NEXT;
    tem_file_point += length+2;
    fsm_point->file_point = tem_file_point;
    return 0;
fsm_note_end:
    fsm_point->cur_fsm_state = XML_STATE_END;
    fsm_point->error = 1;
    return -1;
}

static int fsm_attr(struct fsm *fsm_point)
{
	int attr_cnt = 0;
	const _tchar *tem_file_point = fsm_point->file_point;
    struct attr tem_attr;
	int length = 0;

    assert(fsm_point);

    fsm_point->tem_node->attr_list = array_create(sizeof(struct attr));
	if (fsm_point->tem_node->attr_list == NULL)
        goto fsm_attr_end;

	while ((tem_file_point < fsm_point->file_end) && (*tem_file_point != L'>') && (*                          tem_file_point != L'/0'))
	{
		tem_file_point = skip_space(tem_file_point, fsm_point->file_end, XML_SPACE_STR);
		length = wcscspn(tem_file_point, L"="XML_SPACE_STR);
		tem_attr.attr_name = (_tchar *)malloc(sizeof(_tchar)*(length+1));
		if (tem_attr.attr_name == NULL)
			goto fsm_attr_end;
        memcpy(tem_attr.attr_name, tem_file_point, sizeof(_tchar)*length);
        tem_attr.attr_name[length] = 0;

		tem_file_point += length;
		length = wcscspn(tem_file_point, L"\"");
		tem_file_point += length+1;

		length = wcscspn(tem_file_point, L"\"");
		tem_attr.attr_value = (_tchar *)malloc(sizeof(_tchar)*(length+1));
		if (tem_attr.attr_value == NULL)
			goto fsm_attr_end;
		memcpy(tem_attr.attr_value, tem_file_point, sizeof(_tchar)*length);
        tem_attr.attr_value[length] = 0;

		array_add(fsm_point->tem_node->attr_list, &tem_attr);
		attr_cnt++;
		tem_file_point += length+1;	
        tem_file_point = skip_space(tem_file_point, fsm_point->file_end, XML_SPACE_STR);
        if ((*tem_file_point == L'?') && (*(tem_file_point + 1) == L'>'))
            break;
	}
    fsm_point->file_point = tem_file_point;
	if (attr_cnt == 0)
        goto fsm_attr_end;

    fsm_point->cur_fsm_state = XML_STATE_NEXT;
    return 0;
fsm_attr_end:
    if(tem_attr.attr_name)
        free(tem_attr.attr_name);
    if(tem_attr.attr_value)
        free(tem_attr.attr_value);
    fsm_point->cur_fsm_state = XML_STATE_END;
    fsm_point->error = 1;
    return -1;
}

static int fsm_value(struct fsm *fsm_point)
{
    const _tchar *tem_file_point;
	const _tchar *s = L"<"XML_SPACE_STR;
    int len = 0;
    _tchar *tem_value;

	assert(fsm_point);

    tem_file_point = fsm_point->file_point;
    len = wcscspn(tem_file_point,XML_SPACE_STR);
    tem_file_point += len;
	if (tem_file_point >= fsm_point->file_end)
        goto fsm_value_end;

    if(*tem_file_point == '<')
    {
        add_node(fsm_point);
        fsm_point->cur_fsm_state = XML_STATE_OPEN;
        return 0;
    }
    len = 0;
    while((*tem_file_point != '>') && (*tem_file_point != ' '))
    {
        len++;
        tem_file_point++;
    }

    tem_value = (_tchar *)malloc(sizeof(_tchar)*len);
    if(tem_value == NULL)
        goto fsm_value_end;

    memcpy(tem_value,fsm_point->file_point,len);
	fsm_point->tem_node->node_value = tem_value;
    fsm_point->file_point += len;
    fsm_point->cur_fsm_state = XML_STATE_NEXT;
    return 0;
fsm_value_end:
    fsm_point->cur_fsm_state = XML_STATE_END;
	return -1;
}

static int fsm_close(struct fsm *fsm_point)
{
    assert(fsm_point);

    close_node(fsm_point);
    add_node(fsm_point);
    fsm_point->cur_fsm_state = XML_STATE_NEXT;
	return 0;
}

static int fsm_end(struct fsm *fsm_point)
{
	assert(fsm_point);

    if(fsm_point->error)
        return -1;

    fsm_point->tem_node = fsm_point->cur_node;
    if(fsm_point->root)
        xml_free_root(fsm_point->root);

	return 0;
}

static int fsm_next(struct fsm *fsm_point)
{
    int len = 0;
    const _tchar *s = XML_SPACE_STR;
	const _tchar *tem_file_point = fsm_point->file_point;

    assert(fsm_point);
    
    if(tem_file_point >= fsm_point->file_end)
    {
        fsm_point->cur_fsm_state = XML_STATE_END;
        return 0;
    }
	tem_file_point = skip_space(tem_file_point, fsm_point->file_end, XML_SPACE_STR);
	fsm_point->file_point = tem_file_point;

    switch(fsm_point->last_fsm_state)
    {
    case XML_STATE_OPEN:
        if (fsm_point->tem_node->type == XML_NOTE)
            fsm_point->cur_fsm_state = XML_STATE_NOTE;
        else
            fsm_point->cur_fsm_state = XML_STATE_NAME;
        break;
    case XML_STATE_NAME:
        len = wcscspn(fsm_point->file_point,s);
        if(len != 0)
            fsm_point->cur_fsm_state = XML_STATE_ATTR;
        else if((fsm_point->file_end - fsm_point->file_point == 1) && (*fsm_point->file_point == '>'))
        {
            fsm_point->file_point++;
            fsm_point->cur_fsm_state = XML_STATE_VALUE;
        }
        else
            fsm_point->cur_fsm_state = XML_STATE_END;
        break;
    case XML_STATE_ATTR:
        if(fsm_point->file_point >= fsm_point->file_end)
            fsm_point->cur_fsm_state = XML_STATE_END;
        else if(*fsm_point->file_point == '>')
            fsm_point->cur_fsm_state = XML_STATE_CLOSE;
        else if((fsm_point->file_end - fsm_point->file_point > 2) && ((*fsm_point->file_point == '/')
                    || (*fsm_point->file_point == '?')) && (*(fsm_point->file_point + 1) == '>'))
            fsm_point->cur_fsm_state = XML_STATE_CLOSE;
        else
            fsm_point->cur_fsm_state = XML_STATE_END;
        break;
    case XML_STATE_VALUE:
        if((*fsm_point->file_point == '/') && (*(fsm_point->file_point+1) == '>'))
            fsm_point->cur_fsm_state = XML_STATE_CLOSE;
        else
            fsm_point->cur_fsm_state = XML_STATE_END;
        break;
    case XML_STATE_CLOSE:
        if (*fsm_point->file_point == '<' && *(fsm_point->file_point + 1) == '/')
            fsm_point->cur_fsm_state = XML_STATE_CLOSE;
        else if(fsm_point->file_end > fsm_point->file_point)
            fsm_point->cur_fsm_state = XML_STATE_OPEN;
        else
            fsm_point->cur_fsm_state = XML_STATE_END;
        break;
    case XML_STATE_NOTE:
        fsm_point->cur_fsm_state = XML_STATE_CLOSE;
    default:
        break;

    }
	return 0;
}

typedef int (fsm_func_t)(struct fsm *fsm_point);
fsm_func_t *fsm_func_tbl[] =
{
	fsm_open,
	fsm_name,
	fsm_attr,
	fsm_value,
	fsm_close,
	fsm_end,
	fsm_next,
    fsm_note,
};

static struct node *get_root_node(const _tchar *buffer, int buffer_size)
{
	enum fsm_state tem_state;
	struct fsm fsm_state_content;

    assert(buffer);
    assert(buffer_size);

	if (*buffer == 0xfeff)
		buffer += 2;

    memset(&fsm_state_content, 0, sizeof(struct fsm));
	fsm_state_content.file_point = buffer;
	fsm_state_content.file_end = buffer + buffer_size/2 - 2;

	fsm_state_content.cur_fsm_state = fsm_state_content.last_fsm_state = XML_STATE_OPEN;
	tem_state = XML_STATE_OPEN;

	while (tem_state != XML_STATE_END)
	{
		tem_state = fsm_state_content.cur_fsm_state;
		fsm_func_tbl[tem_state](&fsm_state_content);
		fsm_state_content.last_fsm_state = tem_state;
	}

	return fsm_state_content.root;
}

struct node *parse_xml(const _tchar *file_name)
{
	FILE *file = NULL;
	_tchar *buffer = NULL;
	struct _stat file_attr;
	node *root = NULL;

    assert(file_name);

	file = _tfopen(file_name, L"rb");
	if (file == NULL)
		goto parse_xml_end;

	if (_tstat(file_name, &file_attr) != 0)
		goto parse_xml_end;

	buffer = (_tchar *)malloc(file_attr.st_size * 2);
	if (buffer == NULL)
		goto parse_xml_end;

	if (fread(buffer, 1, file_attr.st_size, file) != file_attr.st_size)
		goto parse_xml_end;

	root = get_root_node(buffer, file_attr.st_size);
	return root;

parse_xml_end:
	if (file)
		fclose(file);
	if (buffer)
		free(buffer);

	return NULL;
}

const _tchar *xml_get_name(struct node *my_node)
{
	if (my_node == NULL)
		return NULL;
	return my_node->node_name;
}

const _tchar *xml_get_value(struct node *my_node)
{
	if (my_node == NULL)
		return NULL;
	return my_node->node_value;
}

struct node *add_child_node(struct node *parent, struct node *child)
{
	struct node *tem;

	if ((parent == NULL) || (child == NULL))
		return NULL;

	if (parent->child_node == NULL)
	{
		parent->child_node = child;
		child->father_node = parent;
		return child;
	}
		

	for (tem = parent->child_node; tem; tem = tem->next_node)
		;
	tem->next_node = child;
	child->pre_node = tem;
	child->father_node = tem->father_node;
	return child;
}

struct node *add_brother_node(struct node *pre, struct node *brother)
{
	struct node *tem;

	if ((pre == NULL) || (brother == NULL))
		return NULL;

	if (pre->next_node == NULL)
	{
		pre->next_node = brother;
		brother->pre_node = pre;
		return brother;
	}

	for (tem = pre; tem->next_node; tem = tem->next_node)
		;
	tem->next_node = brother;
	brother->pre_node = tem;
	brother->father_node = tem->father_node;
	return brother;
}

struct node *search_child(struct node *parent, const wchar_t *name)
{
	struct node *tem;

	assert(parent);
    assert(name);

	for (tem = parent->child_node; tem; tem = tem->next_node)
		if (_tcsncmp(tem->node_name, name,_tstrlen(name)) == 0)
			return tem;

	return NULL;
}

struct node *search_brother(struct node *brother, const wchar_t *name)
{
	struct node *tem;

	assert(brother);
    assert(name);

	for (tem = brother; tem; tem = tem->next_node)
		if (_tcscmp(tem->node_name, name) == 0)
			return tem;
	return NULL;
}

