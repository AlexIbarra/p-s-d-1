#include "soapH.h"
#include "imsService.nsmap"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>



// Listas globales de usuarios y mensajes
struct UsersList userlist;
struct MessageList messagelist;

/*### MACROS ###*/
#define BACKLOG (100)
#define OFFLINE 0
#define ONLINE 1
#define DELETED -1


int checkUsers(char *name);
int checkUsersState(char *name);
int loadUserList();
int loadMessageList();
void salir(int senal);


int main(int argc, char **argv) { 

	int m, s;
	struct soap soap;

	// Asigno el handler para el Ctrl + C
   	signal(SIGINT,salir);

   	// Inicializo la estructura soap
   	soap_init(&soap);

	if (argc < 2) {
    	printf("Usage: %s <port>\n",argv[0]); 
    	soap_serve(&soap); 
      	soap_destroy(&soap);
      	soap_end(&soap); 
		//exit(-1); 
  	}
  	else {
  		soap.send_timeout = 60; 
		soap.recv_timeout = 60; 
		soap.accept_timeout = 3600; // server stops after 1 hour of inactivity
		soap.max_keep_alive = 100;  // max keep-alive sequence
		void *process_request(void*); 
		struct soap *tsoap; 
		pthread_t tid; 
		int port = atoi(argv[1]); // first command-line arg is port
		SOAP_SOCKET m, s; 
		m = soap_bind(&soap, NULL, port, BACKLOG);

		if (!soap_valid_socket(m)){
			soap_print_fault(&soap, stderr);
			exit(1); 
		}

		fprintf(stderr, "Socket connection successful %d\n", m); 

		// Cargamos lus usuarios guardados en disco
		 loadUserList();

		// Cargamos la lista de mensajes 
		 loadMessageList();

		for (;;) { 

			s = soap_accept(&soap); 
			if (!soap_valid_socket(s)) { 
				if (soap.errnum) { 
				   soap_print_fault(&soap, stderr); 
				   exit(1); 
				} 
				fprintf(stderr, "server timed out\n"); 
				break; 
			}

			// gestiono la funcion que me pide el Cliente
			soap_serve(&soap);
			soap_end(&soap);


			tsoap = soap_copy(&soap); 
			if (!tsoap) 
				break; 
			pthread_create(&tid, NULL, (void*(*)(void*))process_request, (void*)tsoap); 
		} 
		 
		soap_done(&soap); 
		return 0; 
  	}


 //  return 0;
}





int loadUserList() {

	int numusers=0, i=0, j=0;
	int friends;
	char user[IMS_MAX_NAME_SIZE];
	struct User usertmp;
	int nread;


	FILE *fd = fopen("bbdd/users.txt","r+");

	while((nread = fread(&usertmp, sizeof(struct User), 1, fd)) > 0) {
		if(usertmp.state != DELETED) {
			userlist.users[numusers] = usertmp;
			printf("Leido usuario %s de la BBDD\n",userlist.users[numusers].name);
			numusers++;
		}
	}

	userlist.numusers = numusers;

	fclose(fd);

	return 1;
}

int loadMessageList() {

	int nummessages=0, i=0;
	char msg[IMS_MAX_MSG_SIZE];
	int nread;
	struct Message msgtmp;


	FILE *fd = fopen("bbdd/messages.txt","r+");

	while((nread = fread(&msgtmp, sizeof(struct Message), 1, fd)) > 0) {
		messagelist.messages[nummessages] = msgtmp;
		nummessages++;
	}

	messagelist.nummessages = nummessages;

	fclose(fd);

	return 1;
}

int checkUsers(char *name) {

	int i = 0;

	for(i=0; i < userlist.numusers; i++){
		
		if(strcmp(userlist.users[i].name,name)==0){
			//printf("Checkeado usuario %s\n", userlist.users[i].name);
			return i;
		}

	}
	return -1;
}

int checkUsersState(char *name) {

	int i = 0;

	
	if((i = checkUsers(name)) != -1){
		return userlist.users[i].state;
	}
	return -2;
}

int checkFriend(char *user, char *name) {

	int i = 0;

	int pos = checkUsers(user);

	/* Busco en la lista de amigos del usuario */
	for(i=0; i < userlist.users[pos].friends.numfriends; i++){
		
		/* Si se encuentra en mi lista de amigos, retorno la posicion */
		if(strcmp(userlist.users[pos].friends.friends[i],name)==0) {
			return i;
		}

	}

	/* Si no esta en la lista, retorno -1 */
	return -1;
}

void guardarUsuarios() {

	int i=0, j=0;

	FILE *fd = fopen("bbdd/users.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "Error open\n");
		exit(1);
	}

	fwrite(&userlist.users, sizeof(struct User), userlist.numusers, fd);

	fclose(fd);
}

void guardarMensajes() {

	int i=0, j=0;

	FILE *fd = fopen("bbdd/messages.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "Error open\n");
		exit(1);
	}

	fwrite(&messagelist.messages, sizeof(struct Message), messagelist.nummessages, fd);

	fclose(fd);
}

void listarAmigos(int pos, char *friends[]) {

	int i=0;

	printf("Listando amigos...\n");

	for(i=0; i<userlist.users[pos].friends.numfriends; i++) {
		//*friends[i] = userlist.users[pos].friends[i];
		printf("%s\n", userlist.users[pos].friends.friends[i]);
		//strcpy(friends[i], userlist.users[pos].friends[i]);
	}
	
	//*friends = (char*)userlist.users[pos].friends;

}



// Handler para la señal Cntrl + C 

void salir(int senal){

	int i;
	fflush (stdout);
	switch(senal){
		case SIGINT:
			printf("\nCerrando servidor...\n");

			/*for(i = 0; i< userlist.contador ; i++){
				if(userlist.listaUsuarios[i].estado == ONLINE){
					userlist.listaUsuarios[i].estado = OFFLINE;
				}
			}

			guardarListaUsuarios();*/
			guardarUsuarios();
			guardarMensajes();
			exit(1);
		break;
	}

}



void *process_request(void *soap) { 

   pthread_detach(pthread_self()); 
   soap_serve((struct soap*)soap); 
   soap_destroy((struct soap*)soap); 
   soap_end((struct soap*)soap); 
   soap_done((struct soap*)soap); 
   free(soap); 

   return NULL; 
}












/*###########-- FUNCIONES IMS --############*/



/*################### MESSAGES ######################*/
int ims__sendMessage (struct soap *soap, struct Message myMessage, int *result){

	printf ("Received by server: \n\tusername:%s \n\tmsg:%s\n", myMessage.emisor, myMessage.msg);
	return SOAP_OK;
}

int ims__receiveMessage (struct soap *soap, struct Message *myMessage){
	/*
		// Allocate space for the message field of the myMessage struct then copy it
		myMessage->msg = (xsd__string) malloc (IMS_MAX_MSG_SIZE);
		// Not necessary with strcpy since uses null-terminated strings
		// memset(myMessage->msg, 0, IMS_MAX_MSG_SIZE);
		strcpy (myMessage.msg, "Invoking the remote function receiveMessage simply retrieves this standard message from the server"); // always same msg

		// Allocate space for the name field of the myMessage struct then copy it
		myMessage.name = (xsd__string) malloc (IMS_MAX_NAME_SIZE);
		// Not necessary with strcpy since uses null-terminated strings
		// memset(myMessage->name, 0, IMS_MAX_NAME_SIZE);  
		strcpy(myMessage.name, "aServer");	
	*/
	return SOAP_OK;
}
/*###################################################*/






/*################### FRIENDS ######################*/
int ims__listFriends (struct soap *soap, char * user, struct Friends * friends, int * result) {
	int pos, i;

	if((pos = checkUsers(user)) != -1) {

		//listarAmigos(pos, friends);
		// for(i=0; i<userlist.users[pos].numfriends; i++) {
		// 	strcpy(friends[i], userlist.users[pos].friends[i]);
		// }
		// (*numfriends) = userlist.users[pos].numfriends;
		friends = (struct Friends *) malloc(sizeof(struct Friends));

		//*friends = userlist.users[pos].friends;

		for(i=0; i<userlist.users[pos].friends.numfriends; i++) {
			//friends->friends[i] = userlist.users[pos].friends.friends[i];
			strcpy(friends->friends[i], userlist.users[pos].friends.friends[i]);
		}

		friends->numfriends = userlist.users[pos].friends.numfriends;

		printf("Listando amigos de %s...\n", user);

		for(i=0; i<friends->numfriends; i++) {
			//*friends[i] = userlist.users[pos].friends[i];
			printf("%s\n", friends->friends[i]);
			//strcpy(friends[i], userlist.users[pos].friends[i]);
		}
		*result = 0;
	}

	return SOAP_OK;
}

int ims__newFriend (struct soap *soap, char * user, char * userfriend, int * result) {

	/* Compruebo que el usuario exista */
	int pos = checkUsers(userfriend);
	
	if (pos != -1){
		/* Compruebo si ya lo tengo como amigo */
		int pos = checkFriend(user, userfriend);
		int numfriends = userlist.users[pos].friends.numfriends;

		/* Si no esta añadido, lo añado */
		if(pos == -1) {
			pos = checkUsers(user);
			if(numfriends < IMS_MAX_FRIENDS) {
				strcpy(userlist.users[pos].friends.friends[numfriends], userfriend);
				userlist.users[pos].friends.numfriends++;
				(*result) = 0;
				//printf("Añadido amigo %s al usuario %s\n", userlist.users[pos].friends.friends[numfriends-1], user);
				printf("Añadido amigo %s al usuario %s\n", userfriend, user);
			}
			/* Informo de que no se pueden añadir mas amigos a la lista */
			else 
				(*result) = -1;
		}
		/* Si ya esta añadido, informo al cliente */
		else {
			(*result) = 1;
		}
	}
	else{
		//printf("El usuario %s no existe", userfriend);
		(*result) = -2;
	}


	return SOAP_OK;
}
/*#################################################*/






/*################### USERS ######################*/
int ims__newUser (struct soap *soap, char * user, int * result) {
	
	// Comprobamos que el usuario existe
	int estado = checkUsersState(user);
	int pos;

	/* El usuario ya existe y esta conectado */
	if(estado == ONLINE) {		
	
		/* Codigo ya existe el usuario */
		(*result) = 1;
	}/* El usuario ya existe y esta desconectado */
	else if(estado == OFFLINE) {

		/* Cambiamos el estado del usuario a conectado*/
		pos = checkUsers(user);
		userlist.users[pos].state = ONLINE;

		(*result) = 2;

	}/* El usuario fue eliminado. Se pregunta por reactivacion */
	else if(estado == DELETED) {

		/* Codigo usuario borrado */
		(*result) = -1;

	}/* El usuario no existe y hay hueco en la lista */
	else if(userlist.numusers < IMS_MAX_USERS) {

		strcpy(userlist.users[userlist.numusers].name, user);
		userlist.users[userlist.numusers].friends.numfriends = 0;
		userlist.users[userlist.numusers].state = OFFLINE;
		userlist.numusers++;

		printf("Añadido usuario %s\n", user);

		/* Codigo exito al añadir */
		(*result) = 0;

	}/* La lista de usuario esta llena e informamos */
	else {

		/* Codigo lista llena */
		(*result) = -2;
	}

	
	return SOAP_OK;
}

int ims__login (struct soap *soap, char * user, int * result) {

	/* Comprobamos la posicion del usuario en la lista */
	int pos = checkUsers(user);


	/* Si el usuario esta en la lista */
	if(pos != -1) {

		/* Ponemos el ussuario ONLINE */
		userlist.users[pos].state = ONLINE;
		printf("Usuario %s conectado\n", user);
		(*result) = 0;
	}
	/* Si el usuario no esta en la lista */
	else {

		(*result) = 1;
	}

	return SOAP_OK;
}

int ims__logout (struct soap *soap, char * user, int * result) {

}

int ims__listUsers (struct soap *soap, int * result) {
	(*result) = loadUserList();
	return SOAP_OK;
}

int ims__deleteUser (struct soap *soap, char * user, int * result) {

	// Comprobamos que el usuario existe
	if(checkUsers(user) >= 0) {
		userlist.users[userlist.numusers].state = DELETED;
		printf("Usuario %s eliminado\n", user);
	}
	else { 
		printf("El usuario %s no existe\n", &user);
	}
	
	return SOAP_OK;
}

int ims__reactivate(struct soap *soap, char * user, int * result) {

}
/*###############################################*/



