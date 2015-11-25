//gsoap ims service name: imsService
//gsoap ims service style: rpc
//gsoap ims service encoding: encoded
//gsoap ims service namespace: urn:imsService
#define IMS_MAX_MSG_SIZE 256
#define IMS_MAX_NAME_SIZE 80
#define IMS_MAX_FRIENDS 50

typedef char *xsd__string;


struct Message {
	xsd__string emisor;
	xsd__string receptor;
	xsd__string msg;
};

struct UserServer{
	xsd__string name;
	struct Message *rcvmsg;
	struct Message *sndmsg;
	xsd__string friends[];
};

int ims__sendMessage (struct Message myMessage, int *result);
int ims__receiveMessage (struct Message *myMessage);
int ims__newUser (struct User myUser, int *result);
int ims_comprobarUsuario(char * user, int *result);
int ims__deleteUser (struct User myUser, int *result);
