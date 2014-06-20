#include "rtsp.h"
#define TEST_REQUEST "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\nCSeq : 2"
#define TEST_RESPONSE "RTSP/1.0 200 OK\r\nCSeq: 5\r\nCSeq: 7\r\nSession : 12345678"

struct _RTSP_MESSAGE parseRTSPRequest(char *rtspMessage) {
	struct _RTSP_MESSAGE msg; 
	char *protocol, *statusStr, *token, *target, *command, flag;
	int statusCode;
	OPTION_ITEM *options = (OPTION_ITEM*)calloc(1, sizeof(OPTION_ITEM)); 
	if (!options){
		// TODO calloc failed, SOS
		// HELP speaking of, how should I handle exceptions? the rest of ll common c uses Limelog
	} 
	/* Tokenize the message string */
	// HELP the static ugly arrays are just sort of what I put as I was figuring out how things worked, how to make them die? 
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
		flag = TYPE_RESPONSE; 
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
		flag = TYPE_REQUEST; 
		command = first;
		if ((target = strtok(NULL, delim)) == NULL){
			// TODO Abort mission something is wrong
		}		
		if ((protocol = strtok(NULL, delim)) == NULL){
			// TODO Abort mission something is wrong
		}
	}

	if (strcmp(protocol, "RTSP/1.0")){
		// TODO Invalid protocol
	}

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
				OPTION_ITEM *newOpt = (OPTION_ITEM*)malloc(sizeof(OPTION_ITEM)); // HELP need to free this. help how do I linked list in C
				if (!newOpt){
					// TODO malloc failed, SOS
				}
				strcpy(newOpt->content, content);
				strcpy(newOpt->option, opt);
				newOpt->next = NULL; 
				insertOption(options, newOpt);
			}			
		}
		typeFlag ^= 1; // flip the flag
	}
	char *sequence = getOptionContent(options, "CSeq"); // Get the sequence #
	int sequenceNum = atoi(sequence);

	/* Package the new parsed message */
	if (flag == TYPE_REQUEST){
		msg = createRtspRequest(command, target, protocol, sequenceNum, options, 0);
	}
	else {
		msg = createRtspResponse(protocol, statusCode, statusStr, sequenceNum, options, 0);
	}
	return msg;
}

/* Create new RTSP message struct with response data */
struct _RTSP_MESSAGE createRtspResponse(char *protocol, int statusCode, char *statusString, int sequenceNumber, OPTION_ITEM *options, char *payload){
	struct _RTSP_MESSAGE msg;
	msg.type = TYPE_RESPONSE; 
	msg.options = options; 
	msg.payload = payload; 
	msg.protocol = protocol; 
	msg.sequenceNumber = sequenceNumber; 
	msg.statusCode = statusCode;
	msg.statusString = statusString;
	return msg; 
}

/* Create new RTSP message struct with request data */
// TODO silly Michelle, you don't return structs in C! 
struct _RTSP_MESSAGE createRtspRequest(char *command, char *target, char *protocol, int sequenceNumber, OPTION_ITEM *options, char *payload){
	struct _RTSP_MESSAGE msg;
	msg.command = command;
	msg.type = TYPE_REQUEST;
	msg.options = options;
	msg.payload = payload;
	msg.protocol = protocol;
	msg.sequenceNumber = sequenceNumber;
	msg.target = target;
	return msg; 
}

/* Retrieves option content from the linked list given the option title */
char *getOptionContent(OPTION_ITEM *list, char *option){
	OPTION_ITEM *current = list;
	while (current != NULL){
		if (!strcmp(current->option, option)){
			return current->content; 
		}
		current = current->next;
	}
	return NULL;
}

/* Adds new option to the struct's option list */
void insertOption(OPTION_ITEM *head, OPTION_ITEM *opt){ // HELP is this what you wanted for the linked list without the jank empty head? If not, you'll need to clarify more.
	if (strlen(head->option) == 0){
		strcpy(head->option, opt->option);
		strcpy(head->content, opt->content);
		head->next = NULL; 
		return; 
	}
	OPTION_ITEM *current = head;
	while (current != NULL){
		if (!strcmp(current->option, opt->option)){
			strcpy(current->content, opt->content);
			return; // Duplicate option found
		}
		if (!current->next){
			current->next = opt;
			return; 
		}
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