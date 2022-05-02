#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>


#define FALHA 1

#define	TAM_MEU_BUFFER	1000

int cria_socket_local(void)
{
	int socket_local;		/* Socket usado na comunicac�o */

	socket_local = socket( PF_INET, SOCK_DGRAM, 0);
	if (socket_local < 0) {
		perror("socket");
		return -1;
	}
	return socket_local;
}

struct sockaddr_in cria_endereco_destino(char *destino, int porta_destino)
{
	struct sockaddr_in servidor; 	/* Endere�o do servidor incluindo ip e porta */
	struct hostent *dest_internet;	/* Endere�o destino em formato pr�prio */
	struct in_addr dest_ip;		/* Endere�o destino em formato ip num�rico */

	if (inet_aton ( destino, &dest_ip ))
		dest_internet = gethostbyaddr((char *)&dest_ip, sizeof(dest_ip), AF_INET);
	else
		dest_internet = gethostbyname(destino);

	if (dest_internet == NULL) {
		fprintf(stderr,"Endereco de rede invalido\n");
		exit(FALHA);
	}

	memset((char *) &servidor, 0, sizeof(servidor));
	memcpy(&servidor.sin_addr, dest_internet->h_addr_list[0], sizeof(servidor.sin_addr));
	servidor.sin_family = AF_INET;
	servidor.sin_port = htons(porta_destino);

	return servidor;
}

void envia_mensagem(int socket_local, struct sockaddr_in endereco_destino, char *mensagem)
{
	/* Envia msg ao servidor */

	if (sendto(socket_local, mensagem, strlen(mensagem)+1, 0, (struct sockaddr *) &endereco_destino, sizeof(endereco_destino)) < 0 )
	{
		perror("sendto");
		return;
	}
}


int recebe_mensagem(int socket_local, char *buffer, int TAM_BUFFER)
{
	int bytes_recebidos;		/* N�mero de bytes recebidos */

	/* Espera pela msg de resposta do servidor */
	bytes_recebidos = recvfrom(socket_local, buffer, TAM_BUFFER, 0, NULL, 0);
	if (bytes_recebidos < 0)
	{
		perror("recvfrom");
	}

	return bytes_recebidos;
}

float read_boiler_level(int local_socket, struct sockaddr_in destination_address)
{
	char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "nivelboiler");
	nrec = recebe_mensagem(local_socket, response, 1000);

    //printf("Boiler level response: %s", response);

    char value[6];
    int i;

    for(i = 13; i < 19; i++)
    {
        value[i-13] = response[i];
    }

    //printf("\t\t Boiler level value: %f\n\n", (float)atof(value));

    return (float)atof(value);
}

float read_boiler_temperature(int local_socket, struct sockaddr_in destination_address)
{
   char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "tempboiler");
	nrec = recebe_mensagem(local_socket, response, 1000);

    //printf("Boiler temperature response: %s", response);

    char value[6];
    int i;

    for(i = 12; i < 18; i++)
    {
        value[i-12] = response[i];
    }

    //printf("\t\t Boiler temperature value: %f\n\n", (float)atof(value));

    return (float)atof(value);
}

float read_plumbing_temperature(int local_socket, struct sockaddr_in destination_address)
{
    char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "tempcanos");
	nrec = recebe_mensagem(local_socket, response, 1000);

    //printf("plumbing temperature response: %s", response);

    char value[6];
    int i;

    for(i = 11; i < 17; i++)
    {
        value[i-11] = response[i];
    }

    //printf("\t\t Plumbing temperature value: %f\n\n", (float)atof(value));

    return (float)atof(value);
}

float read_colector_temperature(int local_socket, struct sockaddr_in destination_address)
{
    char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "tempcoletor");
	nrec = recebe_mensagem(local_socket, response, 1000);

    //printf("collector temperature response: %s", response);

    char value[6];
    int i;

    for(i = 13; i < 18; i++)
    {
        value[i-13] = response[i];
    }

    //printf("\t\t collector temperature value: %f\n\n", (float)atof(value));

    return (float)atof(value);
}

void turn_on_colector_pump(int local_socket, struct sockaddr_in destination_address)
{   

	envia_mensagem(local_socket, destination_address, "bombacoletor 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_recirculation_pump(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "bombacirculacao 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_boiler_heater(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "aquecedor 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_water_inlet(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "valvulaentrada 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_water_outlet(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "valvulaesgoto 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_colector_pump(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "bombacoletor 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_recirculation_pump(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "bombacirculacao 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_boiler_heater(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "aquecedor 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_water_inlet(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "valvulaentrada 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_water_outlet(int local_socket, struct sockaddr_in destination_address)
{
	envia_mensagem(local_socket, destination_address, "valvulaesgoto 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void updateState(int local_socket, struct sockaddr_in destination_address)
{
    float boiler_level = read_boiler_level(local_socket, destination_address);
    float boiler_temperature = read_boiler_temperature(local_socket, destination_address);
    float plumbing_temperature = read_plumbing_temperature(local_socket, destination_address);
    float colector_temperature = read_colector_temperature(local_socket, destination_address);

    float delta = 0.01;

    float boiler_level_reference = 0.48;
    float boiler_temperature_reference = 26.0;
    float plumbing_temperature_reference = 24.0;

    if( boiler_temperature < boiler_temperature_reference*(1-delta) )
    {
        if ( colector_temperature > boiler_temperature*(1+delta) )
        {
            turn_on_colector_pump(local_socket, destination_address);
        }
        else if( boiler_temperature > boiler_temperature_reference*(1+delta) )
        {
            turn_off_colector_pump(local_socket, destination_address);
        }
    }
    else  if( boiler_temperature > boiler_temperature_reference*(1+delta) )
    {
        turn_off_colector_pump(local_socket, destination_address);
    }

    if (plumbing_temperature < plumbing_temperature_reference*(1-delta))
    {
        if (boiler_temperature > plumbing_temperature*(1+delta))
        {
            turn_on_recirculation_pump(local_socket, destination_address);
        }
        else if (plumbing_temperature > plumbing_temperature_reference*(1+delta))
        {
            turn_off_recirculation_pump(local_socket, destination_address);
        }
    }
    else if (plumbing_temperature > plumbing_temperature_reference*(1+delta))
    {
        turn_off_recirculation_pump(local_socket, destination_address);
    }

    if( boiler_temperature < boiler_temperature_reference*(1-delta) )
    {
        turn_on_boiler_heater(local_socket, destination_address);
    }
    else if( boiler_temperature > boiler_temperature_reference*(1+delta))
    {
        turn_off_boiler_heater(local_socket, destination_address);
    }

    if( boiler_level < boiler_level_reference*(1-delta) )
    {
        turn_on_water_inlet(local_socket, destination_address);
    }
    else if (boiler_level > boiler_level_reference*(1+delta))
    {
        turn_off_water_inlet(local_socket, destination_address);
    }

    if( boiler_level > boiler_level_reference*(1+2*delta) )
    {
        turn_on_water_outlet(local_socket, destination_address);
    }
    else if( boiler_level < boiler_level_reference*(1-0.5*delta) )
    {
        turn_off_water_outlet(local_socket, destination_address);
    }

}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr,"Uso: teste-controle <endereco> <porta> \n");
		fprintf(stderr,"<endereco> eh o DNS ou IP do servidor \n");
		fprintf(stderr,"<porta> eh o numero da porta do servidor \n");
		exit(FALHA);
	}

	int porta_destino = atoi( argv[2]);

	int socket_local = cria_socket_local();

	struct sockaddr_in endereco_destino = cria_endereco_destino(argv[1], porta_destino);
        
    struct timespec computationBegging;
    struct timespec computationFinish;
	struct timespec sleepTime;
	struct timespec computationTime;

	int periodNs = 30000000;

    long measuredPeriod;

	sleepTime.tv_sec = 1;
	sleepTime.tv_nsec = 0;

    while(1)
    {
        nanosleep(&sleepTime, NULL);
        measuredPeriod = computationBegging.tv_nsec;
		clock_gettime(CLOCK_MONOTONIC,&computationBegging);
        measuredPeriod = computationBegging.tv_nsec-measuredPeriod;


        printf("period: %ld\n", measuredPeriod);
	
        updateState(socket_local,endereco_destino);

		clock_gettime(CLOCK_MONOTONIC,&computationFinish);

		computationTime.tv_nsec = computationFinish.tv_nsec - computationBegging.tv_nsec;
		computationTime.tv_sec = computationFinish.tv_sec - computationBegging.tv_sec;

        sleepTime.tv_nsec = periodNs - computationTime.tv_nsec;
        sleepTime.tv_sec = 0;
    }
}




