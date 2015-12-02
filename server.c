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

	int numusers, i=0, j=0;
	int friends;
	char user[IMS_MAX_NAME_SIZE];


	int fd = open("bbdd/users.txt",O_CREAT | O_RDONLY, 0666);

	if(read(fd,&numusers,sizeof(int)) < 0)
		printf("Error lectura de la base de datos de los usuarios\n");
	else {
		userlist.numusers = numusers;
		for(i=0; i<numusers; i++) {

			// Leo el nombre del primer usuario
			read(fd,userlist.users[i].name,IMS_MAX_NAME_SIZE);
			//userlist.users[i].name = user;

			// Leo el numero de amigos que tenga
			read(fd,&userlist.users[i].numfriends,sizeof(int));
			//userlist.users[i].numfriends = friends;

			// Leo los amigos
			for(j=0; j<friends; j++) {
				read(fd,userlist.users[i].friends[j],IMS_MAX_NAME_SIZE);
				//userlist.users[i].friends[j] = user;
				lseek(fd,1,SEEK_CUR); // desplazamos 1 byte el cursor por la coma
				//Asignamos el estado del usuario
				userlist.users[i].state = OFFLINE;
			}
		}
	}

	close(fd);
	return 1;
}

int loadMessageList() {

	int nummessages, i=0;
	char msg[IMS_MAX_MSG_SIZE];


	int fd = open("bbdd/messages.txt",O_CREAT | O_RDONLY, 0666);

	if(read(fd,&nummessages,sizeof(int)) < 0)
		printf("Error lectura de la base de datos de los mensajes\n");
	else {
		messagelist.nummessages = nummessages;
		for(i=0; i<nummessages; i++) {

			// Leo el nombre del emisor
			read(fd,messagelist.messages[i].emisor,IMS_MAX_NAME_SIZE);
			//messagelist.messages[i].emisor = user;

			// Leo el nombre del receptor
			read(fd,messagelist.messages[i].receptor,IMS_MAX_NAME_SIZE);
			//messagelist.messages[i].receptor = user;

			// Leo el mensaje
			read(fd,messagelist.messages[i].msg,IMS_MAX_MSG_SIZE);
			//messagelist.messages[i].msg = msg;

		}
	}

	close(fd);
	return 1;
}

int checkUsers(char *name) {

	int i = 0;

	for(i=0; i < userlist.numusers; i++){
		
		if(strcmp(userlist.users[i].name,name)==0){
			printf("%s\n", userlist.users[i].name);
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
	return -1;
}

void guardarUsuarios() {

	int i=0, j=0;

	int fd = open("bbdd/users.txt",O_RDWR, 0666);

	if(fd == -1) {
		fprintf(stderr, "Error open\n");
		exit(1);
	}

	for(i=0; i<userlist.numusers; i++) {

		// Guardamos el numero de usuarios
		write(fd, &userlist.numusers, sizeof(int));

		// Guardamos el nombre
		write(fd, &userlist.users[i].name, sizeof(char)*strlen(userlist.users[i].name));

		// Guardamos el numero de amigos
		write(fd, &userlist.users[i].numfriends, sizeof(int));

		// Si tiene amigos, los guardamos
		for(j=0; j<userlist.users[i].numfriends; j++) {
			write(fd, &userlist.users[i].friends[j], sizeof(char)*strlen(userlist.users[i].friends[j]));		
		}

	}

	close(fd);
}

void guardarMensajes() {

	int i=0, j=0;

	int fd = open("bbdd/messages.txt",O_RDWR, 0666);

	if(fd == -1) {
		fprintf(stderr, "Error open\n");
		exit(1);
	}

	for(i=0; i<messagelist.nummessages; i++) {

		// Guardamos el numero de mensajes
		write(fd, &messagelist.nummessages, sizeof(int));

		// Guardamos el nombre del emisor
		write(fd, &messagelist.messages[i].emisor, sizeof(char)*strlen(messagelist.messages[i].emisor));

		// Guardamos el nombre del receptor
		write(fd, &messagelist.messages[i].receptor, sizeof(char)*strlen(messagelist.messages[i].receptor));

		// Guardamos el mensaje
		write(fd, &messagelist.messages[i].msg, sizeof(char)*strlen(messagelist.messages[i].msg));


	}

	close(fd);
}

void listarAmigos(int pos, char *friends[]) {

	/*int i=0;

	for(i=0; i<userlist.users[pos].numfriends; i++) {
		*friends[i] = userlist.users[pos].friends[i];
	}
	*/
	*friends = (char*)userlist.users[pos].friends;

}



// Handler para la señal Cntrl + C 

void salir(int senal){

	int i;
	fflush (stdout);
	switch(senal){
		case SIGINT:
			printf("Cerrando servidor...\n");

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


int ims__sendMessage (struct soap *soap, struct Message myMessage, int *result){

	printf ("Received by server: \n\tusername:%s \n\tmsg:%s\n", myMessage.emisor, myMessage.msg);
	return SOAP_OK;
}


int ims__receiveMessage (struct soap *soap, struct Message *myMessage){
	/*
		// Allocate space for the message field of the myMessage struct then copy it
		Message->msg = (xsd__string) malloc (IMS_MAX_MSG_SIZE);
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


int ims__listFriends (struct soap *soap, char * user, char *friends[], int * result) {
	int pos;
	if((pos = checkUsers(user)) != -1) {

		listarAmigos(pos, friends);

		*result = 1;
	}

	return SOAP_OK;
}


int ims__newUser (struct soap *soap, char * user, int * result) {
	
	// Comprobamos que el usuario existe
	if(checkUsers(user) >= 0) {
		printf("El usuario %s ya existe\n", user);
	}
	else { 
		strcpy(userlist.users[userlist.numusers].name, user);
		userlist.users[userlist.numusers].numfriends = 0;
		userlist.users[userlist.numusers].state = OFFLINE;
		userlist.numusers++;

		printf("Añadido usuario %s\n", &userlist.users[userlist.numusers].name);
		printf("Numero de usuarios: %d\n", userlist.numusers);
	}
	
	return SOAP_OK;
}



int ims__deleteUser (struct soap *soap, char * user, int * result) {

	// Comprobamos que el usuario existe
	if(checkUsers(user) >= 0) {
		userlist.users[userlist.numusers].state = DELETED;
		
	}
	else { 
		printf("El usuario %s no existe\n", &user);
	}
	
	return SOAP_OK;
}



