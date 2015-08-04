﻿#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include<sys/time.h>

#include "wilddog.h"
#include "test_lib.h"

#define TESTURL_NUM		50
#define TESTURL_HARD	"coap://"
#define TESTURL_TAIL	".wilddogio.com/multipletest"

#define MULTIPLE_HOST1	"c_test1"
#define MULTIPLE_HOST2	"c_test2"
#define MULTIPLE_HOST3	"c_test3"

STATIC  char url1[TESTURL_NUM];
STATIC  char url2[TESTURL_NUM];
STATIC  char url3[TESTURL_NUM];


#define MULTIPLETEST_KEY	"MULTIPLE_key"


#define MULTIPLETEST_ONREQUEST(cmd)		((cmd) == MULTIPLETEST_CMD_ON)
#define MULTIPLETEST_OFFREQUEST(cmd)	((cmd) == MULTIPLETEST_CMD_OFF)
#define MULTIPLETEST_NEXTREQUEST(cmd)	((cmd) = ((cmd) == MULTIPLETEST_CMD_OFF)? \
											MULTIPLETEST_CMD_ON:((cmd)+1))
#define MULTIPLETEST_GETRESULT(p,v)	( p = ( v == 0)?"FAULT":"PASS")

typedef enum _MULTIPLETEST_CMD_TYPE
{
    MULTIPLETEST_CMD_NON = 0,
    MULTIPLETEST_CMD_ON,
    MULTIPLETEST_CMD_GET,
    MULTIPLETEST_CMD_SET,
    MULTIPLETEST_CMD_PUSH,
    MULTIPLETEST_CMD_DELE,
	MULTIPLETEST_CMD_OFF,
	    
}MULTIPLETEST_CMD_TYPE;
typedef struct MULTIPLE_SETDATA_T
{
	u8 d_cmd;
	u8 d_recvFlag;
	u8 d_sendFault;
	
	u8 d_setResult;
	u8 d_getResult;
	u8 d_deleteResult;
	u8 d_pushResult;
	u8 d_observerResult;
	char *p_host;
	
}Multiple_client_T;



STATIC Multiple_client_T multipleClient[3];

STATIC int multiple_judge(Wilddog_Node_T* p_snapshot,char* src)
{
	int len;
	if( p_snapshot != 0 &&
		0 == strcmp( (const char*)wilddog_node_getValue(p_snapshot,&len),src))
		return 1;
	else
		return 0;
}

STATIC void multiple_getValueFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
	Multiple_client_T *p_client = (Multiple_client_T*)arg;
	p_client->d_recvFlag = 1;
	printf("Get : error: %d %s \n",err,p_client->p_host);
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		return;
	else	
		p_client->d_getResult = (u8) multiple_judge(p_snapshot,p_client->p_host);
	
}

STATIC void multiple_removeValueFunc(void* arg, Wilddog_Return_T err)
{
	Multiple_client_T *p_client = (Multiple_client_T*)arg;
	p_client->d_recvFlag = 1;
	
	printf("remove : error: %d %s \n",err,p_client->p_host);
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		return;
	else
		p_client->d_deleteResult = 1;

}
STATIC void multiple_setValueFunc(void* arg, Wilddog_Return_T err)
{
                        
	Multiple_client_T *p_client = (Multiple_client_T*)arg;
	printf("set : error: %d %s \n",err,p_client->p_host);
	p_client->d_recvFlag = 1;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		return;
	else	
		p_client->d_setResult = 1;

}

STATIC void multiple_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{

	Multiple_client_T *p_client = (Multiple_client_T*)arg;
	
	printf("push : error: %d %s \n",err,p_client->p_host);
	p_client->d_recvFlag = 1;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		return;
	else	
		p_client->d_pushResult = 1;

}

STATIC void multiple_addObserverFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
    
	Multiple_client_T *p_client = (Multiple_client_T*)arg;
	
	printf("observer : error: %d %s \n",err,p_client->p_host);
	p_client->d_recvFlag = 1;
	
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		return;
	else	
		p_client->d_observerResult = (u8) multiple_judge(p_snapshot,p_client->p_host);

}

STATIC int multipletest_request(MULTIPLETEST_CMD_TYPE type,
										Wilddog_T client,Multiple_client_T *p_mul_client)
{

	Wilddog_Node_T *p_head = NULL,*p_node = NULL;
	int res = 0;
    /*Create an node which type is an object*/
    p_head = wilddog_node_createObject(NULL);
    
    /*Create an node which type is UTF-8 Sring*/
    p_node = wilddog_node_createUString((Wilddog_Str_T *)MULTIPLETEST_KEY,(Wilddog_Str_T *)p_mul_client->p_host);
    
    /*Add p_node to p_head, then p_node is the p_head's child node*/
    wilddog_node_addChild(p_head, p_node);
	
	p_mul_client->d_cmd= type;
	p_mul_client->d_recvFlag = 0;
	p_mul_client->d_sendFault = 0;
    switch(type)
    {
        case MULTIPLETEST_CMD_GET:
            /*Send the query method*/
            res = wilddog_getValue(client, (onQueryFunc)multiple_getValueFunc, (void*)p_mul_client);
            break;
        case MULTIPLETEST_CMD_SET:  
            /*Send the set method*/
            res = wilddog_setValue(client,p_head,multiple_setValueFunc,(void*)p_mul_client);
            break;
        case MULTIPLETEST_CMD_PUSH:
            /*Send the push method*/
            res = wilddog_push(client, p_head, multiple_pushFunc, (void *)p_mul_client);  
            break;
        case MULTIPLETEST_CMD_DELE:
            /*Send the remove method*/
            res = wilddog_removeValue(client, multiple_removeValueFunc, (void*)p_mul_client);
            break;
        case MULTIPLETEST_CMD_ON:
            /*Observe on*/
            res = wilddog_addObserver(client, WD_ET_VALUECHANGE, multiple_addObserverFunc, (void*)p_mul_client);
            break;
		case MULTIPLETEST_CMD_OFF:
			res = wilddog_removeObserver(client, WD_ET_VALUECHANGE);
			break;
		case MULTIPLETEST_CMD_NON:
		default:
			break;
    }
    /*Delete the node*/
    wilddog_node_delete(p_head);
    /* fautl to request then do not getValue */
    if(res <0)
    	p_mul_client->d_sendFault = 1;
    	
    return res;
}
STATIC void multiple_trysync(void)
{
	/*Handle the event and callback function, it must be called in a special frequency*/
	while(1)
	{
		if(	
			(multipleClient[0].d_sendFault || multipleClient[0].d_recvFlag) &&
			(multipleClient[1].d_sendFault || multipleClient[1].d_recvFlag) &&
			(multipleClient[2].d_sendFault || multipleClient[2].d_recvFlag)  
			)
				break;
				
		wilddog_trySync();
	}
}

STATIC void multiple_testResultPrint(Multiple_client_T *p_mult_client)
{
	char url_temp[TESTURL_NUM];
	char *p_res;
	
	memset(url_temp,0,TESTURL_NUM);
	sprintf(url_temp,"%s%s%s",TESTURL_HARD,p_mult_client->p_host,TESTURL_TAIL);
	printf("@@testing: %s",url_temp);
	printf("\t set request:\t%s\n",MULTIPLETEST_GETRESULT(p_res,p_mult_client->d_setResult));
	printf("\t get request:\t%s\n",MULTIPLETEST_GETRESULT(p_res,p_mult_client->d_getResult));
	printf("\t observer request:\t%s\n",MULTIPLETEST_GETRESULT(p_res,p_mult_client->d_observerResult));
	printf("\t push request:\t%s\n",MULTIPLETEST_GETRESULT(p_res,p_mult_client->d_pushResult));
	printf("\t delete request:\t%s\n",MULTIPLETEST_GETRESULT(p_res,p_mult_client->d_deleteResult));
	
}
int main(void)
{
	
    Wilddog_T client1= 0,client2 = 0,client3 = 0;
	
	printf(" @@mutiple test\n");

	
	sprintf((char*)url1,"%s%s%s",TESTURL_HARD,MULTIPLE_HOST1,TESTURL_TAIL);
	sprintf((char*)url2,"%s%s%s",TESTURL_HARD,MULTIPLE_HOST2,TESTURL_TAIL);
	sprintf((char*)url3,"%s%s%s",TESTURL_HARD,MULTIPLE_HOST3,TESTURL_TAIL);
	
	printf("%s \n",url1);
	printf("%s \n",url2);
	printf("%s \n",url3);

	multipleClient[0].p_host = MULTIPLE_HOST1;
	multipleClient[1].p_host = MULTIPLE_HOST2;
	multipleClient[2].p_host = MULTIPLE_HOST3;
	
    client1 = wilddog_initWithUrl((Wilddog_Str_T *)url1);
    client2 = wilddog_initWithUrl((Wilddog_Str_T *)url2);
    client3 = wilddog_initWithUrl((Wilddog_Str_T *)url3);
    /* set */
    
	printf("test set \n");
	multipletest_request(MULTIPLETEST_CMD_SET,client1,&multipleClient[0]);
	//multipletest_request(MULTIPLETEST_CMD_SET,client2,&multipleClient[1]);
	//multipletest_request(MULTIPLETEST_CMD_SET,client3,&multipleClient[2]);

	multiple_trysync();
	printf("test get \n");
	while(1);
	/*get */
	multipletest_request(MULTIPLETEST_CMD_GET,client1,&multipleClient[0]);
	multipletest_request(MULTIPLETEST_CMD_GET,client2,&multipleClient[1]);
	multipletest_request(MULTIPLETEST_CMD_GET,client3,&multipleClient[2]);    
	multiple_trysync();
	/* ON */
	multipletest_request(MULTIPLETEST_CMD_ON,client1,&multipleClient[0]);
	multipletest_request(MULTIPLETEST_CMD_ON,client2,&multipleClient[1]);
	multipletest_request(MULTIPLETEST_CMD_ON,client3,&multipleClient[2]);
	multiple_trysync();	
	/*PUSH */
	multipletest_request(MULTIPLETEST_CMD_PUSH,client1,&multipleClient[0]);
	multipletest_request(MULTIPLETEST_CMD_PUSH,client2,&multipleClient[1]);
	multipletest_request(MULTIPLETEST_CMD_PUSH,client3,&multipleClient[2]);
	multiple_trysync();	
	/*DELETE */
	multipletest_request(MULTIPLETEST_CMD_DELE,client1,&multipleClient[0]);
	multipletest_request(MULTIPLETEST_CMD_DELE,client2,&multipleClient[1]);
	multipletest_request(MULTIPLETEST_CMD_DELE,client3,&multipleClient[2]);
	multiple_trysync();	
	
	/* OFF 
	multipletest_request(MULTIPLETEST_CMD_ON,client1,&multipleClient[0]);
	multipletest_request(MULTIPLETEST_CMD_ON,client2,&multipleClient[1]);
	multipletest_request(MULTIPLETEST_CMD_ON,client3,&multipleClient[2]);
	multiple_trysync();	
	*/
	/* result printf */
	multiple_testResultPrint(&multipleClient[0]);
	multiple_testResultPrint(&multipleClient[1]);
	multiple_testResultPrint(&multipleClient[2]);

	/* destory*/
    wilddog_destroy(&client1);
    wilddog_destroy(&client2);
    wilddog_destroy(&client3);

    
	return 0;
}
