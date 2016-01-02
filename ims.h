//gsoap ims service name: imsService
//gsoap ims service style: rpc
//gsoap ims service encoding: encoded
//gsoap ims service namespace: urn:imsService
#define IMS_MAX_MSG_SIZE 280
#define IMS_MAX_NAME_SIZE 16
#define IMS_MAX_FRIENDS 50
#define IMS_MAX_USERS 100

typedef char *xsd__string;


struct Message {
	char emisor[16];
	char receptor[16];
	char msg[280];
	int state;
};

struct Friends {
	char friends[50][16];
	int numfriends;
};

struct Request {
	char emisor[16];
	char receptor[16];
	int state;
};

struct RequestList {
	struct Request request[50];
	int numrequest;
};

struct User {
	char name[16];
	struct Friends friends;
	struct RequestList requestlist;
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


/*########## MENSAJES ##########*/
int ims__sendMessage (struct Message myMessage, int * result);
int ims__receiveMessage (struct Message * myMessage);
/*##############################*/



/*########## USUARIOS ##########*/
int ims__newUser (char * user, int * result);
int ims__deleteUser (char * user, int * result);
int ims__login (char * user, int * result);
int ims__logout (char * user, int * result);
int ims__listUsers (int * result);
int ims__reactivate(char * user, int * result);
/*#############################*/



/*########## AMIGOS ##########*/
//int ims__listFriends (char * user, char *friends[], int *numfriend, int * result);
int ims__listFriends (char * user, struct Friends * friends, int * result);
int ims__listFriendRequest (char * user, struct RequestList * request, int * result);
int ims__newFriend (char * user, char * userfriend, int * result);
int ims__deleteFriend (char * user, char * userfriend, int * result);
/*############################*/