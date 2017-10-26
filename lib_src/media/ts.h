#ifndef _TS_H
#define _TS_H

#include "rns.h"
#include <stdint.h>
typedef struct
{
	uint32_t sync_byte 						:8; //	同步字节，固定为0x47
	uint32_t transport_error_indicator 		:1; 	//传输错误指示符，表明在ts头的adapt域后由一个无用字节，通常都为0，这个字节算在adapt域长度内
	uint32_t payload_unit_start_indicator 	:1; 	//负载单元起始标示符，一个完整的数据包开始时标记为1
	uint32_t transport_priority 			:1;	//传输优先级，0为低优先级，1为高优先级，通常取0
	uint32_t pid 							:13; //	pid值
	uint32_t transport_scrambling_control 	:2; 	//传输加扰控制，00表示未加密
	uint32_t adaptation_field_control 		:2; 	//是否包含自适应区，‘00’保留；‘01’为无自适应域，仅含有效负载；‘10’为仅含自适应域，无有效负载；‘11’为同时带有自适应域和有效负载。
	uint32_t continuity_counter 			:4; 	//递增计数器，从0-f，起始值不一定取0，但必须是连续的	
}ts_header_t;

typedef struct
{
uint32_t pes_start_code 	:24; 	//开始码，固定为0x000001
uint32_t stream_id 			:8; 	//音频取值（0xc0-0xdf），通常为0xc0 视频取值（0xe0-0xef），通常为0xe0
uint32_t pes_packet_length 	:16; 	//后面pes数据的长度，0表示长度不限制，只有视频数据长度会超过0xffff
uint32_t flag1 				:8; 	//通常取值0x80，表示数据不加密、无优先级、备份的数据
uint32_t flag2				:8; 	//取值0x80表示只含有pts，取值0xc0表示含有pts和dts
uint32_t pes_data_length 	:8; 	//后面数据的长度，取值5或10
									//pts 	5B 	33bit值
									//dts 	5B 	33bit值
}ts_pes_header_t;

typedef struct ts_pat_program_s
{
	list_head_t list;
	uint32_t program_number   :  16;  //节目号  
    uint32_t program_map_pid  :  13; // 节目映射表的PID，节目号大于0时对应的PID，每个节目对应一个  
}ts_pat_program_t;

typedef struct
{
	uint32_t table_id                     : 8; //固定为0x00 ，标志是该表是PAT表  
    uint32_t section_syntax_indicator     : 1; //段语法标志位，固定为1  
    uint32_t zero                         : 1; //0  
    uint32_t reserved                   : 2; // 保留位  
    uint32_t section_length               : 12; //表示从下一个字段开始到CRC32(含)之间有用的字节数  
    uint32_t transport_stream_id          : 16; //该传输流的ID，区别于一个网络中其它多路复用的流  
    uint32_t reserved1                   : 2;// 保留位  
    uint32_t version_number               : 5; //范围0-31，表示PAT的版本号  
    uint32_t current_next_indicator       : 1; //发送的PAT是当前有效还是下一个PAT有效  
    uint32_t section_number               : 8; //分段的号码。PAT可能分为多段传输，第一段为00，以后每个分段加1，最多可能有256个分段  
    uint32_t last_section_number          : 8;  //最后一个分段的号码  
   
    list_head_t program_head;  
    uint32_t reserved2                   : 3; // 保留位  
    uint32_t network_PID                  : 13; //网络信息表（NIT）的PID,节目号为0时对应的PID为network_PID  
    uint32_t CRC_32                       : 32;  //CRC32校验码  
}ts_pat_t;


typedef struct 
{
	list_head_t list;
	uint32_t stream_type                       : 8; //指示特定PID的节目元素包的类型。该处PID由elementary PID指定  
    uint32_t stream_pid                   		: 13; //该域指示TS包的PID值。这些TS包含有相关的节目元素  
    uint32_t ES_info_length                   : 12; //前两位bit为00。该域指示跟随其后的描述相关节目元素的byte数  
    uint32_t descriptor;  
}ts_pmt_stream_t;

typedef struct
{
	uint32_t table_id                        : 8; //固定为0x02, 表示PMT表  
    uint32_t section_syntax_indicator        : 1; //固定为0x01  
    uint32_t zero                            : 1; //0x01  
    uint32_t reserved1                       : 2; //0x03  
    uint32_t section_length                  : 12;//首先两位bit置为00，它指示段的byte数，由段长度域开始，包含CRC。  
    uint32_t program_number                  : 16;// 指出该节目对应于可应用的Program map PID  
    uint32_t reserved2                        : 2; //0x03  
    uint32_t version_number                  : 5; //指出TS流中Program map section的版本号  
    uint32_t current_next_indicator          : 1; //当该位置1时，当前传送的Program map section可用；  
                                                     //当该位置0时，指示当前传送的Program map section不可用，下一个TS流的Program map section有效。  
    uint32_t section_number                  : 8; //固定为0x00  
    uint32_t last_section_number             : 8; //固定为0x00  
    uint32_t reserved3                        : 3; //0x07  
    uint32_t pcr_pid                         : 13; //指明TS包的PID值，该TS包含有PCR域，  
													//该PCR值对应于由节目号指定的对应节目。  
													//如果对于私有数据流的节目定义与PCR无关，这个域的值将为0x1FFF。  
    uint32_t reserved                        : 4; //预留为0x0F  
    uint32_t program_info_length            : 12; //前两位bit为00。该域指出跟随其后对节目信息的描述的byte数。  
      
    list_head_t ts_pmt_stream_head;  				//每个元素包含8位, 指示特定PID的节目元素包的类型。该处PID由elementary PID指定  
    uint32_t reserved4                        : 3; //0x07  
    uint32_t reserved5                        : 4; //0x0F  
    uint32_t crc                             : 32;   
}ts_pmt_t;

uint32_t is_pat(uchar_t* ts_ptr);
uint32_t is_pid(uchar_t* ts_ptr, uint32_t pid);
uint32_t is_pcr(uchar_t* ts_ptr, uint32_t pid);
uint32_t is_idr(uchar_t* ts_ptr);

uchar_t* find_pid(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t pid);
uchar_t* find_pat(uchar_t* ts_ptr, uchar_t* ts_end);
uchar_t* find_pat_r(uchar_t* ts_ptr, uchar_t* ts_start);
uchar_t* find_pcr(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t pid);
uchar_t* find_pcr_r(uchar_t* ts_ptr, uchar_t* ts_start, uint32_t pid);
uint32_t get_pcr(uchar_t* ts_ptr);

uint32_t parse_ts(uchar_t* ts_ptr, ts_header_t* ts_header);

ts_pat_t* parse_pat(uchar_t* walker);
ts_pat_t* parse_pat2(uchar_t* pat_ptr);
void pat_destroy(ts_pat_t** pat);
ts_pmt_t* parse_pmt2(uchar_t* pmt_ptr);
ts_pmt_t* parse_pmt(uchar_t* walker);
void pmt_destroy(ts_pmt_t** pmt);

uint32_t parse_pes(uchar_t* walker, ts_pes_header_t* pes_header);
uint32_t find_pcr_pid(uchar_t* ts_ptr, uchar_t* ts_end);
uchar_t* find_idr(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t pid);
uchar_t* find_ts_by_pcr(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t tick);
uint32_t find_duration_by_offset(uchar_t* ts_ptr,  uchar_t* ts_end, uchar_t* start_offset, uchar_t* end_offset);


#endif
