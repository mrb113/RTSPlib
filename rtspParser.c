#include "rtsp.h"
#define TEST_REQUEST "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\nCSeq : 2"
#define TEST_RESPONSE "RTSP/1.0 200 OK\r\nCSeq: 5\r\nCSeq: 7\r\nSession : 12345678"

//  
int parseRTSPRequest(struct _RTSP_MESSAGE *msg, char *rtspMessage) {
	char *token, *protocol, *target, *statusStr, *command, flag;
	int statusCode;
	OPTION_ITEM *options = (OPTION_ITEM*)calloc(1, sizeof(OPTION_ITEM)); 	
	char *message = malloc((strlen(rtspMessage) + 1) * sizeof(*rtspMessage));
	if (options == NULL || message == NULL){
		goto ExitFailure;
	}
	/* Tokenize the message string */
	strcpy(message, rtspMessage);
	char* delim = " \r\n";
	char* end = "\r\n"; 
	char* optDelim = " :\r\n";
	 
	token = strtok(message, delim);

	/* The message is a response */
	if (startsWith(message, "RTSP")){
		flag = TYPE_RESPONSE; 
		protocol = token;
		token = strtok(NULL, delim); 
		statusCode = atoi(token);
		statusStr = strtok(NULL, end);

		command = NULL; 
		target = NULL; 
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
		// TODO Invalid protocol
	}

	/* Parse remaining options */
	char typeFlag = 0; // 0 for option, 1 for content
	char *content, *opt = malloc((strlen(message) + 1) * sizeof(*message));
	while (token != NULL){
		token = strtok(NULL, optDelim);
		if (token != NULL){
			if (typeFlag == 0){ // Option
				strcpy(opt, token);
			}
			else { // Content
				content = token;
				// ship off the node
				OPTION_ITEM *newOpt = (OPTION_ITEM*)malloc(sizeof(OPTION_ITEM));
				if (newOpt == NULL){
					// TODO  
				}
				newOpt->option = malloc((strlen(opt) + 1) * sizeof(*opt));
				if (newOpt->option == NULL){
					// TODO 
				}
				newOpt->content = malloc((strlen(content) + 1) * sizeof(*content));
				if (newOpt->content == NULL){
					// TODO 
				}
				strcpy(newOpt->option, opt);
				strcpy(newOpt->content, content);
				newOpt->next = NULL;
				insertOption(options, newOpt);		
			}
		}
		typeFlag ^= 1; // flip the flag
	}
	char *sequence = getOptionContent(options, "CSeq"); // Get the sequence #
	if (sequence == NULL){
		goto ExitFailure; 
	}
	int sequenceNum = atoi(sequence);
	/* Package the new parsed message */
	if (flag == TYPE_REQUEST){
		createRtspRequest(msg, command, target, protocol, sequenceNum, options);
	}
	else {
		createRtspResponse(msg, protocol, statusCode, statusStr, sequenceNum, options);
	}
	return 1;

ExitFailure:
	if (options) {
		free(options);
	}
	if (message) {
		free(message);
	}
	return 0;
}

/* Create new RTSP message struct with response data */
int createRtspResponse(struct _RTSP_MESSAGE *msg, char *protocol, int statusCode, char *statusString, int sequenceNumber, OPTION_ITEM *options){
	
	msg->protocol = malloc((strlen(protocol) + 1) * sizeof(*protocol));
	if (msg->protocol == NULL) {
		goto ExitFailure;
	}
	msg->statusString = malloc((strlen(statusString) + 1) * sizeof(*statusString));
	if (msg->statusString == NULL) {
		goto ExitFailure;
	}
	msg->options = (OPTION_ITEM*)malloc(sizeof(*options));
	if (msg->options == NULL) {
		goto ExitFailure;
	}
	strcpy(msg->protocol, protocol);	
	msg->type = TYPE_RESPONSE; 
	msg->options = options;
	msg->sequenceNumber = sequenceNumber; 
	msg->statusCode = statusCode;
	strcpy(msg->statusString, statusString);

	return 1;

ExitFailure:
	if (msg->protocol) {
		free(msg->protocol);
	}

	return 0;
}

/* Create new RTSP message struct with request data */
int createRtspRequest(struct _RTSP_MESSAGE *msg, char *command, char *target, char *protocol, int sequenceNumber, OPTION_ITEM *options){
	msg->protocol = malloc((strlen(protocol) + 1) * sizeof(*protocol));
	if (msg->protocol == NULL) {
		goto ExitFailure;
	}
	msg->command = malloc((strlen(command) + 1) * sizeof(*command));
	if (msg->command == NULL) {
		goto ExitFailure;
	}
	msg->target = malloc((strlen(target) + 1) * sizeof(*target));
	if (msg->target == NULL) {
		goto ExitFailure;
	}
	msg->options = (OPTION_ITEM*)malloc(sizeof(*options));
	if (msg->options == NULL) {
		goto ExitFailure;
	}

	strcpy(msg->command, command);
	msg->type = TYPE_REQUEST;
	msg->options = options;
	strcpy(msg->protocol, protocol);
	msg->sequenceNumber = sequenceNumber;
	strcpy(msg->target, target);

	return 1; 

ExitFailure:
	if (msg->protocol) {
		free(msg->protocol);
	}
	if (msg->command) {
		free(msg->command);
	}
	if (msg->target) {
		free(msg->target);
	}
	if (msg->options) {
		free(msg->options);
	}

	return 0;
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
void insertOption(OPTION_ITEM *head, OPTION_ITEM *opt){ 
	if (head->option == NULL){
		head->option = malloc((strlen(opt->option) + 1) * sizeof(*opt->option));
		if (head->option == NULL){
			// TODO failure
		}
		head->content = malloc((strlen(opt->content) + 1) * sizeof(*opt->content));
		if (head->content == NULL){
			// TODO failure
		}
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
		if (current->next == NULL){
			current->next = opt;
			return; 
		}
		current = current->next; 
	}	
}

/* Check if String s begins with the given prefix */
int startsWith(const char *s, const char *prefix) {
	if (strncmp(s, prefix, strlen(prefix)) == 0){
		return 1; 
	}
	else {
		return 0;
	}
}

int main() {
	struct _RTSP_MESSAGE msg;
	parseRTSPRequest(&msg, TEST_REQUEST);
	printf("Done");
	getchar(0);
}