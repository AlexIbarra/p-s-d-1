#include "soapH.h"
#include "imsService.nsmap"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>


#define DEBUG_MODE 1

// MENSAJES
#define SEND 0
#define RECEIVE 1
#define READ 2

// AMIGOS
#define DELFRIEND -1
#define REJECTED -2
#define ACEPTEDFRIEND 1
#define PENDINGFRIEND 0


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
void listRequest(char *user);
void newMessage();
void listMessages(char *user, int option);
void reactivateUser(char *user);




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
	login(conectedUser);

	while(status) {

		// Mostramos el menu y recogemos la opcion
		opcion = menu();
		status = gestorMenu(opcion);
	}
  	
	// Clean the environment
  	soap_end(&soap);
  	soap_done(&soap);

  return 0;
}

int menu(){
		
	char opcion[10];
	int value;
	
	printf("\n#############################################\n");
	printf("#######  %s, selecciona una opción    #####\n" , conectedUser);
	printf("#############################################\n");
	printf("|| 1) Ver mensajes recibidos               ||\n");
	printf("|| 2) Ver mensajes enviados                ||\n");
	printf("|| 3) Enviar un mensaje                    ||\n");	
	printf("|| 4) Enviar solicitud de amistad          ||\n");
	printf("|| 5) Ver/aceptar solicitudes de amistad   ||\n");
	printf("|| 6) Listado de amigos                    ||\n");
	printf("|| 7) Borrar a un amigo                    ||\n");	
	printf("|| 8) Darte de baja                        ||\n");
	printf("|| 9) Cerrar sesion                        ||\n");
	printf("#############################################\n");
	printf("\nOpcion: ");
	scanf("%s", opcion);

	value = atoi(opcion);
	printf("\n");
	
	return value;		
}

int gestorMenu(int op) {
	
	int status = 1;
	char user[IMS_MAX_NAME_SIZE];
	int *res;
	
	switch(op) {
		/*case 1: // Dar de alta usuario
			printf("Introduzca nombre de usuario: ");
			scanf("%s", user);		
			newUser(user);
			printf("\n");
			break; */	
		case 1: //Ver mensajes enviados
			listMessages(conectedUser, RECEIVE);
			break;
		case 2: //Ver mensajes recibidos
			listMessages(conectedUser, SEND);
			break;
		case 3: //Enviar un mensaje
			newMessage(conectedUser);
			break;
		case 4: //Añadir un amigo
			printf("Introduzca nombre del usuario que quiere añadir como amigo: ");
			scanf("%s", addFriend);		
			newFriend(addFriend);
			printf("\n");
			break;
		case 5: //Ver solicitudes de amistad
			listRequest(conectedUser);
			break;
		case 6: //Listar amigos
			listFriends(conectedUser);
			break;		
		case 7: //Borrar un amigo
			printf("Introduzca nombre del amigo que quieres eliminar: ");
			scanf("%s", user);
			deleteFriend(user);
			break;
		case 8: // Dar de baja usuario
			printf("Procediendo a dar de baja al usuario... \n");			
			deleteUser(conectedUser);
			status = 0;
			break;
		case 9: //Cerrar sesion
			logout(conectedUser);
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
  		printf("\n====>    Añadido usuario %s    <====\n", user);
  	}
	else if(res == 1) { // Usuario ya existe
		printf("\n====>    El usuario %s ya existe    <====\n", user);
	}
	else if(res == 2) {

		printf("\n====>    Bienvenido %s    <====\n", user);
	}
	else if(res == -1) { // Usuario eliminado
		printf("\n====>    El usuario %s ha sido dado de baja    <====\n", user);
		printf("¿Quiere volver a dar de alta a este usuario? (s/n): ");
		scanf("%c", option);
		printf("\n");

		if(option == 's') {
			reactivateUser(user);
		}
	}
	else if(res == -2) { // Lista llena
		printf("\n====>    Lista de usuarios llena    <====\n\n");
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

  	if(res == 0) {
		printf("Usuario %s eliminado correctamente\n", userdel);
	}
	else if(res == -1) {
		printf("El usuario %s no existe\n", userdel);
	}
}

void login(char *user) {

	int res;
	char opcion[10];

	soap_call_ims__login(&soap, serverURL, "", user, &res);

	if(res == -2) {

		printf("\n====>    El usuario %s no esta dado de alta    <====\n", user);
		printf("¿Quiere darlo de alta? (s/n): ");
		scanf("%s", opcion);

		if(!strcmp(opcion, "s"))
			newUser(user);
		//else
			//salimos

	}
	else if(res == -1) {
		printf("\n====>    El usuario %s ha sido dado de baja    <====\n", user);
		printf("¿Quiere volver a dar de alta a este usuario? (s/n): ");
		scanf("%s", opcion);
		printf("\n");

		if(!strcmp(opcion, "s"))
			reactivateUser(user);
	}
	else if(res == 0)
		printf("\n====>    Bienvenido %s    <====\n", user);
	else if(res == 1) {
		printf("\n====>    Este usuario ya se encuentra conectado    <====\n");
		exit(0);
	}
}

void reactivateUser(char *user) {

	int res;

	soap_call_ims__reactivate(&soap, serverURL, "", user, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

	if(res == 0) {
		printf("Usuario %s reactivado correctamente\n", user);
	}
	else if(res == -1) {
		printf("El usuario %s no existe\n", user);
	}
}

void logout(char *user) {

	int res;

	soap_call_ims__logout(&soap, serverURL, "", user, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

	if(res == -1) {
		printf("El usuario %s no existe\n", user);
	}
	else {
		printf("Hasta pronto %s\n", user);
	}
}
/*##################################*/


/*########### TRATAMIENTO DE AMIGOS ###########*/

void listFriends(char *user){
	
	int res, i;
	struct ListFriends friends;

	soap_call_ims__listFriends (&soap, serverURL, "", user, &friends);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

  	if(friends.result == 0) {

  		printf("Numero de amigos %d\n", friends.numfriends);

  		for(i=0; i<friends.numfriends; i++) {
  			//if(friends.listfriends[i].state != -1)
				printf("- %s \n", friends.listfriends[i].friends);
		}
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
		printf("\n====>    El usuario %s no existe    <====\n", userfriend);
	}
	else if(res == -3) {
  		printf("\n====>    El usuario %s no puede añadir mas amigos    <====\n", userfriend);
  	}
  	else if(res == -4) {
  		printf("\n====>    No tienes sitio para añadir mas amigos    <====\n");
  	}
  	else if(res == -5) {
  		printf("\n====>    No te puedes añadir como amigo a ti mismo    <====\n");
  	}
  	else if(res == -6) {
  		printf("\n====>    Ya tienes una peticion de amistad para el usuario %s    <====\n", userfriend);
  	}
	else if(res == 0) {
  		printf("\n====>    Solicitud de amistad a %s realizada correctamemte    <====\n", userfriend);
  	}
  	else {
  		printf("\n====>    El usuario %s ya estaba en su lista de amigos    <====\n", userfriend);
  	}
}

void deleteFriend(char * name) {

	int res;
	soap_call_ims__deleteFriend (&soap, serverURL, "", conectedUser, name, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

	if (res == 0){
		printf("\n====>    El usuario %s ha sido eliminado de tu lista de amigos    <====\n", name);
	}
	else if(res == -1) {
		printf("\n====>    El usuario %s no existe    <====\n", name);
	}
	else if(res == -2) {
		printf("\n====>    El usuario %s no esta en tu lista de amigos    <====\n", name);
	}
}

void listRequest(char *user){

	int res, i, value,salir=0;
	char op[1];
	char opcion[1];
	struct RequestList requestlist;

	soap_call_ims__listFriendRequest (&soap, serverURL, "", user, &requestlist);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

  	if(requestlist.result == 0) {

  		printf("\nNumero de peticiones %d:\n", requestlist.numrequest);

  		for(i=0; i<requestlist.numrequest; i++) {

  			if(requestlist.request[i].state == DELFRIEND)
				printf(" (%d)- %s : ELIMINADO\n", i ,requestlist.request[i].emisor);
			else if(requestlist.request[i].state == ACEPTEDFRIEND)
				printf(" (%d)- %s : ACEPTADO\n", i ,requestlist.request[i].emisor);
			else if(requestlist.request[i].state == REJECTED)
				printf(" (%d)- %s : RECHAZADO\n", i ,requestlist.request[i].emisor);
			else if(requestlist.request[i].state == PENDINGFRIEND)
				printf(" (%d)- %s : PENDIENTE\n", i ,requestlist.request[i].emisor);
		}

		if(requestlist.numrequest > 0) {
			//Ahora gestionamos el tema de aceptarlas
		  	printf("\nIntroduce el número de la petición que quieres tratar o pusla (s) para salir: ");
		  	scanf("%s", opcion);

		  	/* Si no quiere salir */
		  	if(strcmp(opcion, "s") != 0) {

				value = atoi(opcion);

				
				printf("\n¿Aceptar (a) o rechazar (r)? ");
				scanf("%s", op);

				printf("\n");

				do {
				
					if(strcmp(op, "a") || strcmp(op, "A")){
						//Enviamos la petición al servidor para que lo acepte:
						soap_call_ims__aceptRequest (&soap, serverURL, "", user, requestlist.request[value], &res);

						if (soap.error) {
						  soap_print_fault(&soap, stderr); 
						  exit(1);
						}

						salir = 1;
					}
					else if (strcmp(op, "r") || strcmp(op, "R")){

						soap_call_ims__rejectRequest (&soap, serverURL, "", user, requestlist.request[value], &res);

						if (soap.error) {
						  soap_print_fault(&soap, stderr); 
						  exit(1);
						}

						salir = 1;
					}
					else {
						printf("Opción incorrecta \n");
						printf("¿Aceptar (a) o rechazar (r)? a/r: ");
						scanf("%s", op);
						/*fflush(stdin);
						op = getchar();*/
						printf("\n");
					}

				} while(!salir);
			}
		}
  	} 	
}
/*############################################*/


/*########### TRATAMIENTO DE MENSAJES ###########*/
void newMessage(char *user){
	int res;
	char receptor[16];
	char message[280];
	struct Message msg;

	printf("Escribe el nombre del receptor del mensaje: ");
	scanf("%s", receptor);
	printf("Escribe el mensaje (Max 280 caracteres): ");
	scanf (" %[^\n]", message);
	fflush(stdin);
	//fgets(message, 280, stdin);

	// Guardo el emisor en la estructura
	strcpy(msg.emisor, user);

	// Guardo el receptor en la estructura
	strcpy(msg.receptor, receptor);

	// Guardo el mensaje en la estructura
	strcpy(msg.msg, message);

	soap_call_ims__sendMessage (&soap, serverURL, "", msg, &res);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
		res = 0;
  	}

  	if(res == 0) {
  		printf("Mensaje enviado correctamente\n");
  	}
  	else if(res == -1) {
  		printf("El usuario %s no existe\n", receptor);
  	}
  	else if(res == -2) {
  		printf("Lo sentimos pero el usuario %s no esta en su lista de amigos\n", receptor);
  	}
}

void listMessages(char *user, int option) {

	struct MessageList mList;
	int i;
	char * estado;


	soap_call_ims__receiveMessage (&soap, serverURL, "", user, &option, &mList);

	// Check for errors...
  	if (soap.error) {
      	soap_print_fault(&soap, stderr); 
		exit(1);
  	}

  	/* Muestro los mensajes del usuario */
  	if(mList.result == 0) {

 		if(option == RECEIVE) {
	  		printf("Tienes %d mensajes recibidos:\n\n", mList.nummessages);
			for(i = 0; i < mList.nummessages; i++) {
				printf("-------------------------------\n");
				printf("De: %s \n", mList.messages[i].emisor);
				printf("Mensaje: %s \n", mList.messages[i].msg);
				printf("-------------------------------\n");
			}
		}
		if(option == SEND) {
	  		printf("Tienes %d mensajes enviados:\n\n", mList.nummessages);
			for(i = 0; i < mList.nummessages; i++) {

				printf("-------------------------------\n");
				printf("Para: %s \n", mList.messages[i].emisor);
				printf("Mensaje: %s \n", mList.messages[i].msg);
				if(mList.messages[i].state == RECEIVE)
					printf("Estado: %s \n", "Recibido");
				else if(mList.messages[i].state == READ)
					printf("Estado: %s \n", "Leido");

				printf("(%d)\n", mList.messages[i].state);
				printf("-------------------------------\n");
			}
		}	

  	}

}
/*###############################################*/















