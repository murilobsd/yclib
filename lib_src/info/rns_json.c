#include "rns_json.h"
#include <stdarg.h>

rns_json_t* rns_json_read(uchar_t* buf, uint32_t size)
{
    rns_json_t* json = cJSON_Parse((char_t*)buf);
    if(json == NULL)
    {
        return NULL;
    }
    
    return json;
}

uchar_t* rns_json_write(rns_json_t* json)
{
    if(json == NULL)
    {
        return NULL;
    }

    return (uchar_t*)cJSON_PrintUnformatted(json);
}

void rns_json_destroy(rns_json_t** json)
{
    if(*json == NULL)
    {
        return;
    }

    cJSON_Delete(*json);
    *json = NULL;

    return;
}

rns_json_t* rns_json_node(rns_json_t* json, ...)
{
    va_list list;
    va_start(list, json);
    
    rns_json_t* node = NULL;
    rns_json_t* parent = json;
    
    uchar_t* name = NULL;
    while((name = (uchar_t*)va_arg(list, uchar_t*)) != NULL)
    {
        node = cJSON_GetObjectItem(parent, (char_t*)name);
        if(node == NULL)
        {
            return NULL;
        }
        parent = node;
        name = NULL;
    }
    va_end(list);
    
    return node;
}

uint32_t rns_json_array_size(rns_json_t* json)
{
    return cJSON_GetArraySize(json);
}

rns_json_t* rns_json_array_node(rns_json_t* json, uint32_t item)
{
    return cJSON_GetArrayItem(json, item);
}

rns_json_t* rns_json_create_obj()
{
    return cJSON_CreateObject();
}
rns_json_t* rns_json_create_array()
{
    return cJSON_CreateArray();
}

void rns_json_add(rns_json_t* json, uchar_t* key, rns_json_t* item)
{
    if(json->type == cJSON_Object)
    {
        cJSON_AddItemToObject(json, (char_t*)key, item);
    }
    else if(json->type == cJSON_Array)
    {
        cJSON_AddItemToArray(json, item);
    }
    
    return;
}

int32_t rns_json_add_int(rns_json_t* json, uchar_t* key, int32_t value)
{

    rns_json_t* item = cJSON_CreateNumber(value);
    if(item == NULL)
    {
        return -1;
    }

    
    if(json->type == cJSON_Object && key)
    {
        cJSON_AddItemToObject(json, (char_t*)key, item);
    }
    else if(json->type == cJSON_Array)
    {
        cJSON_AddItemToArray(json, item);
    }
    else
    {
        return -2;
    }

    return 0;
}

int32_t rns_json_add_str(rns_json_t* json, uchar_t* key, uchar_t* value)
{
    if(value == NULL)
    {
        return 0;
    }
    rns_json_t* item = cJSON_CreateString((char_t*)value);
    if(item == NULL)
    {
        return -1;
    }
    
    if(json->type == cJSON_Object && key)
    {
        cJSON_AddItemToObject(json, (char_t*)key, item);
    }
    else if(json->type == cJSON_Array)
    {
        cJSON_AddItemToArray(json, item);
    }
    else
    {
        return -2;
    }
    
    return 0;
}
