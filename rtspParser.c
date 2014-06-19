#include "rtsp.h"
#define TEST_REQUEST "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\nCSeq : 2"
#define TEST_RESPONSE "RTSP/1.0 200 OK\r\nCSeq: 5\r\nSession : 12345678"

struct _RTSP_MESSAGE parseRTSPRequest(char *rtspMessage) {
	struct _RTSP_MESSAGE msg; 
	char *protocol, *statusStr, *token, *target, *command, flag;
	int statusCode;
	optionItem *head = (optionItem*)malloc(sizeof(optionItem)); 

	/* Tokenize the message string */
	char message[RTSP_MAX_SIZE]; 
	strcpy(message, rtspMessage);
	char delim[] = " \r\n";
	char end[] = "\r\n"; 
	char optDelim[] = " :\r\n";
	 
	if ((token = strtok(message, delim)) == NULL){
		// TODO all is lost
	}
	char *first = token;

	/* The message is a response */
	if (startsWith(message, "RTSP")){
		flag = RESPONSE; 
		protocol = first;

		if((token = strtok(NULL, delim)) == NULL){
			// TODO Abort mission something is wrong
		}
		statusCode = atoi(token);

		if ((statusStr = strtok(NULL, end)) == NULL){
			// TODO Abort mission something is wrong
		}
	}

	/* The message is a request */
	else {		
		flag = REQUEST; 
		command = first;
		if ((target = strtok(NULL, delim)) == NULL){
			// TODO Abort mission something is wrong
		}		
		if ((protocol = strtok(NULL, delim)) == NULL){
			// TODO Abort mission something is wrong
		}
	}

	if (!strcmp(protocol, "RTSP/1.0")){
		// TODO Invalid protocol
	}

	/* Create blank head node */
	strcpy(head->content, "");
	strcpy(head->option, "");
	head->next = 0; 
	optionItem options = *head;
	free(head);

	/* Parse remaining options */
	char typeFlag = 0; // 0 for option, 1 for content
	while (token != NULL){
		token = strtok(NULL, optDelim);
		char *opt, *content; 

		if (token != NULL){			
			if (typeFlag == 0){ // Option
				opt = token;
			} else { // Content
				content = token; 
				// ship off the node
				optionItem *newOpt = (optionItem*)malloc(sizeof(optionItem)); // TODO free this. help how d
				strcpy(newOpt->content, content);
				strcpy(newOpt->option, opt);
				newOpt->next = 0; 
				insertOption(&options, newOpt);
			}			
		}
		typeFlag ^= 1; // flip the flag
	}
	char *sequence = getOptionContent(&options, "CSeq"); 
	int sequenceNum = atoi(sequence);

	/* Package the new parsed message */
	if (flag == REQUEST){
		msg = createRtspRequest(command, target, protocol, sequenceNum, &options, 0);
	}
	else {
		msg = createRtspResponse(protocol, statusCode, statusStr, sequenceNum, &options, 0);
	}
	return msg;
}

/* Create new RTSP message struct with response data */
struct _RTSP_MESSAGE createRtspResponse(char *protocol, int statusCode, char *statusString, int sequenceNumber, optionItem *options, char *payload){
	struct _RTSP_MESSAGE msg;
	msg.flag = RESPONSE; 
	msg.options = options; 
	msg.payload = payload; 
	msg.protocol = protocol; 
	msg.sequenceNumber = sequenceNumber; 
	msg.statusCode = statusCode;
	msg.statusString = statusString;
	return msg; 
}

/* Create new RTSP message struct with request data */
struct _RTSP_MESSAGE createRtspRequest(char *command, char *target, char *protocol, int sequenceNumber, optionItem *options, char *payload){
	struct _RTSP_MESSAGE msg;
	msg.command = command;
	msg.flag = REQUEST;
	msg.options = options;
	msg.payload = payload;
	msg.protocol = protocol;
	msg.sequenceNumber = sequenceNumber;
	msg.target = target;
	return msg; 
}

/* Retrieves option content from the linked list given the option title */
char *getOptionContent(optionItem *list, char *option){
	optionItem *current = list;
	char optionContent[RTSP_MAX_SIZE]; 
	while (current != NULL){
		if (!strcmp(current->option, option)){
			strcpy(&optionContent, current->content); 
		}
		current = current->next;
	}
	return optionContent;
}

/* Adds new option to the struct's option list */
void insertOption(optionItem *head, optionItem *opt){
	optionItem *current = head;
	while (current->next != NULL){
		if (!strcmp(current->option, opt->option)){
			return; // Duplicate option found
		}
		current = current->next;
	}
	current->next = opt;
}

void printOptions(optionItem *head){
	if (head->next == NULL){
		printf("Empty list\n");
		return; 
	}
	optionItem *current = head->next; 
	while (current != NULL){
		printf("Option: %s, Content: %s\n", current->option, current->content);
		current = current->next;
	}
}

/* Check if String s begins with the given prefix */
bool startsWith(const char *s, const char *prefix) {
	if (strncmp(s, prefix, strlen(prefix)) == 0){
		return 1; 
	}
	else {
		return 0; 
	}
}

int main() {
	parseRTSPRequest(TEST_RESPONSE);
	printf("Done");
	getchar(0);
}