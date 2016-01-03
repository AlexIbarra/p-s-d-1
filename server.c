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

// USUARIOS
#define OFFLINE 0
#define ONLINE 1
#define DELETED -1

// MENSAJES
#define SEND 0
#define READ 1




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






/*################### GESTION BBDD ######################*/
int loadUserList() {

	printf("---##### TRACE loadUserList #####---\n");

	int numusers=0, i=0, j=0;
	int friends;
	char user[IMS_MAX_NAME_SIZE];
	struct User usertmp;
	int nread;


	FILE *fd = fopen("bbdd/users.txt","r+");

	while((nread = fread(&usertmp, sizeof(struct User), 1, fd)) > 0) {
		//if(usertmp.state != DELETED) {
		userlist.users[numusers] = usertmp;
		printf("Leido usuario %s con estado %d\n",userlist.users[numusers].name, userlist.users[numusers].state);
		numusers++;
		//}
	}

	userlist.numusers = numusers;

	fclose(fd);

	printf("---###############################---\n");

	return 1;
}

void guardarUsuarios() {

	printf("\t---##### TRACE guardarUsuarios #####---\n");

	int i=0, j=0;

	FILE *fd = fopen("bbdd/users.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "Error open\n");
		exit(1);
	}

	for(i=0; i<userlist.numusers; i++) {
		printf("\tGuardando usuario %s, estado %d\n",userlist.users[i].name, userlist.users[i].state);
		fwrite(&userlist.users[i], sizeof(struct User), 1, fd);	
	}
	

	//fwrite(&userlist.users, sizeof(struct User), userlist.numusers, fd);

	fclose(fd);

	printf("\t---###############################---\n");
}

int loadMessageList() {

	printf("---##### TRACE loadMessageList #####---\n");

	int nummessages=0, i=0;
	char msg[IMS_MAX_MSG_SIZE];
	int nread;
	struct Message msgtmp;


	FILE *fd = fopen("bbdd/messages.txt","r+");

	while((nread = fread(&msgtmp, sizeof(struct Message), 1, fd)) > 0) {
		messagelist.messages[nummessages] = msgtmp;
		printf("\t Leyendo emisor %s, receptor %s, mensaje %s, estado %d\n",msgtmp.emisor, msgtmp.receptor, msgtmp.msg, msgtmp.state);
		nummessages++;
	}

	messagelist.nummessages = nummessages;

	fclose(fd);

	printf("---###############################---\n");

	return 1;
}

void guardarMensajes() {

	printf("\t---##### TRACE guardarMensajes #####---\n");

	int i=0, j=0;

	FILE *fd = fopen("bbdd/messages.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "\tError open\n");
		exit(1);
	}

	for(i=0; i<messagelist.nummessages; i++) {
		printf("\tGuardando emisor %s, receptor %s, mensaje %s, estado %d\n",messagelist.messages[i].emisor, messagelist.messages[i].receptor, messagelist.messages[i].msg, messagelist.messages[i].state);
		fwrite(&messagelist.messages[i], sizeof(struct Message), 1, fd);	
	}

	//fwrite(&messagelist.messages, sizeof(struct Message), messagelist.nummessages, fd);

	fclose(fd);

	printf("\t---###############################---\n");
}
/*#######################################################*/




// Handler para la señal Cntrl + C 
void salir(int senal){

	printf("---##### TRACE salir #####---\n");

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

	printf("---###############################---\n");
			exit(1);
		break;
	}

}

int checkUsers(char *name) {

	printf("\t---##### TRACE checkUsers #####---\n");

	int i = 0;
	int pos = -1;
	int encontrado=0;

	while(i < userlist.numusers && !encontrado){
		printf("\tCheckeado usuario %s\n", userlist.users[i].name);
		if(strcmp(userlist.users[i].name,name)==0){
			printf("\tEncontrado usuario --> %s\n", userlist.users[i].name);
			pos = i;
			encontrado = 1;
		}
		i++;
	}

	printf("\t---###############################---\n");

	return pos;
}

int checkUsersState(char *name) {

	printf("\t---##### TRACE checkUsersState #####---\n");

	int i = 0;
	int state = -2;

	printf("\t");
	
	if((i = checkUsers(name)) != -1){
		state =  userlist.users[i].state;
	}

	printf("\t---###############################---\n");

	return state;
}

int checkFriend(char *user, char *name) {

	printf("\t---##### TRACE checkFriend #####---\n");

	printf("\t");

	int i = 0;
	int tmp = -1;
	int pos = checkUsers(user);
	int encontrado=0;

	/* Busco en la lista de amigos del usuario */
	while(i< userlist.users[pos].friends.numfriends && !encontrado){
		
		/* Si se encuentra en mi lista de amigos, retorno la posicion */
		if(strcmp(userlist.users[pos].friends.friends[i],name)==0) {
			printf("Encontrado amigo %s\n", userlist.users[pos].friends.friends[i]);
			tmp = i;
			encontrado = 1;
		}
		i++;
	}

	printf("\t---###############################---\n");

	/* Si no esta en la lista, retorno -1 */
	return tmp;
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

	printf("---##### TRACE ims__sendMessage #####---\n");

	printf ("Received by server: \n\temisor:%s \n\treceptor:%s \n\tmsg:%s\n", myMessage.emisor, myMessage.receptor, myMessage.msg);

	/* Compruebo que el usuario exista */
	int pos = checkUsers(myMessage.receptor);
	
	/* Si el usuario existe */
	if (pos != -1) {

		/* Copiamos los datos a la lista de mensajes */
		strcpy(messagelist.messages[messagelist.nummessages].emisor, myMessage.emisor);
		strcpy(messagelist.messages[messagelist.nummessages].receptor, myMessage.receptor);
		strcpy(messagelist.messages[messagelist.nummessages].msg, myMessage.msg);
		messagelist.messages[messagelist.nummessages].state = SEND;
		messagelist.nummessages++;
		(*result) = 0;
	}
	else {
		(*result) = -1;
	}

	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__receiveMessage (struct soap *soap, char * user, struct MessageList * myListMessage, int *result){
	


	return SOAP_OK;
}
/*###################################################*/






/*################### FRIENDS ######################*/
int ims__listFriends (struct soap *soap, char * user, struct Friends * friends, int * result) {

	printf("---##### TRACE ims__listFriends #####---\n");

	int pos, i;

	if((pos = checkUsers(user)) != -1) {

		friends = (struct Friends *) malloc(sizeof(struct Friends));

		(*friends) = userlist.users[pos].friends;

		/*for(i=0; i<userlist.users[pos].friends.numfriends; i++) {
			//friends->friends[i] = userlist.users[pos].friends.friends[i];
			strcpy((*friends).friends[i], userlist.users[pos].friends.friends[i]);
		}*/

		//(*friends).numfriends = userlist.users[pos].friends.numfriends;

		printf("Listando amigos de %s...\n", user);

		for(i=0; i<(*friends).numfriends; i++) {
			printf("%s\n", (*friends).friends[i]);
		}

		*result = 0;
	}

	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__listFriendRequest (struct soap *soap, char * user, struct RequestList * request, int * result) {

}

int ims__newFriend (struct soap *soap, char * user, char * userfriend, int * result) {

	printf("---##### TRACE ims__newFriend #####---\n");

	int numfriends;
	int posF;

	/* Compruebo que el usuario exista */
	int pos = checkUsers(userfriend);
	
	if (pos != -1) {

		/* Compruebo si ya lo tengo como amigo */
		posF = checkFriend(user, userfriend);
		

		/* Si no esta añadido, lo añado */
		if(posF == -1) {

			pos = checkUsers(user);
			numfriends = userlist.users[pos].friends.numfriends;

			/* Compruebo que tengo sitio para otro amigo */
			if(numfriends < IMS_MAX_FRIENDS) {

				strcpy(userlist.users[pos].friends.friends[numfriends], userfriend);
				userlist.users[pos].friends.numfriends++;
				(*result) = 0;
				printf("Añadido amigo %s al usuario %s\n", userlist.users[pos].friends.friends[numfriends], user);
				printf("Numero de amigos %d\n", userlist.users[pos].friends.numfriends);
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

	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__deleteFriend (struct soap *soap, char * user, char * userfriend, int * result) {

}
/*#################################################*/






/*################### USERS ######################*/
int ims__newUser (struct soap *soap, char * user, int * result) {

	printf("---##### TRACE ims__newUser #####---\n");
	
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

	printf("---###############################---\n");
	
	return SOAP_OK;
}

int ims__login (struct soap *soap, char * user, int * result) {

	printf("---##### TRACE ims__login #####---\n");

	int pos;
	int estado;

	/* Comprobamos la posicion del usuario en la lista */
	pos = checkUsers(user);


	/* Si el usuario esta en la lista */
	if(pos != -1) {

		// Comprobamos que el usuario existe
		estado = checkUsersState(user);


		/* El usuario ya existe y esta conectado */
		if(estado == ONLINE) {		
		
			/* Codigo ya existe el usuario */
			(*result) = 1;
		}/* El usuario ya existe y esta desconectado */
		else if(estado == OFFLINE) {

			/* Ponemos el ussuario ONLINE */
			userlist.users[pos].state = ONLINE;
			printf("Usuario %s conectado, estado %d\n", user, userlist.users[pos].state);
			(*result) = 0;

		}/* El usuario fue eliminado. Se pregunta por reactivacion */
		else if(estado == DELETED) {

			/* Codigo usuario borrado */
			(*result) = -1;

		}

		
	}
	/* Si el usuario no esta en la lista */
	else {
		printf("Usuario %s no existe\n", user);
		(*result) = -2;
	}

	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__logout (struct soap *soap, char * user, int * result) {

	printf("---##### TRACE ims__logout #####---\n");

	int pos;

	/* Comprobamos la posicion del usuario en la lista */
	pos = checkUsers(user);

	/* Si el usuario esta en la lista */
	if(pos != -1) {
		/* Ponemos el ussuario OFFLINE */
		userlist.users[pos].state = OFFLINE;
		printf("Usuario %s desconectado, estado %d\n", user, userlist.users[pos].state);
		(*result) = 0;
	}
	/* Si el usuario no esta en la lista */
	else {

		(*result) = -1;
	}

	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__listUsers (struct soap *soap, int * result) {
	(*result) = loadUserList();
	return SOAP_OK;
}

int ims__deleteUser (struct soap *soap, char * user, int * result) {

	printf("---##### TRACE ims__deleteUser #####---\n");

	int pos;
	// Comprobamos que el usuario existe
	if((pos = checkUsers(user)) >= 0) {
		userlist.users[pos].state = -1;
		(*result) = 0;
		printf("Usuario %s eliminado\n", userlist.users[pos].name);
	}
	else {
		(*result) = -1;
		printf("El usuario %s no existe\n", &user);
	}

	printf("---###############################---\n");
	
	return SOAP_OK;
}

int ims__reactivate(struct soap *soap, char * user, int * result) {

	printf("---##### TRACE ims__reactivate #####---\n");

	printf("\t\n");

	int pos;
	// Comprobamos que el usuario existe
	if((pos = checkUsers(user)) >= 0) {
		userlist.users[pos].state = 0;
		(*result) = 0;
		printf("Usuario %s reactivado\n", userlist.users[pos].name);
	}
	else { 
		(*result) = -1;
		printf("El usuario %s no existe\n", &user);
	}

	printf("---###############################---\n");
	
	return SOAP_OK;
}
/*###############################################*/



