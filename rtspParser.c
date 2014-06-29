#include "rtsp.h"
#define TEST_REQUEST "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\nCSeq : 2"
#define TEST_RESPONSE "RTSP/1.0 200 OK\r\nCSeq: 5\r\nCSeq: 7\r\nSession : 12345678"

/* Check if String s begins with the given prefix */
static int startsWith(const char *s, const char *prefix) {
	if (strncmp(s, prefix, strlen(prefix)) == 0){
		return 1;
	}
	else {
		return 0;
	}
}

/* Gets the length of the message */
static int getMessageLength(PRTSP_MESSAGE msg){
	int count = 1; 
	count += strlen(msg->protocol);
	if (msg->type == TYPE_REQUEST){
		count += strlen(msg->message.request.command);
		count += strlen(msg->message.request.target);
		/* Add 2 for the two spaces */
		count += 2;
	}
	else {
		char *statusCodeStr = malloc(sizeof(int));
		sprintf(statusCodeStr, "%d", msg->message.response.statusCode);
		count += strlen(statusCodeStr);
		count += strlen(msg->message.response.statusString);
		/* Two spaces and \r\n */
		count += 4; 
	}

	POPTION_ITEM current = msg->options; 
	while (current != NULL){
		count += strlen(msg->options->option);
		count += strlen(msg->options->content);
		/* Add 4 because of : and \r\n */
		count += 4; 
		current = current->next; 
	}
	return count; 
}

int parseRTSPRequest(PRTSP_MESSAGE msg, char *rtspMessage) {
	char *token, *protocol, *target, *statusStr, *command, flag;
	int statusCode;
	int exitCode;
	POPTION_ITEM options = NULL;
	char *message = malloc((strlen(rtspMessage) + 1) * sizeof(*rtspMessage));
	if (message == NULL) {
		exitCode = RTSP_ERROR_NO_MEMORY;
		goto ExitFailure;
	}
	/* Tokenize the message string */
	strcpy(message, rtspMessage);
	char* delim = " \r\n";
	char* end = "\r\n"; 
	char* optDelim = " :\r\n";
	 
	token = strtok(message, delim);

	/* The message is a response */
	if (startsWith(token, "RTSP")){
		flag = TYPE_RESPONSE; 
		protocol = token;

		/* Get the status code */
		token = strtok(NULL, delim); 
		statusCode = atoi(token);

		/* Get the status string */
		statusStr = strtok(NULL, end);

		target = NULL;
		command = NULL; 
	}
	/* The message is a request */
	else {		
		flag = TYPE_REQUEST; 
		command = token; 
		target = strtok(NULL, delim);
		protocol = strtok(NULL, delim);

		statusStr = NULL; 
	}

	if (strcmp(protocol, "RTSP/1.0")){
		exitCode = RTSP_ERROR_MALFORMED; 
		goto ExitFailure; 
	}

	/* Parse remaining options */
	char typeFlag = 0x0; // 0 for option, 1 for content
	char *content, *opt = NULL;
	while (token != NULL){
		token = strtok(NULL, optDelim);
		if (token != NULL){
			if (typeFlag == 0){ // Option
				opt = token;
			}
			else { // Content
				content = token;
				// ship off the node
				OPTION_ITEM *newOpt = (OPTION_ITEM*)malloc(sizeof(OPTION_ITEM));
				if (newOpt == NULL){
					freeOptionList(options);
					exitCode = RTSP_ERROR_NO_MEMORY;
					goto ExitFailure;
				}
				newOpt->flags = 0;
				newOpt->option = opt;
				newOpt->content = token;
				newOpt->next = NULL;
				insertOption(&options, newOpt);		
			}
		}
		typeFlag ^= 0x1; // flip the flag
	}
	int sequenceNum;
	char *sequence = getOptionContent(options, "CSeq"); // Get the sequence #
	if (sequence != NULL) {
		sequenceNum = atoi(sequence);
	}
	else {
		sequenceNum = SEQ_INVALID;
	}
	/* Package the new parsed message */
	if (flag == TYPE_REQUEST){
		createRtspRequest(msg, message, FLAG_ALLOCATED_MESSAGE | FLAG_ALLOCATED_OPTION_ITEMS, command, target, protocol, sequenceNum, options);
	}
	else {
		createRtspResponse(msg, message, FLAG_ALLOCATED_MESSAGE | FLAG_ALLOCATED_OPTION_ITEMS, protocol, statusCode, statusStr, sequenceNum, options);
	}
	puts(serializeRtspMessage(msg)); 
	return RTSP_ERROR_SUCCESS;

ExitFailure:
	if (options) {
		free(options);
	}
	if (message) {
		free(message);
	}
	return exitCode;
}

/* Create new RTSP message struct with response data */
void createRtspResponse(PRTSP_MESSAGE msg, char *message, int flags, char *protocol, 
	int statusCode, char *statusString, int sequenceNumber, POPTION_ITEM optionsHead) {
	
	msg->type = TYPE_RESPONSE;
	msg->flags = flags;
	msg->messageBuffer = message;
	msg->protocol = protocol;
	msg->options = optionsHead;
	msg->sequenceNumber = sequenceNumber;
	msg->message.response.statusString = statusString;
	msg->message.response.statusCode = statusCode;
}

/* Create new RTSP message struct with request data */
void createRtspRequest(PRTSP_MESSAGE msg, char *message, int flags, 
	char *command, char *target, char *protocol, int sequenceNumber, POPTION_ITEM optionsHead) {
	msg->type = TYPE_REQUEST;
	msg->flags = flags;
	msg->protocol = protocol; 
	msg->messageBuffer = message;
	msg->options = optionsHead;
	msg->sequenceNumber = sequenceNumber;
	msg->message.request.command = command; 
	msg->message.request.target = target;
}

/* Retrieves option content from the linked list given the option title */
char *getOptionContent(POPTION_ITEM optionsHead, char *option){
	OPTION_ITEM *current = optionsHead;
	while (current != NULL){
		if (!strcmp(current->option, option)){
			return current->content; 
		}
		current = current->next;
	}
	return NULL;
}

/* Adds new option opt to the struct's option list */
void insertOption(POPTION_ITEM *optionsHead, POPTION_ITEM opt){
	opt->next = NULL;
	/* Empty options list */
	if (*optionsHead == NULL){
		*optionsHead = opt;
		return; 
	}
	OPTION_ITEM *current = *optionsHead;
	while (current != NULL){
		/* Check for duplicate option */
		if (!strcmp(current->option, opt->option)){
			current->content = opt->content;
			return;
		}
		if (current->next == NULL){
			current->next = opt;
			return;
		}
		current = current->next;
	}
}

/* Free the entire option list */
void freeOptionList(POPTION_ITEM optionsHead){
	OPTION_ITEM *current = optionsHead; 
	OPTION_ITEM *temp; 
	while (current != NULL){
		temp = current; 
		current = current->next; 
		free(temp);
	}
	current = NULL; 
}

char *serializeRtspMessage(PRTSP_MESSAGE msg, int *serializedLength){
	int size = getMessageLength(msg);
	char *serializedMessage = malloc(size);
	if (msg->type == TYPE_REQUEST){
		/* command [space] */
		strcpy(serializedMessage, msg->message.request.command); 
		strcat(serializedMessage, " ");
		/* target [space] */
		strcat(serializedMessage, msg->message.request.target);
		strcat(serializedMessage, " ");
		/* protocol \r\n */ 
		strcat(serializedMessage, msg->protocol);
		strcat(serializedMessage, "\r\n");		 
	} else {
		/* protocol [space] */ 
		strcpy(serializedMessage, msg->protocol);
		strcat(serializedMessage, " ");
		/* status code [space] */ 
		char *statusCodeStr = malloc(sizeof(int));
		sprintf(statusCodeStr, "%d", msg->message.response.statusCode);
		strcat(serializedMessage, statusCodeStr);
		strcat(serializedMessage, " ");
		/* status str\r\n */ 
		strcat(serializedMessage, msg->message.response.statusString);
		strcat(serializedMessage, "\r\n");
	}
	/* option content\r\n */
	POPTION_ITEM current = msg->options;
	while (current != NULL){
		strcat(serializedMessage, current->option); 
		strcat(serializedMessage, ": ");
		strcat(serializedMessage, current->content);
		strcat(serializedMessage, "\r\n");
		current = current->next; 
	}
	serializedLength = strlen(serializedMessage) + 1; 
	return serializedMessage;
}

void freeMessage(PRTSP_MESSAGE msg){
	
}

int main() {
	struct _RTSP_MESSAGE msg;
	parseRTSPRequest(&msg, TEST_RESPONSE);
	printf("Done");
	getchar(0);
}