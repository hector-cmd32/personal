#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>


typedef struct{
	
	char name[20];
	int sock;
	
}TUser;


typedef struct{
	
	int num;
	TUser user[40];
	
}TListaUsers;

TListaUsers lista_usuarios;

int contador; //Tiene que ser global para que todos los threads puedean incrementarlos

//Estructura necesaria para acceso excluyente
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int Pon(TListaUsers *lista, char nombre[20], int socket){
	//Añade nuevo usuario. Retorna 0  si ok y -1 si la lista ya esta llena
	//y no se puede añadir
	
	if (lista->num == 40)
		
		return -1;
		
	else{
		
		strcpy(lista->user[lista->num].name, nombre);
		lista->user[lista->num].sock = socket;
		lista->num = lista->num + 1;
		return 0;
		
	}
}
	
int Eliminar(TListaUsers *lista, char nombre[20]){
	//Retorna 0 si elimina y -1 si dicho usuario no está en la lista
	
	int i = 0;
	int encontrado = 0;
	
	while ((i < lista->num) && (!encontrado)){
		
		if (strcmp(lista->user[i].name, nombre) == 0)
			encontrado = 1;
		if (!encontrado)
			i = i + 1;
	}
	if (encontrado == 1){
		
		int j;
		for(j = i; j < lista->num-1; j++){
			
			strcpy(lista->user[j].name, lista->user[j+1].name);
			lista->user[j].sock = lista->user[j+1].sock;
		}
		lista->num--;
		return 0;
	}
	else
		return -1;
	
}

void DameConectados (TListaUsers *lista, char conectados[400]){
	//Recibe la lista de conectados y retorna un vector de caracteres con los nombres
	//separados por /. --> "3/Juan/Maria/Pedro"
	
	sprintf(conectados, "%d", lista->num);
	
	int i;
	
	for (i = 0; i < lista->num; i++){
		
		sprintf(conectados, "%s/%s", conectados, lista->user[i].name);
		
	}		
}

void *AtenderCliente(void *socket){

	int sock_conn;
	int *s;
	s = (int *) socket;
	sock_conn = *s;
	
	char peticion[512];
	char respuesta[512];
	char user_name[20];
	
	int ret;
	int terminar = 0;
	
	// Entramos en un bucle para atender todas las peticiones de este cliente
	//hasta que se desconecte
	while (terminar ==0)
	{			
		// Ahora recibimos la petici?n
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf ("Recibido\n");
		
		// Tenemos que a?adirle la marca de fin de string 
		// para que no escriba lo que hay despues en el buffer
		peticion[ret]='\0';
		
		printf ("Peticion: %s\n",peticion);
		
		// vamos a ver que quieren
		char *p = strtok( peticion, "/");
		int codigo =  atoi (p);
		// Ya tenemos el c?digo de la petici?n
		char nombre[20];
		char copia_nombre[20];
		
		if ((codigo !=0) && (codigo != 6)){
			
			p = strtok( NULL, "/");
			
			strcpy (nombre, p);
			// Ya tenemos el nombre
			printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
			
			//Añadimos el nuevo usuario a la lista
			
			strcpy(user_name, nombre);
			
			pthread_mutex_lock(&mutex); //No me interrumpas ahora
				
			Pon(&lista_usuarios, user_name, sock_conn);
				
			pthread_mutex_unlock(&mutex); //Ya me puedes interrumpir
			
			//Hacemos que cada vez que entra un nuevo usuario, se muestren
			//en el servidor los usuarios conectados
			
			char listado_usuarios[400];
			
			DameConectados(&lista_usuarios, listado_usuarios);
			
			printf("%s \n", listado_usuarios);
			
		}
		
		if (codigo ==0){//petici?n de desconexi?n
			
			terminar=1;
			
			//Eliminamos al usuario que quiere salir del servidor
			
			pthread_mutex_lock(&mutex); //No me interrumpas ahora
			
			Eliminar(&lista_usuarios, user_name);
			
			pthread_mutex_unlock(&mutex); //Ya me puedes interrumpir
			
		}
		
		else if (codigo ==1) //piden la longitd del nombre
			
			sprintf (respuesta,"%d",strlen(nombre));
		
		else if (codigo ==2)
			// quieren saber si el nombre es bonito
			
			if((nombre[0]=='M') || (nombre[0]=='S'))
			
			strcpy (respuesta,"SI");
		
			else
				
				strcpy (respuesta,"NO");
			
			else if (codigo ==3)//quiere saber si es alto
			{
				p = strtok( NULL, "/");
				float altura =  atof (p);
				
				if (altura > 1.70)
					
					sprintf (respuesta, "%s: eres alto",nombre);
				
				else
					
					sprintf (respuesta, "%s: eresbajo",nombre);
			}
			else if (codigo ==4){ //quiere saber si su nombre es palindromo
				
				int i = 0; 
				int j = strlen(nombre) - 1; 
				int control = 0;
				
				while (j > i) 
				{ 
					if (nombre[i++] != nombre[j--]) 
					{ 
						sprintf (respuesta, "No :(");	
						printf ("No\n");
						control = 1;
					} 
				} 
				if (control == 0){
					
					sprintf (respuesta, "Si :)"); 
					printf ("Si\n");
					
				}
				
			}
			else if (codigo ==5){ //quiere su nombre en mayusculas
				
				for(int i=0; nombre[i]!='\0'; i++)
				{
					if(nombre[i]>='a' && nombre[i]<='z')
					{
						nombre[i] = nombre[i] - 32;
					}
				}
				sprintf (respuesta, "%s",nombre); 
				printf ("%s\n",nombre);
			}
			else if (codigo ==6){ //piden el numero de servicios
				
				sprintf (respuesta,"%d",contador);
				
			}
			if (codigo !=0)
			{
				if (codigo != 6){
					
					pthread_mutex_lock(&mutex); //No me interrumpas ahora
					
					contador = contador + 1;
					
					pthread_mutex_unlock(&mutex); //Ya me puedes interrumpir
					
				}				
				printf ("Respuesta: %s\n", respuesta);
				// Enviamos respuesta
				write (sock_conn,respuesta, strlen(respuesta));
			}
	}
	// Se acabo el servicio para este cliente
	close(sock_conn);	
}


int main(int argc, char *argv[])
{
	int i = 0;
	
	int sockets[100];
	pthread_t thread[100];
	
	lista_usuarios.num = 0;
	
	contador = 0;
	
	int sock_conn, sock_listen;
	struct sockaddr_in serv_adr;
		
	// INICIALITZACIONS
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creant socket");
	// Fem el bind al port
		
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
	
	// asocia el socket a cualquiera de las IP de la m?quina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	// establecemos el puerto de escucha
	serv_adr.sin_port = htons(9040);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		
		printf ("Error al bind");
	
	if (listen(sock_listen, 3) < 0)
		
		printf("Error en el Listen");
	
	
	// Bucle infinito
	for (;;){
		printf ("Escuchando\n");
		
		sock_conn = accept(sock_listen, NULL, NULL);
		printf ("He recibido conexion\n");
		//sock_conn es el socket que usaremos para este cliente
		
		sockets[i] = sock_conn;
		
		pthread_create(&thread[i], NULL, AtenderCliente, &sockets[i]);
		
		i++;
	}
}
