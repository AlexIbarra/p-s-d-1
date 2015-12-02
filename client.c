#include "soapH.h"
#include "imsService.nsmap"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>


#define DEBUG_MODE 1


// Fariables globales
char *user;
char *serverURL;
struct soap soap;


char menu();
int gestorMenu(int op);
int newUser();
int deleteUser();
int newFriend();
int newMessage();

int main(int argc, char **argv){

  
  //struct Message myMsg[IMS_MAX_FRIENDS];
  char *friends[IMS_MAX_NAME_SIZE];
  
  char *msg;
  int res;
  int opcion;
  
  
	// Usage
  	if (argc != 3) {
	   printf("Usage: %s http://server:port\n",argv[0]);
	   //printf("Usage: %d http://server:port\n",argc);
	   exit(0);
  	}

	// Init gSOAP environment
  	soap_init(&soap);
  	
	// Obtain server address
	serverURL = argv[1];
		
	// Nombre del usuario logueado
	user = argv[2];
	
	
	/* Pregunto al servidor si el usuario esta logueado */
	
	// Le mostramos un mensaje para que se pueda dar de alta

	newUser();
	printf("Nuevo usuario %s añadido\n", user);

	// Mostramos el menu y recogemos la opcion
	opcion = menu();
	opcion = opcion -48;
	
	// Gestionamos la opcion
	if(gestorMenu(opcion))
		printf("Dado de alta con exito\n");
	else
		exit(1);
  	
	// Clean the environment
  	soap_end(&soap);
  	soap_done(&soap);

  return 0;
}

char menu(){
		
		char opcion;
		
		printf("1) Dar de alta usuario\n");
		printf("2) Dar de baja usuario\n");
		printf("3) Listado de amigos\n");
		printf("4) Anadir nuevo amigo\n");
		printf("5) Borrar a un amigo\n");
		printf("6) Ver mensajes enviados\n");
		printf("7) Ver mensajes recibidos\n");
		printf("8) Enviar un mensaje\n");
		printf("9) Cerrar sesion\n");
		printf("Opcion: ");
		
		scanf("%c", &opcion);
		
		return opcion;
		
}

int gestorMenu(int op) {
	
	int status = 0;
	
	switch(op) {
		case 1: // Dar de alta usuario
			status = newUser();
			break;
		case 2: // Dar de baja usuario
			status = deleteUser();
			break;	
		case 3: //Listar amigos
			status = listFriends();
			break;
		case 4: //Añadir un amigo
			status = newFriend();
			break;
		default:
			break;
	}
	
	return status;		
}



int newUser() {
	int res;
	
	printf("Se va a añadir un nuevo usuario\n");
	soap_call_ims__newUser (&soap, serverURL, "", user, &res);
	
	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}
	
	return res;
}



int deleteUser(){
	char* userdel;
	int res;
	
	printf("Introduce el nombre de usuario que deseas eliminar: \n");
	scanf("%s", &userdel);
	
	soap_call_ims__deleteUser (&soap, serverURL, "", userdel, &res);
	
	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}
	
	return res;
}


int listFriends(){
	int res;

	char *friends[IMS_MAX_FRIENDS];
	
	printf("Los amigos de %s son:\n", user);

	soap_call_ims__listFriends (&soap, serverURL, "", user, friends, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}
	
	return res;
}

int newFriend(){
	int res;
	char* userfriend;
	printf("Introduce el nombre del usuario que quieres añadir como amigo: \n");
	scanf("%s", &userfriend);


	//soap_call_ims__newFriend (&soap, serverURL, "", &user, &userfriend, &res);
	
	// Check for errors...
  	/*if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}
	*/
	return res;
}

int newMessage(){
	int res;
	char* receptor;
	char* message;
	struct Message msg;

	printf("Escribe el nombre del receptor del mensaje: \n");
	scanf("%s", &receptor);
	printf("Escribe el mensaje: (Max 280 caracteres)\n");
	fgets(message, 280, stdin);

	// Guardo el emisor en la estructura
	strcpy(msg.emisor, user);

	// Guardo el receptor en la estructura
	strcpy(msg.emisor, receptor);

	// Guardo el mensaje en la estructura
	strcpy(msg.emisor, message);

	soap_call_ims__sendMessage (&soap, serverURL, "", msg, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}
	
	return res;
}
















