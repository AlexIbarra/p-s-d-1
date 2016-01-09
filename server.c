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
struct RequestList requestlist;

/*### MACROS ###*/
#define BACKLOG (100)

// USUARIOS
#define OFFLINE 0
#define ONLINE 1
#define DELETED -1

// MENSAJES
#define SEND 0
#define RECEIVE 1
#define READ 2

// AMIGOS
#define DELFRIEND -1
#define REJECTED -2
#define ACEPTEDFRIEND 1
#define PENDINGFRIEND 0




int checkUsers(char *name);
int checkUsersState(char *name);
int loadUserList();
int loadMessageList();
int loadRequestList();
void salir(int senal);
void disconectUser();


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

		// Inicializamos la lista de peticiones
		 loadRequestList();

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

	//printf("---##### TRACE loadUserList #####---\n");

	int numusers=0, i=0, j=0;
	int friends;
	char user[IMS_MAX_NAME_SIZE];
	struct User usertmp;
	int nread;


	FILE *fd = fopen("bbdd/users.txt","r+");

	if(fd == NULL){
		fd = fopen("bbdd/users.txt","w");
		fclose(fd);
	}
	else {

		while((nread = fread(&usertmp, sizeof(struct User), 1, fd)) > 0) {
			//if(usertmp.state != DELETED) {
			userlist.users[numusers] = usertmp;
		//	printf("Leido usuario %s con estado %d\n",userlist.users[numusers].name, userlist.users[numusers].state);
			numusers++;
			//}
		}

		userlist.numusers = numusers;

		fclose(fd);
	}

	//printf("---###############################---\n");
	printf("Cargada lista de usuarios\n");
	return 1;
}

void guardarUsuarios() {

	//printf("\t---##### TRACE guardarUsuarios #####---\n");

	int i=0, j=0;

	FILE *fd = fopen("bbdd/users.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "Error open\n");
		exit(1);
	}

	for(i=0; i<userlist.numusers; i++) {
	//	printf("\tGuardando usuario %s, estado %d\n",userlist.users[i].name, userlist.users[i].state);
		fwrite(&userlist.users[i], sizeof(struct User), 1, fd);	
	}
	

	//fwrite(&userlist.users, sizeof(struct User), userlist.numusers, fd);

	fclose(fd);

	//printf("\t---###############################---\n");
	printf("Almacenada lista de usuarios\n");
}

int loadMessageList() {

	//printf("---##### TRACE loadMessageList #####---\n");

	int nummessages=0, i=0;
	char msg[IMS_MAX_MSG_SIZE];
	int nread;
	struct Message msgtmp;


	FILE *fd = fopen("bbdd/messages.txt","r+");

	if(fd == NULL){
		fd = fopen("bbdd/messages.txt","w");
		fclose(fd);
	}
	else {

		while((nread = fread(&msgtmp, sizeof(struct Message), 1, fd)) > 0) {
			messagelist.messages[nummessages] = msgtmp;
		//	printf("\t Leyendo emisor %s, receptor %s, mensaje %s, estado %d\n",msgtmp.emisor, msgtmp.receptor, msgtmp.msg, msgtmp.state);
			nummessages++;
		}

		messagelist.nummessages = nummessages;

		fclose(fd);
	}

	//printf("---###############################---\n");

	printf("Cargada lista de mensajes\n");

	return 1;
}

void guardarMensajes() {

	//printf("\t---##### TRACE guardarMensajes #####---\n");

	int i=0, j=0;

	FILE *fd = fopen("bbdd/messages.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "\tError open\n");
		exit(1);
	}

	for(i=0; i<messagelist.nummessages; i++) {
	//	printf("\tGuardando emisor %s, receptor %s, mensaje %s, estado %d\n",messagelist.messages[i].emisor, messagelist.messages[i].receptor, messagelist.messages[i].msg, messagelist.messages[i].state);
		fwrite(&messagelist.messages[i], sizeof(struct Message), 1, fd);	
	}

	//fwrite(&messagelist.messages, sizeof(struct Message), messagelist.nummessages, fd);

	fclose(fd);

	//printf("\t---###############################---\n");
	printf("Almacenada lista de mensajes\n");
}

int loadRequestList(){

	//printf("---##### TRACE loadRequestList #####---\n");

	int numrequest=0, i=0;
	int nread;
	struct Request rqstmp;


	FILE *fd = fopen("bbdd/request.txt","r+");

	if(fd == NULL){
		fd = fopen("bbdd/request.txt","w");
		fclose(fd);
	}
	else {

		while((nread = fread(&rqstmp, sizeof(struct Request), 1, fd)) > 0) {
			requestlist.request[numrequest] = rqstmp;
			//printf("\t Leyendo emisor %s, receptor %s,estado %d\n",rqstmp.emisor, rqstmp.receptor, rqstmp.state);
			numrequest++;
		}

		requestlist.numrequest = numrequest;

		fclose(fd);
	}

	//printf("---###############################---\n");
	printf("Cargada lista de peticiones de amistad\n");
	return 1;
}

void guardarRequestList() {

	//printf("\t---##### TRACE guardarPeticiones #####---\n");

	int i=0, j=0;

	FILE *fd = fopen("bbdd/request.txt","w+");

	if(fd == (FILE *)-1) {
		fprintf(stderr, "\tError open\n");
		exit(1);
	}

	for(i=0; i<requestlist.numrequest; i++) {
		//printf("\tGuardando emisor %s, receptor %s, estado %d\n",requestlist.request[i].emisor, requestlist.request[i].receptor,  requestlist.request[i].state);
		fwrite(&requestlist.request[i], sizeof(struct Request), 1, fd);	
	}

	//fwrite(&messagelist.messages, sizeof(struct Message), messagelist.nummessages, fd);

	fclose(fd);

	//printf("\t---###############################---\n");
	printf("Almacenada lista de peticiones de amistad\n");
}
/*#######################################################*/




// Handler para la señal Cntrl + C 
void salir(int senal){

	//printf("---##### TRACE salir #####---\n");

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
			disconectUser();
			guardarUsuarios();
			guardarMensajes();
			guardarRequestList();

	//printf("---###############################---\n");
			printf("Servidor cerrado.\n");
			exit(1);
		break;
	}

}

int checkUsers(char *name) {

	//	printf("\t---##### TRACE checkUsers #####---\n");

	int i = 0;
	int pos = -1;
	int encontrado=0;

	while(i < userlist.numusers && !encontrado){
		//printf("\tCheckeado usuario %s\n", userlist.users[i].name);
		if(strcmp(userlist.users[i].name,name)==0){
		//	printf("\tEncontrado usuario --> %s\n", userlist.users[i].name);
			pos = i;
			encontrado = 1;
		}
		i++;
	}

	//	printf("\t---###############################---\n");

	return pos;
}

int checkUsersState(char *name) {

	//printf("\t---##### TRACE checkUsersState #####---\n");

	int i = 0;
	int state = -2;

	//printf("\t");
	
	if((i = checkUsers(name)) != -1){
		state =  userlist.users[i].state;
	}

	//printf("\t---###############################---\n");

	return state;
}

int checkFriend(char *user, char *name) {

	//printf("\t---##### TRACE checkFriend #####---\n");

	//printf("\t");

	int i = 0;
	int tmp = -1;
	int pos = checkUsers(user);
	int encontrado=0;

	/* Busco en la lista de amigos del usuario */
	while(i< userlist.users[pos].fList.numfriends && !encontrado){
		
		/* Si se encuentra en mi lista de amigos, retorno la posicion */
		if(strcmp(userlist.users[pos].fList.listfriends[i].friends,name)==0) {
		//	printf("Encontrado amigo %s\n", userlist.users[pos].fList.listfriends[i].friends);
			tmp = i;
			encontrado = 1;
		}
		i++;
	}

	//	printf("\t---###############################---\n");

	/* Si no esta en la lista, retorno -1 */
	return tmp;
}

int checkPetition(char *emisor, char *receptor) {

	//	printf("\t---##### TRACE checkPetition #####---\n");

	printf("\t");

	int i = 0;
	int tmp = -1;
	int encontrado=0;

	/* Busco en la lista de peticiones */
	while(i< requestlist.numrequest && !encontrado) {

		
		/* Si la peticion ya esta en la lista (como pendiente) */
		if(!strcmp(requestlist.request[i].emisor, emisor) && !strcmp(requestlist.request[i].receptor, receptor) && requestlist.request[i].state == PENDINGFRIEND) {
		//	printf("Encontrada peticion de %s a %s con estado %d\n", emisor, receptor, requestlist.request[i].state);
			tmp = i;
			encontrado = 1;
		}
		i++;
	}

	//	printf("\t---###############################---\n");

	/* Si no esta en la lista, retorno -1 */
	return tmp;
}

int checkFreeFriendPosition(char *name) {

	//	printf("\t---##### TRACE checkFreeFriendPosition #####---\n");

	//	printf("\t");

	int i = 0;
	int tmp = -1;
	int encontrado=0;

	int pos = checkUsers(name);

	if(pos != -1) {

		/* Si no ha superado el maximo de amigos */
		if(userlist.users[pos].fList.numfriends < IMS_MAX_FRIENDS)
			/* Retorno la ultima posicion de la lista (para que añada el nuevo amigo) */
			tmp = userlist.users[pos].fList.numfriends;
		/* Si no, miro si hay huecos libre en la lista (amigos eliminados) */
		else {

			/* Busco en la lista de amigos para ver si hay hueco */
			while(i < userlist.users[pos].fList.numfriends && !encontrado) {

				/* Si hay algun amigo que haya sido eliminado */
				if(userlist.users[pos].fList.listfriends[i].state == DELFRIEND) {
				//	printf("Encontrade hueco en %d a %s con estado %d\n", i, userlist.users[pos].fList.listfriends[i].friends, userlist.users[pos].fList.listfriends[i].state);
					tmp = i;
					encontrado = 1;
				}
				i++;
			}
		}
	}

	//	printf("\t---###############################---\n");

	/* Si no hay hueco, retorno -1 */
	return tmp;
}

void disconectUser() {
	int i=0;

	for(i=0; i<userlist.numusers; i++) {

		if(userlist.users[i].state == ONLINE)
			userlist.users[i].state = OFFLINE;
	}
	printf("Desconectados todos los usuarios\n");
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

	//printf("---##### TRACE ims__sendMessage #####---\n");

	//printf ("Received by server: \n\temisor:%s \n\treceptor:%s \n\tmsg:%s\n", myMessage.emisor, myMessage.receptor, myMessage.msg);
	
	/* Compruebo que el usuario exista */
	int pos = checkUsers(myMessage.receptor);
	
	/* Si el usuario existe */
	if (pos != -1) {

		/* Comprobamos de que ambos usuarios sean amigos */
		pos = checkFriend(myMessage.emisor, myMessage.receptor);

		if(pos != -1) {

			/* Copiamos los datos a la lista de mensajes */
			strcpy(messagelist.messages[messagelist.nummessages].emisor, myMessage.emisor);
			strcpy(messagelist.messages[messagelist.nummessages].receptor, myMessage.receptor);
			strcpy(messagelist.messages[messagelist.nummessages].msg, myMessage.msg);
			messagelist.messages[messagelist.nummessages].state = RECEIVE;
			messagelist.nummessages++;
			(*result) = 0;

			printf("Enviado '%s' de %s a %s con estado %d \n", messagelist.messages[messagelist.nummessages-1].msg, messagelist.messages[messagelist.nummessages-1].emisor, messagelist.messages[messagelist.nummessages-1].receptor, messagelist.messages[messagelist.nummessages-1].state);
		}
		else 
			(*result) = -2;
	}
	else {
		(*result) = -1;
	}

	//printf("---###############################---\n");

	return SOAP_OK;
}

int ims__receiveMessage (struct soap *soap, char * user, int * state, struct MessageList * myListMessage) {

	//printf("---##### TRACE ims__receiveMessage #####---\n");
	
	int i=0;
	int encontrado = 0;
	

	/* Busco los mensajes en los que el usuario es el receptor */
	if((*state) == RECEIVE) {

		while(i < messagelist.nummessages && !encontrado) {

			if(!strcmp(messagelist.messages[i].receptor, user)) {

				/* Copio los datos del mensaje */
				strcpy((*myListMessage).messages[i].receptor, messagelist.messages[i].receptor);
				strcpy((*myListMessage).messages[i].emisor, messagelist.messages[i].emisor);
				strcpy((*myListMessage).messages[i].msg, messagelist.messages[i].msg);
				/* Le indico al cliente que los mensajes seran aquellos recibidos */
				(*myListMessage).messages[i].state = RECEIVE;
				/* Modifico el estado del mensaje para marcarlos como leidos */
				messagelist.messages[i].state = READ;
				(*myListMessage).nummessages++;
				(*myListMessage).result = 0;
				encontrado = 1;
				printf("%s ha leido '%s' de %s para %s con estado %d\n", user, (*myListMessage).messages[i].msg, (*myListMessage).messages[i].emisor, (*myListMessage).messages[i].receptor, messagelist.messages[i].state);
				//printf("%s ha leido '%s' de %s para %s con estado %d \n", user, messagelist.messages[i].msg, messagelist.messages[i].emisor, messagelist.messages[i].receptor, messagelist.messages[i].state);
			}
			i++;
		}
	}
	/* Busco los mensajes en los que el usuario es el emisor */
	else if((*state) == SEND) {
		while(i < messagelist.nummessages && !encontrado) {

			if(!strcmp(messagelist.messages[i].emisor, user)) {

				/* Copio los datos del mensaje */
				strcpy((*myListMessage).messages[i].receptor, messagelist.messages[i].receptor);
				strcpy((*myListMessage).messages[i].emisor, messagelist.messages[i].emisor);
				strcpy((*myListMessage).messages[i].msg, messagelist.messages[i].msg);
				/* Le indico al cliente el estado en el que se encuentra el mensaje */
				(*myListMessage).messages[i].state = messagelist.messages[i].state;
				(*myListMessage).nummessages++;
				(*myListMessage).result = 0;
				encontrado = 1;
				printf("Leido '%s' de %s para %s con estado %d\n", (*myListMessage).messages[i].msg, (*myListMessage).messages[i].emisor, (*myListMessage).messages[i].receptor, messagelist.messages[i].state);
				//printf("Leido '%s' de %s para %s con estado %d \n", messagelist.messages[i].msg, messagelist.messages[i].emisor, messagelist.messages[i].receptor, messagelist.messages[i].state);
			}
			i++;
		}
	}
		

	//printf("---###############################---\n");

	return SOAP_OK;
}
/*###################################################*/






/*################### FRIENDS ######################*/
int ims__newFriend (struct soap *soap, char * user, char * userfriend, int * result) {

	
	//printf("---##### TRACE ims__newFriend #####---\n");
	printf("El usuario %s ha envíado una solicitud de amistad a %s", user, userfriend);
	int numfriends;
	int posF;
	int posP;

	/* Compruebo que no intente añadirse como amigo a si mismo */
	if(!strcmp(user, userfriend)) {

		(*result) = -5;
	}
	/* Si los usuarios son distintos */
	else {

		/* Compruebo que el usuario exista */
		int pos = checkUsers(userfriend);
		int posuser = checkUsers(user);
		
		if (pos != -1) {

			/* Compruebo si ya lo tengo como amigo */
			posF = checkFriend(user, userfriend);
			

			/* Si no esta añadido, creo una nueva petición de amistad */
			if(posF == -1) {

				/* Compruebo si la peticion ya esta hecha para ese usuario */
				posP = checkPetition(user, userfriend);

				/* Todavia no esta hecha la peticion de amistad a ese usuario */
				if(posP == -1) {

					/* No caben mas amigos al usuario */
					if(userlist.users[pos].fList.numfriends == 50) {
						(*result) = -3;
					}
					else if(userlist.users[posuser].fList.numfriends == 50) {
						(*result) = -4;
					}
					else {

						strcpy(requestlist.request[requestlist.numrequest].emisor, user);
						strcpy(requestlist.request[requestlist.numrequest].receptor, userfriend);
						requestlist.numrequest ++;
						(*result) = 0;

						//printf("Numero de peticiones %d\n", requestlist.numrequest);
					//printf("Añadida peticion de %s a %s\n", requestlist.request[requestlist.numrequest-1].emisor, requestlist.request[requestlist.numrequest-1].receptor);
					}
				}
				/* La peticion de amistad ya estaba hecha */
				else {
					(*result) = -6;
				}
				
			}	
			/* Si ya esta añadido, informo al cliente */
			else {
				(*result) = -1;
			}
		}
		else{
			//printf("El usuario %s no existe", userfriend);
			(*result) = -2;
		}

	}

	//printf("---###############################---\n");

	return SOAP_OK;
}

int ims__deleteFriend (struct soap *soap, char * user, char * userfriend, int * result) {


	//printf("---##### TRACE ims__deleteFriend #####---\n");
	printf("Los usuarios %s y %s ya no son amigos", user, userfriend);
	int posF, pos, posFf;
	/* Compruebo que el usuario exista */
	int posAmigo = checkUsers(userfriend);

	if (posAmigo != -1) {

		/* Busco la posicion del usuario */
		pos = checkUsers(user);

		/* Compruebo que esta en la lista de amigos */
		posF = checkFriend(user, userfriend);
		posFf = checkFriend(userfriend, user);

		if(posF != -1) {

			/* Marcamos al amigo como eliminado */
			userlist.users[posAmigo].fList.listfriends[posFf].state = DELFRIEND;
			userlist.users[pos].fList.listfriends[posF].state = DELFRIEND;

			/* decrementamos el numero de amigos */
			userlist.users[pos].fList.numfriends--;
			userlist.users[posAmigo].fList.numfriends--;

			//printf("Eliminado amigo %s, estado %d\n", userlist.users[pos].fList.listfriends[posF].friends, userlist.users[pos].fList.listfriends[posF].state);
		}
		/* No es amigo */
		else {
			(*result) = -2;
		}
	}
	/* El amigo que quiere eliminar no existe */
	else {
		(*result) = -1;
	}

	//printf("---###############################---\n");

	return SOAP_OK;

}

int ims__listFriends (struct soap *soap, char * user, struct ListFriends * friends) {

	//printf("---##### TRACE ims__listFriends #####---\n");

	int pos, i;


	if((pos = checkUsers(user)) != -1) {

		for(i=0; i<userlist.users[pos].fList.numfriends; i++) {
			strcpy((*friends).listfriends[i].friends, userlist.users[pos].fList.listfriends[i].friends);
		}

		(*friends).numfriends = userlist.users[pos].fList.numfriends;

		//printf("Numero de amigos %d\n", (*friends).numfriends);
		printf("Listando amigos de %s...\n", user);

		for(i=0; i<(*friends).numfriends; i++) {
			printf(">%s : %d\n", (*friends).listfriends[i].friends, (*friends).listfriends[i].state);
		}

		(*friends).result = 0;
	}
	else {

		(*friends).result = -1;
	}

	//printf("---###############################---\n");

	return SOAP_OK;
}

int ims__listFriendRequest (struct soap *soap, char * user, struct RequestList * lRequest) {

	//	printf("---##### TRACE ims__listFriendRequest #####---\n");

	int pos, i;

	/* Comprobamos que el usuario existe */
	pos = checkUsers(user);

	if(pos != -1) {

		for(i=0; i<requestlist.numrequest; i++) {

			/* Busco en la lista las peticions al usuario con peticiones pendientes */
			if(!strcmp(requestlist.request[i].receptor, user) && requestlist.request[i].state == PENDINGFRIEND) {
				/* Copio el emisor de la peticion */
				strcpy((*lRequest).request[i].emisor, requestlist.request[i].emisor);
				/* Copio el receptor de la peticion */
				strcpy((*lRequest).request[i].receptor, user);
				/* Copio el estado de la peticion */
				(*lRequest).request[i].state = PENDINGFRIEND;
				/* Incremento el numero de peticiones */
				(*lRequest).numrequest++;
			}
		}

		//printf("Numero de peticiones %d\n", (*lRequest).numrequest);
		printf("Listando peticiones de %s...\n", user);

		/*for(i=0; i<(*lRequest).numrequest; i++) {
			printf(">%s : %d\n", (*lRequest).request[i].receptor, (*lRequest).request[i].state);
		}*/

		(*lRequest).result = 0;
	}
	else {

		(*lRequest).result = -1;
	}

	//printf("---###############################---\n");

	return SOAP_OK;
}

int ims__aceptRequest(struct soap *soap, char * user, struct Request request, int * result) {

	//printf("---##### TRACE ims__aceptRequest #####---\n");

	//Lo primero es buscar la petición en la lista
	int i = 0;
	int pos;
	int posAux;
	int encontrado = -1;

	while (i < requestlist.numrequest && encontrado == -1){
		if ((strcmp(requestlist.request[i].emisor, request.emisor) == 0) && (strcmp(requestlist.request[i].receptor, request.receptor) == 0)){
			//printf("Peticion encontrada de %s a %s \n", requestlist.request[i].emisor, requestlist.request[i].receptor);
			encontrado = 0;
			pos = i;
		}
		i++;
	}

	//Ahora que tenemos encontrada la posicón procedemos a marcarla como aceptada (1) y modificar las estructuras de usuario de ambos

	if(encontrado == -1)
		(*result) = -1;
	else {
		//Marcamos la solicitud como aceptada
		requestlist.request[pos].state = ACEPTEDFRIEND;

		//Añadimos al usuario emisor como amigo del receptor.
		posAux = checkUsers(requestlist.request[pos].emisor);
		strcpy(userlist.users[posAux].fList.listfriends[userlist.users[posAux].fList.numfriends].friends, requestlist.request[pos].receptor);
		userlist.users[posAux].fList.numfriends ++;
		printf("Añadido %s como amigo de %s\n", userlist.users[posAux].fList.listfriends[userlist.users[posAux].fList.numfriends-1].friends, requestlist.request[pos].receptor);		
		
		//Y ahora viceversa
		posAux = checkUsers(requestlist.request[pos].receptor);
		strcpy(userlist.users[posAux].fList.listfriends[userlist.users[posAux].fList.numfriends].friends, requestlist.request[pos].emisor);
		userlist.users[posAux].fList.numfriends ++;
		printf("Añadido %s como amigo de %s\n", userlist.users[posAux].fList.listfriends[userlist.users[posAux].fList.numfriends-1].friends, requestlist.request[pos].emisor);
		
		(*result) = 0;
	}

	//printf("---###############################---\n");

	return SOAP_OK;	
}

int ims__rejectRequest(struct soap *soap, char * user, struct Request request, int * result) {

	//printf("---##### TRACE ims__rejectRequest #####---\n");
	
	//Lo primero es buscar la petición en la lista
	int i = 0;
	int pos;
	int encontrado = -1;
	while (i < requestlist.numrequest && encontrado == -1){
		if ((strcmp(requestlist.request[i].emisor, request.emisor) == 0) && (strcmp(requestlist.request[i].receptor, request.receptor) == 0)){
			encontrado = 0;
			pos = i;
		}
		i++;
	}



	//Ahora que tenemos encontrada la posicón procedemos a marcarla como rechazada (-1)
	if (encontrado == -1)
		(*result)= -1;
	else{
		printf("%s ha rechazado una petición de amistad de %s", request.receptor, request.emisor);
		requestlist.request[pos].state = -1;
		(*result) = 0;
	}

	//printf("---###############################---\n");

	return SOAP_OK;
}
/*#################################################*/






/*################### USERS ######################*/
int ims__newUser (struct soap *soap, char * user, int * result) {

	//printf("---##### TRACE ims__newUser #####---\n");
	
	
	/* Compruebo que el usuario no exista */
	int pos = checkUsers(user);
	
	if(pos == -1) {

		/* Compruebo que se pueden añadir mas usuario */
		if(userlist.numusers < IMS_MAX_USERS) {

			strcpy(userlist.users[userlist.numusers].name, user);
			userlist.users[userlist.numusers].fList.numfriends = 0;
			userlist.users[userlist.numusers].state = ONLINE;
			userlist.numusers++;

			printf("Añadido usuario %s\n", user);

			/* Codigo exito al añadir */
			(*result) = 0;

		}/* La lista de usuario esta llena e informamos */
		else {

			/* Codigo lista llena */
			(*result) = -2;
		}
	}
	/* El usuario ya existe */
	else {
		(*result) = -1;
	}

	//printf("---###############################---\n");
	
	return SOAP_OK;
}

int ims__login (struct soap *soap, char * user, int * result) {

	//	printf("---##### TRACE ims__login #####---\n");

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
			printf("Usuario %s conectado\n", user);
			(*result) = 0;

		}/* El usuario fue eliminado. Se pregunta por reactivacion */
		else if(estado == DELETED) {

			/* Codigo usuario borrado */
			(*result) = -1;

		}

		
	}
	/* Si el usuario no esta en la lista */
	else {
	//	printf("Usuario %s no existe\n", user);
		(*result) = -2;
	}

	//	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__logout (struct soap *soap, char * user, int * result) {

	//	printf("---##### TRACE ims__logout #####---\n");

	int pos;

	/* Comprobamos la posicion del usuario en la lista */
	pos = checkUsers(user);

	/* Si el usuario esta en la lista */
	if(pos != -1) {
		/* Ponemos el ussuario OFFLINE */
		userlist.users[pos].state = OFFLINE;
		printf("Usuario %s desconectado\n", user);
		(*result) = 0;
	}
	/* Si el usuario no esta en la lista */
	else {

		(*result) = -1;
	}

		//	printf("---###############################---\n");

	return SOAP_OK;
}

int ims__deleteUser (struct soap *soap, char * user, int * result) {

	//printf("---##### TRACE ims__deleteUser #####---\n");

	int pos;
	// Comprobamos que el usuario existe
	if((pos = checkUsers(user)) >= 0) {
		userlist.users[pos].state = -1;
		(*result) = 0;
		printf("Usuario %s eliminado\n", userlist.users[pos].name);
	}
	else {
		(*result) = -1;
	//	printf("El usuario %s no existe\n", &user);
	}

	//	printf("---###############################---\n");
	
	return SOAP_OK;
}

int ims__reactivate(struct soap *soap, char * user, int * result) {

	//	printf("---##### TRACE ims__reactivate #####---\n");

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
	//	printf("El usuario %s no existe\n", &user);
	}

	//	printf("---###############################---\n");
	
	return SOAP_OK;
}
/*###############################################*/



