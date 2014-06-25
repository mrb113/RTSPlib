#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define TYPE_REQUEST 0
#define TYPE_RESPONSE 1

/* Linked List to store the options */
typedef struct _OPTIONS_LIST {
	char *option; 
	char *content; 
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

int parseRTSPRequest(struct _RTSP_MESSAGE *msg, char *rtspMessage);
int createRtspResponse(struct _RTSP_MESSAGE *msg, char *protocol, int statusCode, char *statusString, int sequenceNumber, OPTION_ITEM *options);
int createRtspRequest(struct _RTSP_MESSAGE *msg, char *command, char *target, char *protocol, int sequenceNumber, OPTION_ITEM *options);
char * getOptionContent(OPTION_ITEM *list, char option);
char *generatePayload(OPTION_ITEM *options);
int startsWith(const char *s, const char *prefix);
void insertOption(OPTION_ITEM *head, OPTION_ITEM *opt);