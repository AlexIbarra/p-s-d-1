
#include "soapH.h"
#include "ims.nsmap"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>

#define BACKLOG (100)	
#define MAX_REGISTROS 50

//Estados del usuario

#define CONECTADO 1
#define DESCONECTADO -1
#define INACTIVO -2
 
struct ListaUsuarios registrados;
//struct ListaAmigos pendientes;

int comprobarNick(char * nick);
void salir(int senal);
int guardarListaUsuarios();








int main(int argc, char **argv){ 
   
   // Asigno el handler para el Ctrl + C
   signal(SIGINT,salir);

   // Declaro estructura soap
   struct soap soap; 

   // Inicializo la estructura soap
   soap_init(&soap); 


   // Compruebo que me han pasado bien los argumentos al ejecutar el servidor
   if (argc < 2) 
   { 
      printf("Usage: ./server xxxx (port)\n"); 
      soap_serve(&soap); 
      soap_destroy(&soap);
      soap_end(&soap); 
   } 
   else{ 
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
      cargarListaUsuarios();
 
      for (;;) 
      { 
         s = soap_accept(&soap); 
         if (!soap_valid_socket(s)) 
         { 
            if (soap.errnum) 
            { 
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
   } 
   soap_done(&soap); 
   return 0; 
} 









// ******** FUNCIONES PROPIAS DEL SERVIDOR ********* //

// Handler para la señal Cntrl + C
void salir(int senal){

	int i;
	fflush (stdout);
	switch(senal){
		case SIGINT:
			printf("Cerrando servidor...\n");

			for(i = 0; i< registrados.contador ; i++){
				if(registrados.listaUsuarios[i].estado == CONECTADO){
					registrados.listaUsuarios[i].estado = DESCONECTADO;
				}
			}

			guardarListaUsuarios();
			exit(1);
		break;
	}
}

// Comprueba que el usuario esta en la estructura de "registrados"
int comprobarNick(char * nick){

	int i = 0;

	for(i=0; i < registrados.contador; i++){
		
		if(strcmp(registrados.listaUsuarios[i].login,nick)==0){
			return i;
		}

	}
	return -1;

}

// Guarda en disco una lista de usuarios
int guardarListaUsuarios(){

	FILE * fichero = fopen("serverFiles/listaUsuarios","w");
	if(fichero == NULL){
		printf("Error al abrir el archivo ListaUsuario\n");
		return -1;
	}
	else{
		printf("Guardando listaUsuarios\n");
		fwrite(&registrados, sizeof(struct ListaUsuarios), 1, fichero);
		printf("Lista guardada con éxito\n");
		fclose(fichero);
		return 1;
	}
}

// Carga de disco una lista de usuarios
int cargarListaUsuarios(){

	int i = 0;
	FILE * fichero = fopen("serverFiles/listaUsuarios","r");
	if(fichero == NULL){
		printf("Error al abrir el archivo ListaUsuario, creando uno nuevo.\n");	
		fichero = fopen("serverFiles/listaUsuarios","w");
		fclose(fichero);
		return -1;
	}
	else{
		printf("Cargando listaUsuarios\n");
		fread(&registrados, sizeof(struct ListaUsuarios), 1, fichero);
		printf("Lista cargada con éxito\n");
		fclose(fichero);
		return 1;
	}

}



// ******** FUNCIONES IMS ********* //


void *process_request(void *soap) { 

   pthread_detach(pthread_self()); 
   soap_serve((struct soap*)soap); 
   soap_destroy((struct soap*)soap); 
   soap_end((struct soap*)soap); 
   soap_done((struct soap*)soap); 
   free(soap); 

   return NULL; 
}







int ims__darAltaUsuario(struct soap *soap, char * user, int * result){

	int ocupado;
	char carpeta[100];
	char nombrefpendientes[100];
	char nombrefamigos[100];

	ocupado = comprobarNick(user);
	if(ocupado > -1 && registrados.listaUsuarios[ocupado].estado != INACTIVO){
		printf("Nick en uso (%s). No se pudo dar de alta.\n",user);
		*result = 0;
	}
	else if(registrados.listaUsuarios[ocupado].estado == INACTIVO){
		registrados.listaUsuarios[ocupado].estado = DESCONECTADO;
		printf("El usuario %s ha sido dado de alta correctamente\n",user);
		*result = 1;
	}

	else{
		sprintf(registrados.listaUsuarios[registrados.contador].login, "%s",user);
		registrados.listaUsuarios[registrados.contador].estado = DESCONECTADO;
		registrados.contador++;
		printf("El usuario %s ha sido dado de alta correctamente\n",user);	

		printf("Creando carpeta para usuario...\n");
		
		sprintf(carpeta,"serverFiles/users/%s",user);
		mkdir(carpeta,0777);

		*result = 1;	
	}
	return SOAP_OK;

}

int ims__darBajaUsuario(struct soap *soap,char * login, char * user, int * result){

	int posicion;

	printf("%s intenta dar de baja a %s\n",login,user);

	posicion = comprobarNick(user);	

    if(strcmp(login,user)==0){
		registrados.listaUsuarios[posicion].estado = INACTIVO;
		printf("El usuario %s ha sido dado de baja correctamente\n",user);	
		*result = 1;
	}

	else{
		printf("posicion del usuario: %d\n",posicion);
		if(posicion == -1){
			printf("Nick no encontrado.\n");
			*result = 0;
		}
		else if(registrados.listaUsuarios[posicion].estado == CONECTADO){
			printf("El usuario %s está conectado y no se le puede dar de baja.\n",user);
			*result = 2;
		}	
		else{
			registrados.listaUsuarios[posicion].estado = INACTIVO;
			printf("El usuario %s ha sido dado de baja correctamente\n",user);
			*result = 3;	
		}
	}
	return SOAP_OK;

}

int ims__listarAmigos(struct soap *soap, char * login, int * result, struct ListaUsuarios *listaAmigos){
	
 	printf ("%s pide sus amigos.\n", login);
	int i = 0;
	char nombreFichero [FILENAME_MAX];
	char cadena [FILENAME_MAX];

	FILE * fich = NULL;
	sprintf (nombreFichero, "serverFiles/users/%s/amigos", login); 
	
	fich = fopen (nombreFichero, "r");
	
	if (fich == NULL) 
	{
		printf ("El usuario %s no tiene amigos.\n", login);
		*result = -1;
	}
	else 
	{
		printf ("Recuperando lista de amigos de %s\n", login);
		fscanf (fich, "%s", cadena);
		while (!feof (fich)) 
		{
			sprintf (listaAmigos->listaUsuarios[i].login, "%s", cadena);
			i++;
			fscanf (fich, "%s", cadena);		
		}
		listaAmigos->contador = i;
		fclose (fich);	
		*result = 1;
	}
	return SOAP_OK;
}

int ims__login(struct soap *soap, char * login, int * result){

	int pos;
	char op;

	pos = comprobarNick(login);


	if (pos == -1){
		printf("No existe el usuario %s.\n", login);
		*result = -1;
	}
	else if(registrados.listaUsuarios[pos].estado == CONECTADO){
		printf("El usuario %s ya está conectado. \n", login);
		*result = -2;
		
	}	
	else if (registrados.listaUsuarios[pos].estado == INACTIVO){
		printf("El usuario %s ha sido dado de baja. \n", login);
		*result = -3;
	}
	else {
		printf("Login correcto. Usuario: %s\n",login);
		registrados.listaUsuarios[pos].estado = CONECTADO;
		*result = 1;
	}


	return SOAP_OK;
	
}

int ims__logout(struct soap *soap, char * login, int * result){

	int pos;	

	pos = comprobarNick(login);
	registrados.listaUsuarios[pos].estado=DESCONECTADO;
	printf("Usuario %s desconectado\n",login);

	return SOAP_OK;

}

int ims__anadirAmigo(struct soap *soap, char * login, char * destino, int * result){
	
	printf ("ims__anadir amigo %s quiere ser amigo de %s\n", login, destino);
	int pos;
	char nombreFicheroPendientes [FILENAME_MAX];
	FILE * fich = NULL;
	sprintf (nombreFicheroPendientes, "serverFiles/users/%s/amistadesPendientes", destino);
	
	pos=comprobarNick(destino);
	if (pos == -1){
		printf ("No se puede ser amigo de %s porque no existe.\n", destino);
		*result = -2;
	}
	else if(pos > -1 && registrados.listaUsuarios[pos].estado == INACTIVO){
		printf("No se puede ser amigo de %s porque está Inactivo. \n",destino);
		*result = -1;
		
	}
	else{
		printf ("%s existe. Agregandolo la petición de amistad...\n", destino);
		fich = fopen (nombreFicheroPendientes, "a");
		
		if (fich == NULL)
		{
			printf ("No se puede acceder al fichero de amistades pendientes de %s creando uno nuevo\n", destino);
			fich = fopen (nombreFicheroPendientes, "w");
			fprintf (fich, "%s\n", login);		
			*result = 1;
		}
		else
		{
			fprintf (fich, "%s\n", login);		
			*result = 1;
		}	
		fclose (fich);			
	}
	return SOAP_OK;
}


int ims__tratarSolicitud(struct soap *soap, char * login, char * solicitante, int * result){

	struct ListaUsuarios listaAmigos;
	char cadena [FILENAME_MAX];
	char nombreFichero [FILENAME_MAX];
	char nombreFichero2 [FILENAME_MAX];

	FILE * fich = NULL;
	FILE * fich2 = NULL;	

	sprintf(nombreFichero,"serverFiles/users/%s/amigos",login);
	sprintf(nombreFichero2,"serverFiles/users/%s/amigos",solicitante);

	fich = fopen (nombreFichero, "a");
	fich2 = fopen(nombreFichero2, "a");

	printf ("Abierto el fichero de amigos de %s\n", login);
	printf ("Abierto el fichero de amigos de %s\n", solicitante);

	fprintf (fich, "%s\n", solicitante);
	fprintf (fich2, "%s\n", login);		

	*result = 1;

	printf ("Amistad de %s aceptada por %s\n", login,solicitante);
	printf ("Amistad de %s aceptada por %s\n", solicitante,login);
	
	fclose (fich);	
	fclose (fich2);

	return SOAP_OK;
}

int ims__listarPendientes(struct soap *soap, char * login, int * result, struct ListaUsuarios *listaAmigos){

	int pos = -1;
	int i = 0;
	char nombreFicheroPendientes [FILENAME_MAX];
	char cadena [FILENAME_MAX];
	struct User usuario;
	FILE * fichPendientes = NULL;
	sprintf (nombreFicheroPendientes, "serverFiles/users/%s/amistadesPendientes", login); 
	
	printf ("%s pide sus amigos.\n", login);		
	fichPendientes = fopen (nombreFicheroPendientes, "r"); 
	if (fichPendientes == NULL) 
	{
		printf ("%s no tiene amistades pendientes\n", login); 
		*result = -1;
	}
	else 
	{
		fscanf (fichPendientes, "%s", cadena); 
		while (! feof (fichPendientes))
		{
			sprintf (usuario.login, "%s", cadena);
			listaAmigos->listaUsuarios[i] = usuario;
			++i;
			fscanf (fichPendientes, "%s", cadena); 		
		}
		listaAmigos->contador = i;
		fclose (fichPendientes);
		*result = 1;
		remove(nombreFicheroPendientes);
	}


 return SOAP_OK;
}

int ims__enviarMensaje (struct soap *soap,char * login, char * destino, char * msg, int * resul){
	printf ("Servidor: Enviar mensajea %s de %s\n", destino, login);
	
	int pos; 
	FILE * fich; 
	char rutaFichero [FILENAME_MAX];
	
	pos = comprobarNick(destino);
	if ( pos >= 0 ){
		sprintf (rutaFichero, "serverFiles/users/%s/%s", destino,login); 
		
		fich = fopen (rutaFichero, "a");
		fprintf (fich, "%s\n", msg);
		fflush (fich);
		fclose (fich);
		
		*resul = 1;
	}
	else 
	{
		printf ("Usuario destino no encontrado, no se puede realizar la operacion\n");
		*resul = -1;
	}	
	return SOAP_OK;	
}

int ims__recibirMensajes(struct soap *soap, char * login, struct ListaMensajes * listaMsg){
	
	printf ("%s pide sus mensajes. \n", login);

	char nombreCarpeta [FILENAME_MAX];
	char nombreFichero [FILENAME_MAX];
	char nombreUsuario [20];
	char mensaje [144];
	DIR *dir;
	FILE * fich;
	struct dirent *ent;
	int i = 0;
	
	sprintf (nombreCarpeta, "serverFiles/users/%s", login);
	dir = opendir (nombreCarpeta);
	
	if (dir != NULL) {

	  while ((ent = readdir (dir)) != NULL) {
		if ((strcmp (ent->d_name, "..") == 0) || (strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "amistadesPendientes") == 0)|| (strcmp(ent->d_name, "amigos") == 0)) {

		}
		else{
			printf ("%s\n", ent->d_name);
			sprintf (nombreUsuario, "%s", ent->d_name);
			sprintf (nombreFichero, "serverFiles/users/%s/%s", login, nombreUsuario);
			printf ("Fichero al que accedo: %s\n", nombreFichero);
			fich = fopen (nombreFichero, "r");
		
			if (fich != NULL){	
				fscanf (fich, " %[^\n]", mensaje);
				while (!feof (fich)){
					sprintf((*listaMsg).mensajes[i].name, "%s", nombreUsuario); //se rellena el nombre del origen del msg
					sprintf((*listaMsg).mensajes[i].msg, "%s", mensaje);					
					++i;
					fscanf (fich, " %[^\n]", mensaje);
					printf ("Mensaje que se enviara al destino: %s: %s\n", nombreUsuario, mensaje);
				}
				fclose (fich);
			}
		}
	  }
	  (*listaMsg).numMensajes = i;
	  printf ("Num mensajes total: %d\n", (*listaMsg).numMensajes);
	  closedir (dir);
	}
	else{
	  perror ("");
	  return SOAP_OK;
	}
	return SOAP_OK;

}

int ims__mensajesRecibidos (struct soap *soap, char * login, int * result){

	char nombreCarpeta [FILENAME_MAX];
	char nombreFichero [FILENAME_MAX];
	char nombreUsuario [20];
	char mensaje [144];

	DIR *dir;
	FILE * fich;
	struct dirent *ent;
	int i = 0;
	
	sprintf (nombreCarpeta, "serverFiles/users/%s", login);
	dir = opendir (nombreCarpeta);
	
	if (dir != NULL) {

	  while ((ent = readdir (dir)) != NULL) {
		if ((strcmp (ent->d_name, "..") == 0) || (strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "amistadesPendientes") == 0)|| (strcmp(ent->d_name, "amigos") == 0)){

		}
		else{
			sprintf (nombreUsuario, "%s", ent->d_name);
			sprintf (nombreFichero, "serverFiles/users/%s/%s", login, nombreUsuario);
			printf ("Fichero a borrar: %s\n", nombreFichero);
			remove (nombreFichero);
			*result = 1;
		}
	  }	 
	  closedir (dir);
	}
	else{
		  perror ("");
		  *result = -1;
		  return SOAP_OK;	  
	}
	return SOAP_OK;
}

