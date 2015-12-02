//gsoap ims service name: imsService
//gsoap ims service style: rpc
//gsoap ims service encoding: encoded
//gsoap ims service namespace: urn:imsService
#define IMS_MAX_MSG_SIZE 280
#define IMS_MAX_NAME_SIZE 16
#define IMS_MAX_FRIENDS 50

typedef char *xsd__string;


struct Message {
	char emisor[16];
	char receptor[16];
	char msg[280];
};

struct User {
	char name[16];
	char *friends[50];
	int numfriends;
	int state;
};

struct UsersList {
	struct User users[100];
	int numusers;
};

struct MessageList {
	struct Message messages[1000];
	int nummessages;
};



int ims__sendMessage (struct Message myMessage, int *result);
int ims__receiveMessage (struct Message *myMessage);
int ims__newUser (char * user, int * result);
int ims__deleteUser (char *user, int *result);
int ims__listFriends (char * user, char *friends[], int * result);
