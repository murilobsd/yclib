/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_xml.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-11 14:54:59
 * -------------------------
**************************************************************************/

#ifndef _RNS_XML_H_
#define _RNS_XML_H_

#include "rns.h"
#include "expat.h"

typedef struct rns_xml_attr_s
{
    rbt_node_t node;
    uchar_t* attr;
}rns_xml_attr_t;

typedef struct rns_xml_node_s
{
    rbt_node_t node;
    rbt_root_t attr;
    rbt_root_t children;
    uchar_t* value;
    uint32_t len;
    uint32_t size;
    struct rns_xml_node_s* parent;
}rns_xml_node_t;

typedef struct rns_xml_s
{
    rns_xml_node_t root;
    rns_xml_node_t* curr;
    XML_Parser parser;
    uint32_t depth;
}rns_xml_t;

rns_xml_t* rns_xml_create(uchar_t* buf, uint32_t size);
uchar_t* rns_xml_node_value(rns_xml_t* xml, ...);
rns_xml_node_t* rns_xml_node(rns_xml_t* xml, ...);
rns_xml_node_t* rns_xml_node_next(rns_xml_node_t* xml_node);
uchar_t* rns_xml_subnode_value(rns_xml_node_t* xml_node, ...);
uchar_t* rns_xml_node_attr(rns_xml_t* xml_node, uchar_t* attr, ...);
uchar_t* rns_xml_subnode_attr(rns_xml_node_t* xml_node, uchar_t* attr);
void rns_xml_destroy(rns_xml_t** xml);

#endif

