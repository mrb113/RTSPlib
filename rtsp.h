#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define REQUEST 0
#define RESPONSE 1
#define RTSP_MAX_SIZE 10000 // Magic #, TODO come up with a reasonable one

/* Linked List to store the options */
struct _OPTIONS_LIST {
	char option[RTSP_MAX_SIZE]; 
	char content[RTSP_MAX_SIZE]; 
	struct _OPTIONS_LIST *next; 
};
typedef struct _OPTIONS_LIST optionItem;

/* RTSP Message *
* In this implementation, a flag indicates the message type:
* 0 = request
* 1 = response */
struct _RTSP_MESSAGE {
	char flag;
	int sequenceNumber;
	char *protocol;
	optionItem *options;

	// Request fields
	char *command;
	char *target;

	// Response fields
	char *payload;
	char *statusString;
	int statusCode;
};

struct _RTSP_MESSAGE parseRTSPRequest(char *rtspMessage);
struct _RTSP_MESSAGE createRtspResponse(char *protocol, int statusCode, char *statusString, int sequenceNumber, optionItem *options, char *payload);
struct _RTSP_MESSAGE createRtspRequest(char *command, char *target, char *protocol, int sequenceNumber, optionItem *options, char *payload);
char * getOptionContent(optionItem *list, char option);
bool startsWith(const char *s, const char *prefix);
void insertOption(optionItem *head, optionItem *opt);
void printOptions(optionItem *head);