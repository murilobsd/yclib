#include "rns_xml.h"
#include <stdarg.h>
#include <stdio.h>

static  void begin_node(void *data, const XML_Char *name, const XML_Char **attr)
{
    rns_xml_t* xml = (rns_xml_t*)data;

    
    rns_xml_node_t* xml_node = (rns_xml_node_t*)rns_malloc(sizeof(rns_xml_node_t));
    if(xml_node == NULL)
    {
        return;
    }

    rbt_init(&xml_node->attr, RBT_TYPE_STR);
    rbt_init(&xml_node->children, RBT_TYPE_STR);

    rns_xml_node_t* parent = xml->curr;
    rbt_set_key_str(&xml_node->node, (uchar_t*)name);
    rbt_insert(&parent->children, &xml_node->node);

    
    uint32_t i = 0;
    while(attr[i])
    {
        rns_xml_attr_t* xml_attr = (rns_xml_attr_t*)rns_malloc(sizeof(rns_xml_attr_t));
        rbt_set_key_str(&xml_attr->node, (uchar_t*)attr[i]);
        xml_attr->attr = (uchar_t*)rns_malloc(rns_strlen(attr[i + 1]) + 1);
        rns_strncpy(xml_attr->attr, attr[i + 1], rns_strlen(attr[i + 1]));

        rbt_insert(&xml_node->attr, &xml_attr->node);

        i += 2;
    }

    xml_node->parent = parent;
    xml->curr = xml_node;
    
    return;
}

static  void end_node(void *data, const XML_Char *name)
{
    rns_xml_t* xml = (rns_xml_t*)data;
    rns_xml_node_t* curr = xml->curr;

    xml->curr = curr->parent;
}

static void text(void* data, const XML_Char* text, int32_t len)
{
    while(len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\n'))--len;

    if(len <= 0)
    {
        return;
    }

    uchar_t *buf = (uchar_t*)rns_malloc(len + 1);
    rns_strncpy(buf, text, len);
    
    
    rns_xml_t* xml = (rns_xml_t*)data;
    rns_xml_node_t* curr = xml->curr;

    if(curr->value == NULL)
    {
        uint32_t size = len > 1024 ? len : 1024;
        curr->value = (uchar_t*)rns_malloc(size);
        curr->len = 0;
        curr->size = size;
    }
    else
    {
        if(curr->len + len > curr->size)
        {
            rns_free(curr->value);
            curr->value = (uchar_t*)rns_malloc(curr->len + len);
            if(curr->value == NULL)
            {
                return;
            }
            curr->size = curr->len + len;
        }
    }
    
    rns_strncpy(curr->value + curr->len, text, len);
    curr->len += len;

    return;
}

rns_xml_t* rns_xml_create(uchar_t* buf, uint32_t size)
{
    rns_xml_t* xml = (rns_xml_t*)rns_malloc(sizeof(rns_xml_t));
    if(xml == NULL)
    {
        return NULL;
    }

    rbt_init(&xml->root.attr, RBT_TYPE_STR);
    rbt_init(&xml->root.children, RBT_TYPE_STR);
    xml->curr = &xml->root;
    
    xml->parser = XML_ParserCreate( NULL );
    if(xml->parser == NULL)
    {
        rns_free(xml);
        return NULL;
    }

    XML_SetUserData(xml->parser, xml);
    XML_SetElementHandler(xml->parser, begin_node, end_node);
    XML_SetCharacterDataHandler(xml->parser, text);
    int32_t ret =  XML_Parse(xml->parser, (XML_Char*)buf, size, 1);
    if(ret == XML_STATUS_ERROR)
    {
        rns_free(xml);
        return NULL;
    }
    return xml; 
}

rns_xml_node_t* rns_xml_node(rns_xml_t* xml, ...)
{
    va_list list;
    va_start(list, xml);
    
    rns_xml_node_t* xml_node = NULL;
    rbt_root_t* root = &xml->root.children;
    
    uchar_t* name = NULL;
    while((name = va_arg(list, uchar_t*)) != NULL)
    {
        rbt_node_t* node = rbt_search_str(root, name);
        if(node == NULL)
        {
            return NULL;
        }
        xml_node = rbt_entry(node, rns_xml_node_t, node);
        root = &xml_node->children;
        name = NULL;
    }
    va_end(list);
    
    return xml_node;
}

rns_xml_node_t* rns_xml_node_next(rns_xml_node_t* xml_node)
{
    rbt_node_t* node = rbt_next(&xml_node->node);
    if(node == NULL)
    {
        return NULL;
    }
    rns_xml_node_t* xn = rbt_entry(node, rns_xml_node_t, node);

    if(rns_strncmp(xn->node.key.str, xml_node->node.key.str, rns_strlen(xn->node.key.str)))
    {
       return NULL; 
    }

    return xn;
}

uchar_t* rns_xml_node_value(rns_xml_t* xml, ...)
{
    va_list list;
    va_start(list, xml);

    rns_xml_node_t* xml_node = NULL;
    rbt_root_t* root = &xml->root.children;
    
    uchar_t* name = NULL;
    while((name = va_arg(list, uchar_t*)) != NULL)
    {
        rbt_node_t* node = rbt_search_str(root, name);
        if(node == NULL)
        {
            return NULL;
        }
        xml_node = rbt_entry(node, rns_xml_node_t, node);
        root = &xml_node->children;
        name = NULL;
    }
    va_end(list);

    return xml_node->value;
}

uchar_t* rns_xml_subnode_value(rns_xml_node_t* xml_node, ...)
{
    rns_xml_node_t* xn = NULL;
    va_list list;
    va_start(list, xml_node);
    
    rbt_root_t* root = &xml_node->children;
    
    uchar_t* name = NULL;
    while((name = va_arg(list, uchar_t*)) != NULL)
    {
        rbt_node_t* node = rbt_search_str(root, name);
        if(node == NULL)
        {
            return NULL;
        }
        xn = rbt_entry(node, rns_xml_node_t, node);
        root = &xn->children;
        name = NULL;
    }
    va_end(list);

    if(xn == NULL)
    {
        return NULL;
    }
    
    return xn->value;
}

    
uchar_t* rns_xml_node_attr(rns_xml_t* xml, uchar_t* attr, ...)
{
    va_list list;
    va_start(list, attr);
    
    rns_xml_node_t* xml_node = NULL;
    rbt_root_t* root = &xml->root.children;
    
    uchar_t* name = NULL;
    while((name = va_arg(list, uchar_t*)) != NULL)
    {
        rbt_node_t* node = rbt_search_str(root, name);
        if(node == NULL)
        {
            return NULL;
        }
        xml_node = rbt_entry(node, rns_xml_node_t, node);
        root = &xml_node->children;
        name = NULL;
    }
    va_end(list);
    
    rbt_node_t* node = rbt_search_str(&xml_node->attr, attr);
    if(node == NULL)
    {
        return NULL;
    }

    rns_xml_attr_t* xml_attr = rbt_entry(node, rns_xml_attr_t, node);
    
    return xml_attr->attr;
}

uchar_t* rns_xml_subnode_attr(rns_xml_node_t* xml_node, uchar_t* attr)
{
    rbt_node_t* node = rbt_search_str(&xml_node->attr, attr);
    if(node == NULL)
    {
        return NULL;
    }
    
    rns_xml_attr_t* xml_attr = rbt_entry(node, rns_xml_attr_t, node);
    
    return xml_attr->attr;
}

void rns_xml_destroy(rns_xml_t** xml)
{
    if(*xml == NULL)
    {
        return;
    }
    
    rns_free(*xml);
    *xml = NULL;
}
