#include "stbMgr.h"
#include "rns_dbpool.h"
#include "rns_hash.h"
#include "rns_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h> 
#include <sys/time.h>


screen_info_t* scree_info_create(uchar_t* buf, uint32_t size)
{
    screen_info_t *si = (screen_info_t*)rns_malloc(sizeof(screen_info_t));
    if(NULL == si)
    {		
        goto ERR_EXIT;
    }
    rns_json_t* p = rns_json_read(buf, size);
    if(p == NULL)
    {
        goto ERR_EXIT;
    }	
    rns_json_t* node = rns_json_node(p, "SDcontentid", NULL);
    if(node != NULL)
    {
        si->SDcontentid = rns_dup((uchar_t*)node->valuestring);
    }
    node = rns_json_node(p, "HDcontentid", NULL);
    if(node != NULL)
    {
        si->HDcontentid = rns_dup((uchar_t*)node->valuestring);
    }
    
    
    node = rns_json_node(p, "content", NULL);
    if(node != NULL)
    {
        si->content = rns_dup((uchar_t*)node->valuestring);
    }
    node = rns_json_node(p, "name", NULL);
    if(node != NULL)
    {
        si->name = rns_dup((uchar_t*)node->valuestring);
    }
    
    
    node = rns_json_node(p, "media_id", NULL);
    if(node != NULL)
    {
        si->media_id = rns_dup((uchar_t*)node->valuestring);
    }
    
    
    node = rns_json_node(p, "provider_id", NULL);
    if(node != NULL)
    {
        si->provider_id = rns_dup((uchar_t*)node->valuestring);
    }   
    
    node = rns_json_node(p, "metaType", NULL);
    if(node != NULL)
    {
        printf("node type : %d, string ptr : %x, int value:%d\n", node->type, node->valuestring, node->valueint);
        if(node->valuestring == NULL)
        {
            si->metatype = node->valueint;
        }
        else
        {
            si->metatype = rns_atoi(node->valuestring);
        }
    }
    
    node = rns_json_node(p, "serial", NULL);
    if(node != NULL)
    {
        if(node->valuestring == NULL)
        {
            si->serial = node->valueint;
        }
        else
        {
            si->serial = rns_atoi(node->valuestring);
        }
    }
    node = rns_json_node(p, "time", NULL);
    if(node != NULL)
    {
        si->time = node->valueint;
    } 
    node = rns_json_node(p, "status", NULL);
    if(node != NULL)
    {
        si->status = node->valueint;
    }    
    node = rns_json_node(p, "screenCode", NULL);
    if(node != NULL)
    {
        si->screenCode = rns_dup((uchar_t*)node->valuestring);
    }
    node = rns_json_node(p, "keycode", NULL);
    if(node != NULL)
    {
        si->keycode = node->valueint;
    }  
    node = rns_json_node(p, "delay", NULL);
    if(node != NULL)
    {
        si->delay = node->valueint;
    }     
    
    node = rns_json_node(p, "totaltime", NULL);
    if(node != NULL)
    {
        si->totaltime = node->valueint;
    }

    node = rns_json_node(p, "phoneid", NULL);
    if(node != NULL)
    {
        si->phoneid = rns_dup((uchar_t*)node->valuestring);
    }

//fuxaing start
	node = rns_json_node(p,"mobile",NULL);
	if(node  != NULL)
	{
		si->mobile = rns_dup((uchar_t*)node->valuestring);
	}
//fuxiang end
    rns_json_destroy(&p);
    return si;
ERR_EXIT:
    screen_info_destroy(&si);
    rns_json_destroy(&p);
    return NULL;

}
void screen_info_destroy(screen_info_t** si)
{
    if(*si == NULL)
    {
        return;
    }

    if((*si)->phoneid)
    {
        rns_free((*si)->phoneid);
    }
    if((*si)->SDcontentid)
    {
        rns_free((*si)->SDcontentid);
    }
    if((*si)->HDcontentid)
    {
        rns_free((*si)->HDcontentid);
    }
    if((*si)->content)
    {
        rns_free((*si)->content);
    }
    if((*si)->screenCode)
    {
        rns_free((*si)->screenCode);
    }
    if((*si)->name)
    {
        rns_free((*si)->name);
    }
    if((*si)->media_id)
    {
        rns_free((*si)->media_id);
    }
    if((*si)->provider_id)
    {
        rns_free((*si)->provider_id);
    }

//fuxiang start
	if((*si)->mobile)
   {
	   rns_free((*si)->mobile);
   }
//fuxiang end

	
    rns_free(*si);
    *si = NULL;
    return;
}

stb_playhistory_t* stb_playhistory_create(uchar_t* buf, uint32_t size)
{
    stb_playhistory_t* shistory = (stb_playhistory_t*)rns_malloc(sizeof(stb_playhistory_t));
    if(shistory == NULL)
    {
        goto ERR_EXIT;
    }

    
    rns_json_t* p = rns_json_read(buf, size);
    if(p == NULL)
    {
        goto ERR_EXIT;
    }
    rns_json_t* node = rns_json_node(p, "playsecond", NULL);
    if(node != NULL)
    {
        shistory->playsecond = node->valueint;
    }
    node = rns_json_node(p, "stbid", NULL);
    if(node == NULL)
    {
        goto ERR_EXIT;
    }
    shistory->stb_id = rns_dup((uchar_t*)node->valuestring);
    
    node = rns_json_node(p, "uid", NULL);
    if(node != NULL)
    {
        shistory->user_id = rns_dup((uchar_t*)node->valuestring);
    }

	//fuxiang start
	node = rns_json_node(p,"mobile",NULL);
	if(node != NULL)
	{
		shistory->mobile = rns_dup((uchar_t * )node->valuestring);
	}
	//fuxaing end
    
    node = rns_json_node(p, "addtime", NULL);
    if(node != NULL)
    {
        shistory->addtime = node->valueint;
    }
    
    
    node = rns_json_node(p, "metatype", NULL);
    if(node != NULL)
    {
        shistory->metatype = node->valueint;
    }
    
    
    node = rns_json_node(p, "serial", NULL);
    if(node != NULL)
    {
        shistory->serial = node->valueint;
    }
    
    
    node = rns_json_node(p, "screenCode", NULL);
    if(node != NULL)
    {
        shistory->screencode = rns_dup((uchar_t*)node->valuestring);
    }
    
    
    node = rns_json_node(p, "id", NULL);
    if(node != NULL)
    {
        shistory->id = node->valueint;
    }
    
    
    node = rns_json_node(p, "totaltime", NULL);
    if(node != NULL)
    {
        shistory->totaltime = node->valueint;
    }


    rns_json_destroy(&p);
    return shistory;
ERR_EXIT:
    if(shistory != NULL)
    {
        rns_free(shistory);
    }
    rns_json_destroy(&p);
    return NULL;
}

stb_mark_t* stb_mark_create(uchar_t* buf, uint32_t size)
{
    return stb_playhistory_create(buf, size);
}

stb_mark_t* stb_mark_create2(uchar_t* stb_id, uchar_t* user_id, int32_t metatype, uchar_t* screencode, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial)
{
    return stb_create_playhistory(stb_id, user_id, metatype, screencode, addtime, playsecond, totaltime, serial);
}

stb_playhistory_t* stb_create_playhistory(uchar_t* stb_id, uchar_t* user_id, int32_t metatype, uchar_t* screencode, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial)
{
    stb_playhistory_t* stb = (stb_playhistory_t*)rns_malloc(sizeof(stb_playhistory_t));
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }

    stb->stb_id= rns_dup(stb_id);
    stb->user_id= rns_dup(user_id);
    stb->screencode= rns_dup(screencode);
    
    stb->addtime	= addtime;
    stb->metatype   = metatype;
    stb->playsecond = playsecond;
    stb->totaltime  = totaltime;
    stb->serial	  	= serial;

    return stb;
ERR_EXIT:
    if(stb != NULL)
    {
        if(stb->stb_id!= NULL)
        {
            rns_free(stb->stb_id);
        }
        if(stb->user_id!= NULL)
        {
            rns_free(stb->user_id);
        }
        if(stb->screencode!= NULL)
        {
            rns_free(stb->screencode);
        }	
        rns_free(stb);
    }

    return NULL;
}

void stb_destroy_playhistory(stb_playhistory_t **stb)
{
    if(*stb != NULL)
    {
        if((*stb)->stb_id!= NULL)
        {
            rns_free((*stb)->stb_id);
        }
        if((*stb)->user_id!= NULL)
        {
            rns_free((*stb)->user_id);
        }
	//fuxaing sart
	if((*stb)->mobile != NULL)
	{
		rns_free((*stb)->mobile);
	}
	//fuxiang end
        if((*stb)->screencode!= NULL)
        {
            rns_free((*stb)->screencode);
        }	
        rns_free(*stb);
    }
    *stb = NULL;

    return;

}


stbdev_t* stb_create(uchar_t* stbid, uchar_t* version, uchar_t* StbModel, uchar_t* StbCom, uchar_t* StbMac, uchar_t* StbUserId, uint32_t StbVolume, uchar_t* trsid)
{
    stbdev_t* stb = (stbdev_t*)rns_malloc(sizeof(stbdev_t));
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }

    INIT_LIST_HEAD(&stb->phones);
    

    stb->stbid = rns_malloc(rns_strlen(stbid) + 1);
    if(stb->stbid == NULL)
    {
        goto ERR_EXIT;
    }
    rns_strncpy(stb->stbid, stbid, rns_strlen(stbid));

   
    stb->version = rns_dup(version);
    stb->StbModel = rns_dup(StbModel);
    stb->StbCom = rns_dup(StbCom);
    stb->StbMac = rns_dup(StbMac);
    
    stb->StbUserId = rns_dup(StbUserId);
    stb->StbVolume = StbVolume;
    stb->trsid = rns_dup(trsid);
    
    return stb;

ERR_EXIT:
    if(stb != NULL)
    {
        if(stb->stbid != NULL)
        {
            rns_free(stb->stbid);
        }
        if(stb->version != NULL)
        {
            rns_free(stb->version);
        }
        if(stb->StbModel != NULL)
        {
            rns_free(stb->StbModel);
        }
        if(stb->StbCom != NULL)
        {
            rns_free(stb->StbCom);
        }
        if(stb->StbMac != NULL)
        {
            rns_free(stb->StbMac);
        }
        if(stb->StbUserId != NULL)
        {
            rns_free(stb->StbUserId);
        }
        if(stb->trsid != NULL)
        {
            rns_free(stb->trsid);
        }
        
        rns_free(stb);
    }

    return NULL;
}

stbdev_t* stb_create2(uchar_t* buf, uint32_t size)
{
    rns_json_t* jstb = rns_json_read(buf, size);
    if(jstb == NULL)
    {
        return NULL;
    }
    
    rns_json_t* stbid = rns_json_node(jstb, "stbid", NULL);
    rns_json_t* StbMac = rns_json_node(jstb, "StbMac", NULL);
    rns_json_t* StbModel = rns_json_node(jstb, "StbModel", NULL);
    rns_json_t* StbCom = rns_json_node(jstb, "StbCom", NULL);
    rns_json_t* StbUserId = rns_json_node(jstb, "StbUserId", NULL);

    stbdev_t* stb = stb_create((uchar_t*)stbid->valuestring, NULL, StbModel ? (uchar_t*)StbModel->valuestring : NULL, StbCom ? (uchar_t*)StbCom->valuestring : NULL, StbMac ? (uchar_t*)StbMac->valuestring : NULL, StbUserId ? (uchar_t*)StbUserId->valuestring : NULL, 0, NULL);
    
    return stb;
}

void stb_destroy(stbdev_t** stb)
{
    if(*stb == NULL)
    {
        return;
    }

    if((*stb)->stbid != NULL)
    {
        rns_free((*stb)->stbid);
    }
    if((*stb)->version != NULL)
    {
        rns_free((*stb)->version);
    }
    if((*stb)->StbModel != NULL)
    {
        rns_free((*stb)->StbModel);
    }
    if((*stb)->StbCom != NULL)
    {
        rns_free((*stb)->StbCom);
    }
    if((*stb)->StbMac != NULL)
    {
        rns_free((*stb)->StbMac);
    }
    if((*stb)->StbUserId != NULL)
    {
        rns_free((*stb)->StbUserId);
    }
    if((*stb)->trsid != NULL)
    {
        rns_free((*stb)->trsid);
    }
    
    rns_free(*stb);
    *stb = NULL;
    
    return;
}


stbdev_t* stb_parse(rns_mysql_t* mysql)
{
    uint32_t volume = 0;
    if(mysql->row[6] != NULL)
    {
        volume = rns_atoi(mysql->row[6]);
    }
    stbdev_t* stb = stb_create((uchar_t*)mysql->row[0], (uchar_t*)mysql->row[1], (uchar_t*)mysql->row[2], (uchar_t*)mysql->row[3], (uchar_t*)mysql->row[4], (uchar_t*)mysql->row[5], volume, (uchar_t*)mysql->row[7]);
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
    return stb;

ERR_EXIT:
    stb_destroy(&stb);
    return NULL;
}

int32_t stb_findall(rns_mysql_t* mysql, uchar_t* trsid, rns_mysql_cb_t* cb)
{
    char_t* cmd = "select stbid,version,StbModel,StbCom,StbMac,StbUserId,StbVolume,tr_peer from foss where tr_peer='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, trsid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t stb_findbystbid(rns_mysql_t* mysql, stbdev_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        printf("input stb invalid\n");
        return -1;
    }

    char_t* cmd = "select stbid,version,StbModel,StbCom,StbMac,StbUserId,StbVolume,tr_peer from foss where stbid='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, stb->stbid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t stb_find(rns_mysql_t* mysql, uchar_t* stbid, rns_mysql_cb_t* cb)
{
    if(stbid == NULL)
    {
        return -1;
    }
    
    char_t* cmd = "select stbid,version,StbModel,StbCom,StbMac,StbUserId,StbVolume,tr_peer from foss where stbid='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, stbid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}


int32_t stb_findbyuserid(rns_mysql_t* mysql, char_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        printf("input stb invalid\n");
        return -1;
    }

    char_t* cmd = "select stbid,version,StbModel,StbCom,StbMac,StbUserId,StbVolume,tr_peer from foss where StbUserId='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, stb);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

int32_t stb_update_bystbid(rns_mysql_t* mysql, uchar_t* trsid, stbdev_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "update foss set tr_peer='%s', StbModel='%s', StbCom='%s', StbMac='%s', StbUserId='%s', StbVolume=%d where stbid ='%s'";
    snprintf((char_t*)buf, 1024, sql, trsid, stb->StbModel, stb->StbCom, stb->StbMac, stb->StbUserId, stb->StbVolume,stb->stbid);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

int32_t stb_update_info(rns_mysql_t* mysql, stbdev_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        return -1;
    }
    
    uchar_t buf[1024];
    char_t* sql = "update foss set StbModel='%s',StbCom='%s',StbMac='%s',StbUserId='%s',StbVolume=%d where stbid='%s'";
    snprintf((char_t*)buf, 1024, sql, stb->StbModel, stb->StbCom, stb->StbMac, stb->StbUserId, stb->StbVolume,stb->stbid);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t stb_insert(rns_mysql_t* mysql, uchar_t* trsid, stbdev_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "replace into foss(stbid,tr_peer,StbModel,StbCom,StbMac,StbUserId,StbVolume) value('%s','%s','%s','%s','%s','%s',%d)";
    snprintf((char_t*)buf, 1024, sql,stb->stbid, trsid, stb->StbModel, stb->StbCom, stb->StbMac, stb->StbUserId, stb->StbVolume, trsid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t stb_registry(rns_mysql_t* mysql, uchar_t* stbid, uchar_t* trsid, rns_mysql_cb_t* cb)
{
    if(stbid == NULL || trsid == NULL)
    {
        return -1;
    }
    
    uchar_t buf[1024];
    char_t* sql = "insert into foss(stbid,tr_peer) value('%s','%s')";
    snprintf((char_t*)buf, 1024, sql, stbid, trsid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t stb_playhistory_search(rns_mysql_t* mysql, char_t* stbid, rns_mysql_cb_t* cb)
{
    char_t* cmd = "select stb_id,user_id,metatype,screencode,addtime,playsecond,totaltime,serial from stb_playhistory where stb_id='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, stbid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);	
}

int32_t stb_mark_search(rns_mysql_t* mysql, char_t* stbid, rns_mysql_cb_t* cb)
{

    char_t* cmd = "select stb_id,user_id,metatype,screencode,addtime,totaltime,serial from stb_mark where stb_id='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, stbid);

    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

stb_playhistory_t* stb_playhistory_parse(rns_mysql_t* mysql)
{
    stb_playhistory_t* stb = stb_create_playhistory((uchar_t*)mysql->row[0],(uchar_t*)mysql->row[1], rns_atoi(mysql->row[2]), (uchar_t*)mysql->row[3], rns_atoi(mysql->row[4]), rns_atoi(mysql->row[5]), rns_atoi(mysql->row[6]), rns_atoi(mysql->row[7]));
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
	
    return stb;

ERR_EXIT:
    
    return NULL;

}
stb_playhistory_t* stb_mark_parse(rns_mysql_t* mysql)
{
    stb_playhistory_t* stb = stb_create_playhistory((uchar_t*)mysql->row[0],(uchar_t*)mysql->row[1], rns_atoi(mysql->row[2]), (uchar_t*)mysql->row[3], rns_atoi(mysql->row[4]), 0, rns_atoi(mysql->row[5]), rns_atoi(mysql->row[6]));
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
	
    return stb;

ERR_EXIT:
    return NULL;
}
int32_t stb_playhistory_insert(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        return -1;
    }

    uchar_t buf[1024];
	//fuxiang start
    char_t* sql = "replace into stb_playhistory(stb_id,user_id,mobile,screencode,metatype,addtime,totaltime,serial,playsecond) values ('%s','%s','%s','%s',%d,%d,%d,%d,%d);";
    snprintf((char_t*)buf, 1024, sql,stb->stb_id,stb->user_id,stb->mobile,stb->screencode,stb->metatype,stb->addtime,stb->totaltime,stb->serial,stb->playsecond);
    	//fuxiang end
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}
int32_t stb_mark_insert(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "replace into stb_mark(stb_id,user_id,screencode,metatype,addtime,totaltime,serial) values ('%s','%s','%s',%d,%d,%d,%d);";
    snprintf((char_t*)buf, 1024, sql,stb->stb_id,stb->user_id,stb->screencode,stb->metatype,stb->addtime,stb->totaltime,stb->serial);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}
int32_t stb_playhistory_update(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        printf("input stb invalid\n");
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "update stb_playhistory set metatype=%d,addtime=%d,totaltime=%d,serial=%d ,playsecond=%d where stb_id='%s' and screencode='%s';";
    snprintf((char_t*)buf, 1024, sql, stb->metatype,stb->addtime,stb->totaltime,stb->serial,stb->playsecond,stb->stb_id,stb->screencode);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);


}
int32_t stb_mark_update(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        printf("input stb invalid\n");
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "update stb_mark set metatype=%d,addtime=%d,totaltime=%d,serial=%d where stb_id='%s' and screencode='%s';";
    snprintf((char_t*)buf, 1024, sql, stb->metatype,stb->addtime,stb->totaltime,stb->serial,stb->stb_id,stb->screencode);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

static int32_t mark_delete(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb)
{
    if(NULL == stb)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "delete from stb_mark where stb_id='%s'";
    snprintf((char_t*)buf, 1024, sql, stb->stb_id);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

void stb_playhistory_dao_init(stb_playhistory_dao_t* dao)
{
    dao->search = stb_playhistory_search;
    dao->parse = stb_playhistory_parse;
    dao->insert = stb_playhistory_insert;
    dao->update = stb_playhistory_update;
    
    return;
}

void stb_mark_dao_init(stb_mark_dao_t* dao)
{
    dao->search = stb_mark_search;
    dao->parse = stb_mark_parse;
    dao->insert	= stb_mark_insert;
    dao->update	= stb_mark_update;
    dao->delete	= mark_delete;

    return;
}

void stb_dao_init(stb_dao_t* dao)
{
    dao->search = stb_find;
    dao->search_all = stb_findall;
    dao->search_bystbid = stb_findbystbid;
	dao->search_byuserid = stb_findbyuserid;
    dao->parse = stb_parse;
    dao->insert	= stb_insert;
    dao->registry = stb_registry;
    dao->update_bystbid = stb_update_bystbid;
    dao->update_info = stb_update_info;
   
    return;
}

