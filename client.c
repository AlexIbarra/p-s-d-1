#include "soapH.h"
#include "imsService.nsmap"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>


#define DEBUG_MODE 1


// Variables globales
char *serverURL;
struct soap soap;
char conectedUser[IMS_MAX_NAME_SIZE];
char addFriend[IMS_MAX_NAME_SIZE];


int menu();
int gestorMenu(int op);
void newUser(char *user);
void deleteUser(char *userdel);
void login(char *user);
void logout(char *user);
void newFriend(char *userfriend);
void deleteFriend(char * name);
void listFriends(char *user);
void newMessage();
void listMessages();




int main(int argc, char **argv){

  
  //struct Message myMsg[IMS_MAX_FRIENDS];
	char *friends[IMS_MAX_NAME_SIZE];
	char user[IMS_MAX_NAME_SIZE];
	char *msg;
	int res;
	int opcion, status=1;
  
  
	// Usage
  	if (argc != 3) {
	   printf("Usage: %s http://server:port\n",argv[0]);
	   exit(0);
  	}

	// Init gSOAP environment
  	soap_init(&soap);
  	
	// Obtain server address
	serverURL = argv[1];
		
	// Nombre del usuario logueado
	//user = argv[2];
	strcpy(conectedUser, argv[2]);
	
	/* Añadimos el usuario con el que se ha concetado el cliente */
	//newUser(user);
	login(conectedUser);

	while(status) {

		// Mostramos el menu y recogemos la opcion
		opcion = menu();
		//if(opcion != 10) {
		status = gestorMenu(opcion);
		//}
	}
  	
	// Clean the environment
  	soap_end(&soap);
  	soap_done(&soap);

  return 0;
}

int menu(){
		
	char opcion[10];
	int value;
	
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
	
	scanf("%s", opcion);
	//fflush(stdin);

	value = atoi(opcion);

	//opcion = getchar();

	// if(opcion == '\n')
	// 	value = 10;
	// else
	// 	value = opcion - 48;


	printf("\n");
	
	return value;
		
}

int gestorMenu(int op) {
	
	int status = 1;
	char user[IMS_MAX_NAME_SIZE];
	int *res;
	
	switch(op) {
		case 1: // Dar de alta usuario
			printf("Introduzca nombre de usuario: ");
			scanf("%s", user);		
			newUser(user);
			printf("\n");
			break;
		case 2: // Dar de baja usuario
			printf("Introduce el nombre del usuario que deseas eliminar: ");
			scanf("%s", user);			
			deleteUser(user);
			printf("\n");
			break;	
		case 3: //Listar amigos
			listFriends(conectedUser);
			break;
		case 4: //Añadir un amigo
			printf("Introduzca nombre del usuario que quiere añadir como amigo: ");
			scanf("%s", addFriend);		
			newFriend(addFriend);
			printf("\n");
			break;
		case 5: //Borrar un amigo
			//newUser(&user);
			break;
		case 6: //Ver mensajes enviados
			//newUser(&user);
			break;
		case 7: //Ver mensajes recibidos
			//newUser(&user);
			break;
		case 8: //Enviar un mensaje
			//newUser(&user);
			break;
		case 9: //Cerrar sesion
			//newUser(&user);
			// hay que avisar al servidor de que vamos a salir de la sesion
			status = 0;
			break;
		default:
			printf("Opcion incorrecta!!!\n");
			break;
	}

	return status;	
}



/*##### TRATAMIENTO DE USUARIOS #####*/
void newUser(char *user) {

	int res;
	char option;
	
	/* Hacemos la peticion el servidor */
	soap_call_ims__newUser (&soap, serverURL, "", user, &res);
	
	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}

  	/* Tratamiento de la respuesta del servidor */

  	if(res == 0) { // Añadido correctamente
  		printf("Añadido usuario %s\n\n", user);
  	}
	else if(res == 1) { // Usuario ya existe
		printf("El usuario %s ya existe\n\n", user);
	}
	else if(res == 2) {
		printf("Bienvenido %s\n\n", user);
	}
	else if(res == -1) { // Usuario eliminado
		printf("El usuario fue eliminado anteriormente. ¿Desea reactivarlo? (s/n): ");
		fflush(stdin);
		scanf("%c", option);
		fflush(stdin);
		printf("\n");

		if(option == 's') {
			/* Paedimos al servidor que añada al usuario nuevamente */
			soap_call_ims__reactivate(&soap, serverURL, "", user, &res);

			if(res == 0)
				printf("Usuario %s reactivado correctamente\n\n", user);
		}
	}
	else if(res == -2) { // Lista llena
		printf("Lista de usuarios llena\n\n");
	}
}

void deleteUser(char *userdel) {

	int res;	
	
	soap_call_ims__deleteUser (&soap, serverURL, "", userdel, &res);
	
	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}
}

void login(char *user) {

	int res;
	char opcion[10];

	soap_call_ims__login(&soap, serverURL, "", user, &res);

	if(res == 1) {

		printf("El usuario %s no esta dado de alta\n", user);
		printf("¿Quiere darlo de alta? (s/n): ");
		scanf("%s", opcion);

		if(!strcmp(opcion, "s"))
			newUser(user);
		//else
			//salimos

	}
	else
		printf("Bienvenido %s\n", user);
}

void logout(char *user) {

}
/*##################################*/


/*########### TRATAMIENTO DE AMIGOS ###########*/
void listFriends(char *user){
	
	int res, i;
	//int numfriends;
	struct Friends friends;
	//char friends[IMS_MAX_FRIENDS][IMS_MAX_NAME_SIZE];

	//soap_call_ims__listFriends (&soap, serverURL, "", user, friends, &numfriends, &res);
	//friends = (struct Friends *) malloc(sizeof(struct Friends));

	soap_call_ims__listFriends (&soap, serverURL, "", user, &friends, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

  	if(res == 0) {

  		printf("Numero de amigos %d\n", friends.numfriends);
  		// if(friends->numfriends < 1)
  		// 	printf("No tienes amigos --> %d\n", friends.numfriends);
  		// else {
  		// 	printf("Los amigos de %s son:\n", user);
	  	// 	for(i=0; i<friends.numfriends; i++) {
	  	// 		printf("- %s\n", friends.friends[i]);
	  	// 		//printf("%d\n", i);
	  	// 	}
	  	// }
  	}
}

void newFriend(char *userfriend){
	int res;

	soap_call_ims__newFriend (&soap, serverURL, "", conectedUser, userfriend, &res);
	
	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}
	if (res == -2){
		printf("El usuario %s no existe\n", userfriend);
	}
	else if(res == 0) {
  		printf("El usuario %s fue añadido a su lista de amigos\n", userfriend);
  	}
  	else {
  		printf("El usuario %s ya estaba en su lista de amigos\n", userfriend);
  	}
}

void deleteFriend(char * name) {

}
/*############################################*/


/*########### TRATAMIENTO DE MENSAJES ###########*/
void newMessage(char *user){
	int res;
	char* receptor;
	char* message;
	struct Message msg;

	printf("Escribe el nombre del receptor del mensaje: \n");
	scanf("%s", &receptor);
	fflush(stdin);
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
}

void listMessages() {}
/*###############################################*/















