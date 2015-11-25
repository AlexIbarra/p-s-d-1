#include "soapH.h"
#include "ims.nsmap"
#include <stdio.h>

#define DEBUG_MODE 1
#define VACIO -9999;

#define ACEPTAR 1
#define RECHAZAR 0

char usuario[20];

int menu();

int soyAmigo(char * amigo, struct ListaUsuarios * listaAmigos);

int main(int argc, char **argv){

	struct soap soap;
	struct ListaUsuarios listaPendientes;
	struct ListaUsuarios listaAmigos;
	struct User auxUser;
	struct ListaMensajes listaMensajes;

	char *serverURL;
	char mensaje [FILENAME_MAX];
	char x[2];
	char * login;
	char destino[20];
	char user[20];
	char opcion[10];

	int solicitud;
	int exit = 1;
	int i=0;
	int res,op,op2;

	
	if(argc != 3){
		printf("Login incorrecto. Modo de uso: ./client http://ptoNºPUESTO.fdi.ucm.es:(port) (nick)\n");
	}
	serverURL = argv[1];
	login = argv[2];

	printf("Servidor: %s\n", serverURL);
	printf("Login: %s\n", login);

	soap_init(&soap);
	soap_call_ims__login(&soap, serverURL, "", login, &res);
	if( res == -1){
		printf("El usuario no esta dado de alta.\n");
		printf("Desea darlo de alta? (S/N) \n");
		scanf("%s",x);
		if(strcmp(x,"s")==0){
			soap_call_ims__darAltaUsuario(&soap, serverURL, "", login, &res);
			printf("El usuario %s ha sido dado de alta correctamente\n",login);
		}	
		else{
			printf("Debe iniciar sesion con un usuario dado de alta\n");
			exit=0;
		}		
	}
	else if( res == -2){
		printf("El usuario %s ya está conectado\n",login);
		exit = 0;
	}
	else if( res == -3){
		printf("El usuario %s ha sido dado de baja.\n",login);
		printf("Desea volver a darlo de alta? (S/N).\n");
		scanf("%s",x);
		if(strcmp(x,"s")==0){
			soap_call_ims__darAltaUsuario(&soap, serverURL, "", login, &res);
			printf("El usuario %s ha sido dado de alta correctamente\n",login);
		}	
		else{
			printf("Debe iniciar sesion con un usuario dado de alta\n");
			exit=0;
		}		
	}

	while(exit==1){
		op=menu();
		switch(op){
			case 1:
				printf("Introduzca el nombre de usuario que quiere dar de alta: ");
				scanf("%s",user);
				printf("\n");
				soap_call_ims__darAltaUsuario(&soap, serverURL, "", user, &res);
				if(res == 0){
					printf("Nick en uso. No se pudo dar de alta a %s\n",user);
				}
				else{
					printf("El usuario %s ha sido dado de alta correctamente.\n",user);
				}
			break;

			case 2:
				printf("Introduzca el nombre de usuario que quiere dar de baja: ");
				scanf("%s",user);
				printf("\n");
				soap_call_ims__darBajaUsuario(&soap, serverURL, "",login, user, &res);	
				switch(res){
					case 0:
						printf("Nick no encontrado. No se pudo dar de baja a %s\n",user);
					break;

					case 1:
						printf("Has sido dado de baja correctamente\n");
						printf("Podrás darte de alta intentando loguearte con tu nick antiguo.\n");
						printf("Hasta pronto!\n");
						exit = 0;
					break;

					case 2:
						printf("El usuario %s está conectado y no se le puede dar de baja.\n",user);
					break;

					case 3:
						printf("El usuario %s ha sido dado de baja correctamente\n",user);
					break;
	
				}
	
			break;

			case 3:

			        soap_call_ims__listarAmigos(&soap, serverURL, "", login, &res,&listaAmigos);
				if(listaAmigos.contador > 0){
					printf("Lista de Amigos:\n");
					for(i=0; i < listaAmigos.contador; i++){
						printf("%d.- %s\n",i+1,listaAmigos.listaUsuarios[i].login);
					}
				}
				else{
					printf("No tienes amigos.\n");
				}
					           
			break;

			case 4:
				printf ("Introduce el nombre del usuario al que quieres enviar un mensaje: \n");
				scanf ("%s", destino);
				printf ("Introduce el mensaje para %s: \n",destino);
				scanf (" %[^\n]", mensaje);
				
				soap_call_ims__listarAmigos(&soap, serverURL, "", login, &res,&listaAmigos);				 
				if (soyAmigo (destino, &listaAmigos)){
					soap_call_ims__enviarMensaje (&soap, serverURL, "", login, destino,mensaje ,&res);
					if (res > 0)
						printf ("Mensaje enviado\n");
					else
						printf ("Error: mensaje no enviado\n");
				 }
				 else{
					 printf ("%s no es amigo tuyo o no ha aceptado tu petición de amistad\n", destino);
					 printf ("Si no le has agregado aún, agrégalo.\n", destino);
					 printf ("Si ya le ha agregado espera a que acepte tu petición.\n");
				 }
			break;

			case 5:
				soap_call_ims__recibirMensajes(&soap, serverURL, "", login, &listaMensajes);
				if(listaMensajes.numMensajes==0){
					printf("No tienes mensajes pendientes.\n");
				}
				else{
					for(i = 0 ; i < listaMensajes.numMensajes ; i++){
						printf("%d.- Mensaje de: %s.\n",i+1,listaMensajes.mensajes[i].name);
						printf("%s\n",listaMensajes.mensajes[i].msg);
					}
					soap_call_ims__mensajesRecibidos(&soap, serverURL, "", login, &res);
				}
			break;

			case 6:
				printf("Introduzca el nombre de usuario al que quiere agregar como amigo\n");
				scanf("%s",destino);
				soap_call_ims__listarAmigos(&soap, serverURL, "", login, &res,&listaAmigos);	
			 
				if(!soyAmigo(destino,&listaAmigos)){
					soap_call_ims__anadirAmigo(&soap, serverURL, "", login, destino, &res);
					switch(res){
						case -2:
							printf("El usuario %s no existe.\n",destino);
						break;
						case -1:
							printf("El usuario %s está inactivo y no puede ser agregado como amigo\n",destino);
						break;
						case 1:
							printf("El usuario %s ha recibido la peticion de amistad.\n",destino);
						break;
					}
				}
				else{
					printf("Ya eres amigo de %s.\n",destino);
				}
				
			break;

			case 7:
				soap_call_ims__listarPendientes(&soap, serverURL, "",login, &res, &listaPendientes);

				if(res==-1){
					printf("No tienes amistades pendientes.");
				}
				else{
					printf("Lista de solicitudes de amistad pendientes:\n");

					for(i=0;i<listaPendientes.contador;i++){
						auxUser = listaPendientes.listaUsuarios[i];
						printf("%d.- %s\n",i+1,auxUser.login);
						printf("Aceptar la solicitud? (S/N)\n");
						scanf("%s",opcion);
						while(strcmp(opcion,"s") != 0 && strcmp(opcion,"n") != 0){
							scanf("%s",opcion);
						}
						if(strcmp(opcion,"s")==0){
							soap_call_ims__listarAmigos(&soap, serverURL, "", login, &res,&listaAmigos);	
							if(!soyAmigo(auxUser.login,&listaAmigos)){
								soap_call_ims__tratarSolicitud(&soap, serverURL, "", login, auxUser.login, &res);
								printf("Ahora eres amigo de %s.\n",auxUser.login);
							}
							else{
								printf("Ya eres amigo de %s. No puedes agregarle de nuevo.\n",auxUser.login);
							}

						}
						else{
							printf("Has rechazado la solicitud de %s.\n",listaPendientes.listaUsuarios[i].login);
						}
					}
				}
				
			break;

			case 0:
				soap_call_ims__logout(&soap, serverURL, "", login, &res);
				printf("Hasta pronto!\n");
				exit=0;
				
			break;

			default:
			break;
		}
	}
}

int menu(){

	char opcion[100];
	int op;
	
	sleep(1);

	printf("********************** MENU **********************\n");
	printf("* 1.- Dar de alta a un usuario.                  *\n");
	printf("* 2.- Dar de baja a un usuario.                  *\n");
	printf("* 3.- Listado de usuarios amigos.                *\n");
	printf("* 4.- Enviar mensaje a un amigo.                 *\n");
	printf("* 5.- Comprobar mensajes entrantes.              *\n");
	printf("* 6.- Añadir amigo.                              *\n");
	printf("* 7.- Aceptar o rechazar solicitudes de amistad. *\n");
	printf("* 0.- Salir.                                     *\n");
	printf("**************************************************\n\n");

	printf("Elija la opcion que desee: ");
	
	scanf(" %s", opcion);
	printf("\n");
	op = atoi(opcion);
	
	while(op > 7 || op < 0){
		printf("Opcion incorrecta.\n");
		printf("Elija una opcion de 0 a 7:");
		scanf(" %s", opcion);
		op = atoi(opcion);

	}
	return op;
}

int soyAmigo(char * amigo, struct ListaUsuarios * listaAmigos){

	int found = 0;
	int i = 0;
	if (listaAmigos->contador != 0){
		for(i = 0 ; i < listaAmigos->contador || !found ; i++){
			if(strcmp(amigo , listaAmigos->listaUsuarios[i].login) == 0){
				found = 1;
			}
		}
	}
	return found; 
}


