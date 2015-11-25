//gsoap ns service name: ims
//gsoap ns service style: rpc
//gsoap ns service encoding: encoded
//gsoap ns service namespace: urn:ims

#define IMS_MAX_MSG_SIZE 256

typedef char *xsd__string;

struct Message{
	char name [20];
	char msg  [144];
};

struct User{
	char login[20];
	int estado;
};

struct ListaUsuarios{
	struct User listaUsuarios[50];
	int contador;
};

struct ListaMensajes 
{
	struct Message mensajes[100];
	int numMensajes;
};

int ims__login			(char * login, int * result);
int ims__logout			(char * login, int * result);
int ims__darAltaUsuario		(char * login, int * result);
int ims__darBajaUsuario		(char * login, char * user, int * result);
int ims__listarAmigos		(char * login, int * result, struct ListaUsuarios *listaAmigos);
int ims__anadirAmigo		(char * login, char * destino,  int * result);
int ims__tratarSolicitud	(char * login, char * solicitante,int * result);
int ims__listarPendientes	(char * login, int * result, struct ListaUsuarios *listaAmigos);
int ims__enviarMensaje 		(char * login, char * destino, char * msg, int * resul);
int ims__recibirMensajes	(char * login, struct ListaMensajes * listaMsg);
int ims__mensajesRecibidos 	(char * login, int * result);
