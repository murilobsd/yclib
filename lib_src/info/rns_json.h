/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_json.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-13 09:59:07
 * -------------------------
**************************************************************************/

#ifndef _RNS_JSON_H_
#define _RNS_JSON_H_

#include "rns.h"
#include "cJSON.h"

typedef cJSON rns_json_t;

rns_json_t* rns_json_read(uchar_t* buf, uint32_t size);
uchar_t* rns_json_write(rns_json_t* json);
void rns_json_destroy(rns_json_t** json);

rns_json_t* rns_json_create_obj();
rns_json_t* rns_json_create_array();

void rns_json_add(rns_json_t* json, uchar_t* key, rns_json_t* item);
int32_t rns_json_add_int(rns_json_t* json, uchar_t* key, int32_t value);
int32_t rns_json_add_str(rns_json_t* json, uchar_t* key, uchar_t* value);

rns_json_t* rns_json_node(rns_json_t* json, ...);
rns_json_t* rns_json_array_node(rns_json_t* json, uint32_t item);
uint32_t rns_json_array_size(rns_json_t* json);

#endif

