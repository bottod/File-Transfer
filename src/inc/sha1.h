#ifndef SHA1_H
#define SHA1_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>






 /* 得到单个文件的sha1值
* 唯一对外暴露的接口
* char hashcode[40]即可接收
*/
char * get_sha1_from_file(char * filename);




#endif