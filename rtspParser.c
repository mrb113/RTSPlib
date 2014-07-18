#include "rtsp.h"

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
	POPTION_ITEM current;
	/* Initialize to 1 for null terminator */
	int count = 1; 
	/* Add the length of the protocol */
	count += strlen(msg->protocol);

	/* Add length of request-specific strings */
	if (msg->type == TYPE_REQUEST){
		count += strlen(msg->message.request.command);
		count += strlen(msg->message.request.target);
		/* Add 4 for the two spaces and \r\n*/
		count += 4;
	}
	/* Add length of response-specific strings */
	else {
		char *statusCodeStr = malloc(sizeof(int));
		sprintf(statusCodeStr, "%d", msg->message.response.statusCode);
		count += strlen(statusCodeStr);
		count += strlen(msg->message.response.statusString);
		/* Two spaces and \r\n */
		count += 4; 
	}
	/* Count the size of the options */
	current = msg->options;
	while (current != NULL){
		count += strlen(current->option);
		count += strlen(current->content);
		/* Add 5 because of :[space] and \r\n */
		count += 4; 
		current = current->next; 
	}
	/* Add 2 more for extra /r/n ending */
	count += 2;

	/* Add the length of the payload, if any */
	if (msg->payload != NULL)
		count += strlen(msg->payload);

	return count; 
}

/* Given an RTSP message string, parse it into an RTSP_MESSAGE struct */
int parseRtspMessage(PRTSP_MESSAGE msg, char *rtspMessage) {
	char *token, *protocol, *endCheck, *target, *statusStr, *command, *sequence, flag;
	char *content, *payload = NULL, *opt = NULL;
	int statusCode, sequenceNum, offset;
	int exitCode;
	POPTION_ITEM options = NULL;
	POPTION_ITEM newOpt; 
	char messageEnded = 0; 
	char *delim = " \r\n";
	char *end = "\r\n";
	char *messageEnd = "\r\n\r\n";
	char *optDelim = " :\r\n";
	char typeFlag = TOKEN_OPTION;

	char *messageBuffer = malloc((strlen(rtspMessage) + 1) * sizeof(*rtspMessage));
	if (messageBuffer == NULL) {
		exitCode = RTSP_ERROR_NO_MEMORY;
		goto ExitFailure;
	}
	/* Tokenize the message string */
	strcpy(messageBuffer, rtspMessage);	
	 
	/* Get the first token */
	token = strtok(messageBuffer, delim);
	if (token == NULL){
		exitCode = RTSP_ERROR_MALFORMED; 
		goto ExitFailure; 
	}

	/* The message is a response */
	if (startsWith(token, "RTSP")){
		flag = TYPE_RESPONSE; 
		protocol = token;

		/* Get the status code */
		token = strtok(NULL, delim); 
		statusCode = atoi(token);
		if (token == NULL){
			exitCode = RTSP_ERROR_MALFORMED; 
			goto ExitFailure;
		}

		/* Get the status string */
		statusStr = strtok(NULL, end);
		if (statusStr == NULL){
			exitCode = RTSP_ERROR_MALFORMED;
			goto ExitFailure; 
		}

		/* Request fields - we don't care about them here */
		target = NULL;
		command = NULL; 
	}
	/* The message is a request */
	else {		
		flag = TYPE_REQUEST; 
		command = token; 
		/* Get the target */
		target = strtok(NULL, delim);
		if (target == NULL){
			exitCode = RTSP_ERROR_MALFORMED;
			goto ExitFailure;
		}
		/* Get the protocol */
		protocol = strtok(NULL, delim);
		if (protocol == NULL){
			exitCode = RTSP_ERROR_MALFORMED;
			goto ExitFailure;
		}
		/* Response field - we don't care about it here */
		statusStr = NULL; 
	}
	/* Check that the protocol is valid */
	if (strcmp(protocol, "RTSP/1.0")){
		exitCode = RTSP_ERROR_MALFORMED; 
		goto ExitFailure; 
	}
	/* Parse remaining options */
	while (token != NULL){

		token = strtok(NULL, optDelim);
		if (token != NULL){			

			/* The token is an option */
			if (typeFlag == TOKEN_OPTION){ 
				opt = token;
			}
			/* The token is content */
			else { 
				content = token;
				/* Create a new node containing the option and content */
				newOpt = (POPTION_ITEM)malloc(sizeof(OPTION_ITEM));
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

				/* Check if we're at the end of the message portion */
				offset = &token[0] + strlen(token) + 1;
				endCheck = offset;

				/* The first \r is missing because it's been tokenized */ 
				if (startsWith(endCheck, "\n\r\n")){
					/* We've encountered the end of the message */
					messageEnded = 1; 
					/* The payload is the remainder of the message. If none, then payload = null */
					if (endCheck[3] != '\0')
						payload = &endCheck[3];

					break; 
				}
			}
		}
		typeFlag ^= 1; // flip the flag
	}
	/* If we never encountered the double CRLF, then the message is malformed! */
	if (!messageEnded){
		printf("Malformed");
		exitCode = RTSP_ERROR_MALFORMED; 
		goto ExitFailure; 
	}

	/* Get sequence number */
	sequence = getOptionContent(options, "CSeq"); 
	if (sequence != NULL) {
		sequenceNum = atoi(sequence);
	}
	else {
		sequenceNum = SEQ_INVALID;
	}
	/* Package the new parsed message into the struct */
	if (flag == TYPE_REQUEST){
		createRtspRequest(msg, messageBuffer, FLAG_ALLOCATED_MESSAGE_BUFFER | FLAG_ALLOCATED_OPTION_ITEMS, command, target, protocol, sequenceNum, options, payload);
	}
	else {
		createRtspResponse(msg, messageBuffer, FLAG_ALLOCATED_MESSAGE_BUFFER | FLAG_ALLOCATED_OPTION_ITEMS, protocol, statusCode, statusStr, sequenceNum, options, payload);
	}
	return RTSP_ERROR_SUCCESS;

	/* Cleanup in failure condition */
ExitFailure:
	if (options) {
		free(options);
	}
	if (messageBuffer) {
		free(messageBuffer);
	}
	return exitCode;
}

/* Create new RTSP message struct with response data */
void createRtspResponse(PRTSP_MESSAGE msg, char *message, int flags, char *protocol, 
	int statusCode, char *statusString, int sequenceNumber, POPTION_ITEM optionsHead, char *payload) {	
	msg->type = TYPE_RESPONSE;
	msg->flags = flags;
	msg->messageBuffer = message;
	msg->protocol = protocol;
	msg->options = optionsHead;
	msg->payload = payload; 
	msg->sequenceNumber = sequenceNumber;
	msg->message.response.statusString = statusString;
	msg->message.response.statusCode = statusCode;
}

/* Create new RTSP message struct with request data */
void createRtspRequest(PRTSP_MESSAGE msg, char *message, int flags, 
	char *command, char *target, char *protocol, int sequenceNumber, POPTION_ITEM optionsHead, char *payload) {
	msg->type = TYPE_REQUEST;
	msg->flags = flags;
	msg->protocol = protocol; 
	msg->messageBuffer = message;
	msg->options = optionsHead;
	msg->payload = payload;
	msg->sequenceNumber = sequenceNumber;
	msg->message.request.command = command; 
	msg->message.request.target = target;
}

/* Retrieves option content from the linked list given the option title */
char *getOptionContent(POPTION_ITEM optionsHead, char *option){
	OPTION_ITEM *current = optionsHead;
	while (current != NULL){
		/* Check if current node is what we're looking for */
		if (!strcmp(current->option, option)){
			return current->content; 
		}
		current = current->next;
	}
	/* Not found */
	return NULL;
}

/* Adds new option opt to the struct's option list */
void insertOption(POPTION_ITEM *optionsHead, POPTION_ITEM opt){
	OPTION_ITEM *current = *optionsHead;
	opt->next = NULL;

	/* Empty options list */
	if (*optionsHead == NULL){
		*optionsHead = opt;
		return; 
	}
	/* Traverse the list and insert the new option at the end */
	while (current != NULL){
		/* Check for duplicate option; if so, replace the option currently there */
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

/* Free every node in the message's option list */
void freeOptionList(POPTION_ITEM optionsHead){
	POPTION_ITEM current = optionsHead;
	POPTION_ITEM temp; 
	while (current != NULL){
		temp = current; 
		current = current->next;
		if (optionsHead->flags & FLAG_ALLOCATED_OPTION_FIELDS){
			free(temp->option);
			free(temp->content);
		}
		free(temp);
	}
}

/* Serialize the message struct into a string containing the RTSP message */
char *serializeRtspMessage(PRTSP_MESSAGE msg, int *serializedLength){
	int size = getMessageLength(msg);
	char *serializedMessage = malloc(size);
	POPTION_ITEM current = msg->options;
	char *statusCodeStr = malloc(sizeof(int));

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
		sprintf(statusCodeStr, "%d", msg->message.response.statusCode);
		strcat(serializedMessage, statusCodeStr);
		strcat(serializedMessage, " ");
		/* status str\r\n */ 
		strcat(serializedMessage, msg->message.response.statusString);
		strcat(serializedMessage, "\r\n");
	}
	/* option content\r\n */
	while (current != NULL){
		strcat(serializedMessage, current->option); 
		strcat(serializedMessage, ": ");
		strcat(serializedMessage, current->content);
		strcat(serializedMessage, "\r\n");
		current = current->next; 
	}
	*serializedLength = strlen(serializedMessage) + 1; 
	return serializedMessage;
}

/* Free everything in a msg struct */
void freeMessage(PRTSP_MESSAGE msg){
	/* If we've allocated the message buffer */
	if (msg->flags & FLAG_ALLOCATED_MESSAGE_BUFFER){
		free(msg->messageBuffer);
	}

	/* If we've allocated any option items*/
	if (msg->flags & FLAG_ALLOCATED_OPTION_ITEMS){
		freeOptionList(msg->options); 
	}
	free(msg);
}