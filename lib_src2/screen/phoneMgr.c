#include "phoneMgr.h"
#include "rns_dbpool.h"
#include "rns_hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h> 
#include <sys/time.h>
#include "rns_json.h"

phone_playhistory_t* phone_playhistory_create(uchar_t* buf, uint32_t size)
{
    phone_playhistory_t* phistory = (phone_playhistory_t*)rns_malloc(sizeof(phone_playhistory_t));
    if(phistory == NULL)
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
        phistory->playsecond = node->valueint;
    }
    node = rns_json_node(p, "phoneid", NULL);
    if(node != NULL)
    {
        phistory->phoneid = node->valuestring;
    }
//fuxaing start
	node = rns_json_node(p, "mobile", NULL);
	  if(node != NULL)
	  {
		  phistory->mobile = node->valuestring;
	  }

//fuxaing end
	
    phistory->phoneid = rns_dup((uchar_t*)node->valuestring);
    
    node = rns_json_node(p, "media_id", NULL);
    if(node != NULL)
    {
        phistory->media_id = rns_dup((uchar_t*)node->valuestring);
    }
    
    
    node = rns_json_node(p, "addtime", NULL);
    if(node != NULL)
    {
        if(node->valuestring == NULL)
        {
            phistory->addtime = node->valueint;
        }
        else
        {
            phistory->addtime = rns_atoi(node->valuestring);
        }
    }
    
    
    node = rns_json_node(p, "metatype", NULL);
    if(node != NULL)
    {
        if(node->valuestring == NULL)
        {
            phistory->metatype = node->valueint;
        }
        else
        {
            phistory->metatype = rns_atoi(node->valuestring);
        }
    }
    
    
    node = rns_json_node(p, "serial", NULL);
    if(node != NULL)
    {
        if(node->valuestring == NULL)
        {
            phistory->serial = node->valueint;
        }
        else
        {
            phistory->serial = rns_atoi(node->valuestring);
        }
    }
    
    
    node = rns_json_node(p, "title", NULL);
    if(node != NULL)
    {
        phistory->title = rns_dup((uchar_t*)node->valuestring);
    }  
    
    node = rns_json_node(p, "totaltime", NULL);
    if(node != NULL)
    {
        if(node->valuestring == NULL)
        {
            phistory->totaltime = node->valueint;
        }
        else
        {
            phistory->totaltime = rns_atoi(node->valuestring);
        }
    }

    rns_json_destroy(&p);
    return phistory;
ERR_EXIT:
    if(phistory != NULL)
    {
        rns_free(phistory);
    }
    rns_json_destroy(&p);
    return NULL;
}

phone_mark_t* phone_mark_create(uchar_t* buf, uint32_t size)
{
    return phone_playhistory_create(buf, size);
}

phone_playhistory_t* phone_create_playhistory(uchar_t* phoneid, uchar_t* media_id, int32_t metatype, uchar_t* title, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial)
{
    phone_playhistory_t* phone = (phone_playhistory_t*)rns_malloc(sizeof(phone_playhistory_t));
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }

    phone->phoneid = rns_dup(phoneid);
    if(NULL == phone->phoneid)
    {
        goto ERR_EXIT;

    }
    phone->media_id = rns_dup(media_id);
    if(NULL == phone->media_id)
    {
        goto ERR_EXIT;

    }
    phone->title = rns_dup(title);
    if(NULL == phone->title)
    {
        goto ERR_EXIT;
    }
    phone->addtime    = addtime;
    phone->metatype   = metatype;
    phone->playsecond = playsecond;
    phone->totaltime  = totaltime;
    phone->serial	  = serial;

    return phone;
ERR_EXIT:
    if(phone != NULL)
    {
        if(phone->phoneid != NULL)
        {
            rns_free(phone->phoneid);
        }
        if(phone->media_id != NULL)
        {
            rns_free(phone->media_id);
        }
        if(phone->title != NULL)
        {
            rns_free(phone->title);
        }	
        rns_free(phone);
    }

    return NULL;

}

//fuxaing start
phone_playhistory_t* phone_create_playhistory1(uchar_t* phoneid, uchar_t* media_id,uchar_t * mobile, int32_t metatype, uchar_t* title, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial)
{
    phone_playhistory_t* phone = (phone_playhistory_t*)rns_malloc(sizeof(phone_playhistory_t));
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }

    phone->phoneid = rns_dup(phoneid);
    if(NULL == phone->phoneid)
    {
        goto ERR_EXIT;

    }
    phone->media_id = rns_dup(media_id);
    if(NULL == phone->media_id)
    {
        goto ERR_EXIT;

    }

//fuxiang start
 phone->mobile = rns_dup(mobile);
    if(NULL == phone->mobile)
    {
        goto ERR_EXIT;

    }
//fuxiang end
    phone->title = rns_dup(title);
    if(NULL == phone->title)
    {
        goto ERR_EXIT;
    }
    phone->addtime    = addtime;
    phone->metatype   = metatype;
    phone->playsecond = playsecond;
    phone->totaltime  = totaltime;
    phone->serial	  = serial;

    return phone;
ERR_EXIT:
    if(phone != NULL)
    {
        if(phone->phoneid != NULL)
        {
            rns_free(phone->phoneid);
        }
        if(phone->media_id != NULL)
        {
            rns_free(phone->media_id);
        }
//fuxiang start
	 if(phone->mobile != NULL)
        {
            rns_free(phone->mobile);
        }
//fuxaing end
        if(phone->title != NULL)
        {
            rns_free(phone->title);
        }	
        rns_free(phone);
    }

    return NULL;

}


//fxiang end

void phone_mark_destroy(phone_mark_t** mark)
{
    phone_playhistory_destroy(mark);
}
void phone_playhistory_destroy(phone_playhistory_t **phone)
{
    if(*phone != NULL)
    {
        if((*phone)->phoneid != NULL)
        {
            rns_free((*phone)->phoneid);
        }
        if((*phone)->media_id != NULL)
        {
            rns_free((*phone)->media_id);
        }
//fuxiang start
	 if((*phone)->mobile != NULL)
        {
            rns_free((*phone)->mobile);
        }
//fuxiang end
        if((*phone)->title != NULL)
        {
            rns_free((*phone)->title);
        }	
        rns_free(*phone);
    }
    *phone = NULL;

    return;

}


phonedev_t* phone_create(uchar_t* phoneid)
{
    phonedev_t* phone = (phonedev_t*)rns_malloc(sizeof(phonedev_t));
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }
    
    phone->phoneid = rns_malloc(rns_strlen(phoneid) + 1);
    if(phone->phoneid == NULL)
    {
        goto ERR_EXIT;
    }
    rns_strncpy(phone->phoneid, phoneid, rns_strlen(phoneid));

   
    
    return phone;
ERR_EXIT:
    return NULL;
}

void phone_destroy(phonedev_t** phone)
{
    if(*phone == NULL)
    {
        return;
    }

    if((*phone)->phoneid != NULL)
    {
        rns_free((*phone)->phoneid);
    }
    
    if((*phone)->stbid != NULL)
    {
        rns_free((*phone)->stbid);
    }
    rns_free(*phone);
    return;
}


phonedev_t* phone_parse(rns_mysql_t* mysql)
{
    phonedev_t* phone = phone_create((uchar_t*)mysql->row[0]);
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }
    
    phone->stbid = rns_dup((uchar_t*)mysql->row[1]);
    
    return phone;

ERR_EXIT:
    if(phone != NULL)
    {
        if(phone->phoneid != NULL)
        {
            rns_free(phone);
        }
    }

    return NULL;
}

int32_t phone_findall(rns_mysql_t* mysql, rns_mysql_cb_t* cb)
{
    char_t* cmd = "select phoneid, stbid, date from phone";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t phone_findbyid(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        return -1;
    }

    char_t* cmd = "select phoneid, stbid, date from phone where phoneid='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, phone->phoneid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t phone_find(rns_mysql_t* mysql, uchar_t* phoneid, rns_mysql_cb_t* cb)
{
    if(NULL == phoneid)
    {
        return -1;
    }
    
    char_t* cmd = "select phoneid, stbid, date from phone where phoneid='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, phoneid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t phone_findbyuserid(rns_mysql_t* mysql, char_t* userid, rns_mysql_cb_t* cb)
{
    if(NULL == userid)
    {
        printf("input stb invalid\n");
        return -1;
    }

    char_t* cmd = "select stbid, version, StbModel, StbCom, StbMac, StbUserId, StbVolume from foss where StbUserId='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, userid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

int32_t phone_findbystbid(rns_mysql_t* mysql, char_t* stbid, rns_mysql_cb_t* cb)
{
    if(NULL == stbid)
    {
        return -1;
    }

    char_t* cmd = "select phoneid, stbid, date from phone where stbid='%s'";
    uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, stbid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

int32_t phone_update_byid(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        printf("input phone invalid\n");
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "update phone set stbid='%s' where phoneid ='%s';";
    snprintf((char_t*)buf, 1024, sql, phone->stb->stbid, phone->phoneid);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

int32_t phone_insert(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "replace into phone(phoneid, stbid, date) value('%s', '%s', %u)";
    snprintf((char_t*)buf, 1024, sql,phone->phoneid, phone->stb->stbid, phone->time);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t phone_insert2(rns_mysql_t* mysql, uchar_t* phoneid, uchar_t* stbid, rns_mysql_cb_t* cb)
{
    time_t t = time(NULL);
    uchar_t buf[1024];
    char_t* sql = "replace into phone(phoneid, stbid, date) value('%s', '%s', %u)";
    snprintf((char_t*)buf, 1024, sql,phoneid, stbid, t);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t phone_delete_byid(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "delete from phone where phoneid ='%s'";
    snprintf((char_t*)buf, 1024, sql, phone->phoneid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

int32_t phone_delete_byid2(rns_mysql_t* mysql, uchar_t* phoneid, rns_mysql_cb_t* cb)
{
    if(phoneid == NULL)
    {
        return -1;
    }
    
    uchar_t buf[1024];
    char_t* sql = "delete from phone where phoneid ='%s'";
    snprintf((char_t*)buf, 1024, sql, phoneid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}


void phone_update(rns_mysql_t* mysql, void* data)
{
    phone_dao_t phonedao;
    phone_dao_init(&phonedao);

    int ret = 0;
    phonedev_t* phone = (phonedev_t*)data;

    if(NULL == phone)
    {
        return;
    }

    ret = phonedao.update_byid(mysql, phone, &mysql->qcb);
    if(ret < 0)
    {
    }

    return;
}



int32_t phone_playhistory_search(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb)
{
//fuxiang start
    char_t* cmd = "select phoneid,media_id,mobile,metatype,title,addtime,playsecond,totaltime,serial from phone_playhistory where phoneid='%s'";
//fuxaing end
	uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, phone->phoneid);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);	
}

int32_t phone_mark_search(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb)
{
//fuxiang start
    char_t* cmd = "select phoneid,media_id,mobile,metatype,title,addtime,totaltime,serial  from phone_mark where phoneid='%s'";
//fuxiang end
uchar_t buf[1024];
    snprintf((char_t*)buf, 1024, cmd, phone->phoneid);

    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);
}

phone_playhistory_t* phone_playhistory_parse(rns_mysql_t* mysql)
{
    phone_playhistory_t* phone = phone_create_playhistory1((uchar_t*)mysql->row[0],(uchar_t*)mysql->row[1],(uchar_t*)mysql->row[2], mysql->row[3] == NULL ? 0 : rns_atoi(mysql->row[3]),(uchar_t*)mysql->row[4], mysql->row[5] == NULL ? 0 : rns_atoi(mysql->row[5]), mysql->row[6] == NULL ? 0 : rns_atoi(mysql->row[6]), mysql->row[7] == NULL ? 0 : rns_atoi(mysql->row[7]), mysql->row[8] == NULL ? 0 : rns_atoi(mysql->row[8]));
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }
	
    return phone;

ERR_EXIT:
    if(phone != NULL)
    {
        if(phone->phoneid != NULL)
        {
            rns_free(phone);
        }
    }
    return NULL;
}
phone_mark_t* phone_mark_parse(rns_mysql_t* mysql)
{
    phone_playhistory_t* phone = phone_create_playhistory1((uchar_t*)mysql->row[0],(uchar_t*)mysql->row[1], (uchar_t*)mysql->row[2],mysql->row[3] == NULL ? 0 : rns_atoi(mysql->row[3]),(uchar_t*)mysql->row[4], mysql->row[5] == NULL ? 0 : rns_atoi(mysql->row[5]), 0, mysql->row[6] == NULL ? 0 : rns_atoi(mysql->row[6]), mysql->row[7] == NULL ? 0 : rns_atoi(mysql->row[7]));
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }
	
    return phone;

ERR_EXIT:
    if(phone != NULL)
    {
        if(phone->phoneid != NULL)
        {
            rns_free(phone);
        }
    }
    return NULL;
}
int32_t phone_playhistory_insert(rns_mysql_t* mysql, phone_playhistory_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        return -1;
    }

    uchar_t buf[1024];
//fuxiang start
    char_t* sql = "replace into phone_playhistory(phoneid,media_id,mobile,metatype,title,addtime,totaltime,serial,playsecond) values ('%s','%s','%s',%d,'%s',%d,%d,%d,%d);";
    snprintf((char_t*)buf, 1024, sql,phone->phoneid,phone->media_id,phone->mobile,phone->metatype,phone->title,phone->addtime,phone->totaltime,phone->serial,phone->playsecond);
    //fuxaing end
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}
int32_t phone_mark_insert(rns_mysql_t* mysql, phone_playhistory_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        return -1;
    }

    uchar_t buf[1024];
//fuxiang start
    char_t* sql = "replace into phone_mark(phoneid,media_id,metatype,title,addtime,totaltime,serial,mobile) values ('%s','%s',%d,'%s',%d,%d,%d,%s);";
//fuaing end
	snprintf((char_t*)buf, 1024, sql,phone->phoneid,phone->media_id,phone->metatype,phone->title,phone->addtime,phone->totaltime,phone->serial,phone->mobile);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}
int32_t phone_playhistory_update(rns_mysql_t* mysql, phone_playhistory_t* phone, rns_mysql_cb_t* cb)
{
    if(NULL == phone)
    {
        printf("input phone invalid\n");
        return -1;
    }

    uchar_t buf[1024];
	//fuxiang start
    char_t* sql = "update phone_playhistory set metatype=%d,addtime=%d,totaltime=%d,serial=%d ,playsecond=%d ,title='%s',mobile = '%s'  where phoneid='%s' and media_id='%s';";
    snprintf((char_t*)buf, 1024, sql, phone->metatype,phone->addtime,phone->totaltime,phone->serial,phone->playsecond,phone->title,phone->mobile,phone->phoneid,phone->media_id);
	//fuxiang end
	printf("updata cmd = %s\n", buf);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);


}
int32_t phone_mark_update(rns_mysql_t* mysql, phone_mark_t* mark, rns_mysql_cb_t* cb)
{
    if(mark == NULL)
    {
        return -1;
    }

    uchar_t buf[1024];
//fuxiang start
    char_t* sql = "update phone_mark set metatype=%d,addtime=%d,totaltime=%d,serial=%d ,title='%s' ,mobile = '%s'   where phoneid='%s' and media_id='%s';";
snprintf((char_t*)buf, 1024, sql, mark->metatype, mark->addtime, mark->totaltime, mark->serial, mark->title, mark->mobile,mark->phoneid, mark->media_id);
//fuxiang end

	printf("updata cmd = %s\n", buf);
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}
int32_t phone_mark_del(rns_mysql_t* mysql, uchar_t* phoneid, uchar_t* media_id, rns_mysql_cb_t* cb)
{
    if(phoneid == NULL || media_id == NULL)
    {
        return -1;
    }

    uchar_t buf[1024];
    char_t* sql = "delete from phone_mark where phoneid='%s' and media_id='%s'";
    snprintf((char_t*)buf, 1024, sql, phoneid, media_id);
    
    return rns_mysql_query(mysql, buf, rns_strlen(buf), cb);

}

void phone_playhistory_dao_init(phone_playhistory_dao_t* dao)
{
    dao->search = phone_playhistory_search;
    dao->parse = phone_playhistory_parse;
    dao->insert = phone_playhistory_insert;
    dao->update = phone_playhistory_update;

    return;
}

void phone_mark_dao_init(phone_mark_dao_t* dao)
{
    dao->search = phone_mark_search;
    dao->parse = phone_mark_parse;
    dao->insert = phone_mark_insert;
    dao->update = phone_mark_update;
    dao->delete = phone_mark_del;

    return;
}

void phone_dao_init(phone_dao_t* dao)
{
    dao->search = phone_find;
    dao->search_all = phone_findall;
    dao->search_byid = phone_findbyid;
	dao->search_bystbid = phone_findbystbid;
	dao->search_byuserid = phone_findbyuserid;
    dao->parse = phone_parse;
    dao->delete_byid = phone_delete_byid;
    dao->delete_byid2 = phone_delete_byid2;
    dao->insert	= phone_insert;
    dao->insert2	= phone_insert2;
    dao->update_byid = phone_update_byid;
	
    return;
}


