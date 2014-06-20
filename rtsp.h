#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define TYPE_REQUEST 0
#define TYPE_RESPONSE 1
#define RTSP_MAX_SIZE 10000 // HELP can you clarify how you want to dynamically allocate the option? 

/* Linked List to store the options */
typedef struct _OPTIONS_LIST {
	char option[RTSP_MAX_SIZE]; 
	char content[RTSP_MAX_SIZE]; 
	struct _OPTIONS_LIST *next; 
} OPTION_ITEM;

/* RTSP Message *
* In this implementation, a flag indicates the message type:
* 0 = request
* 1 = response */
struct _RTSP_MESSAGE { // TODO the workers must unionize (that is, make a union)
	char type;
	int sequenceNumber;
	char *protocol;
	OPTION_ITEM *options;

	// Request fields
	char *command;
	char *target;

	// Response fields
	char *payload;
	char *statusString;
	int statusCode;
};

struct _RTSP_MESSAGE parseRTSPRequest(char *rtspMessage);
struct _RTSP_MESSAGE createRtspResponse(char *protocol, int statusCode, char *statusString, int sequenceNumber, OPTION_ITEM *options, char *payload);
struct _RTSP_MESSAGE createRtspRequest(char *command, char *target, char *protocol, int sequenceNumber, OPTION_ITEM *options, char *payload);
char * getOptionContent(OPTION_ITEM *list, char option);
bool startsWith(const char *s, const char *prefix);
void insertOption(OPTION_ITEM *head, OPTION_ITEM *opt);