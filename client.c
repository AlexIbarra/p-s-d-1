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
		printf("4) Enviar petición de amistad\n");
		printf("5) Borrar a un amigo\n");
		printf("6) Ver mensajes enviados\n");
		printf("7) Ver mensajes recividos\n");
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
		default:
			break;
	}
	
	return status;		
}



int newUser() {
	int res;
	//struct UserServer myUser;
	
	// Reservamos espacio para el nombre
	//myUser.name = (xsd__string) malloc (IMS_MAX_NAME_SIZE);
	
	// Copiamos el nombre de usuario recogido a la estructura
	//strcpy (myUser.name, user);
	
	// Hacemos la llamada a la funcion newUser de gsoap para pasarle la info al servidor
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
