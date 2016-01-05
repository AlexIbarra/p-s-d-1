//gsoap ims service name: imsService
//gsoap ims service style: rpc
//gsoap ims service encoding: encoded
//gsoap ims service namespace: urn:imsService
#define IMS_MAX_MSG_SIZE 280
#define IMS_MAX_NAME_SIZE 16
#define IMS_MAX_FRIENDS 50
#define IMS_MAX_USERS 100

typedef char *xsd__string;




struct Friend {
	char friends[16]; 	// nombre del amigo
	int state; 			// estado de la amistad (aceptada/pendiente/eliminada) --> mas facil para borrar amigos
};

struct ListFriends {
	struct Friend listfriends[50]; 	// lista de amigos (50 max)
	int numfriends;					// numeros de amigos hasta el momento
	int result;			// resultado de las operaciones que se realicen sobre la estructura (para informar al cliente)
};

struct Request {
	char emisor[16];
	char receptor[16];
	int state;
};

struct RequestList {
	struct Request request[500];
	int numrequest;
	int result;	// resultado de las operaciones que se realicen sobre la estructura (para informar al cliente)
};

struct User {
	char name[16];
	struct ListFriends fList;
	int state;
};

struct UsersList {
	struct User users[100];
	int numusers;
};

struct Message {
	char emisor[16];
	char receptor[16];
	char msg[280];
	int state;
};

struct MessageList {
	struct Message messages[1000];
	int nummessages;
	int result;	// resultado de las operaciones que se realicen sobre la estructura (para informar al cliente)
};


/*########## MENSAJES ##########*/
int ims__sendMessage (struct Message myMessage, int * result);
int ims__receiveMessage (char * user, int * state, struct MessageList * myListMessage);
/*##############################*/



/*########## USUARIOS ##########*/
int ims__newUser (char * user, int * result);
int ims__deleteUser (char * user, int * result);
int ims__login (char * user, int * result);
int ims__logout (char * user, int * result);
int ims__reactivate(char * user, int * result);
/*#############################*/



/*########## AMIGOS ##########*/
int ims__listFriends (char * user, struct ListFriends * friends);
int ims__listFriendRequest (char * user, struct RequestList * lRequest);
int ims__aceptRequest(char * user, struct Request request, int * result);
int ims__rejectRequest (char * user, struct Request request, int * result);
int ims__newFriend (char * user, char * userfriend, int * result);
int ims__deleteFriend (char * user, char * userfriend, int * result);
/*############################*/
