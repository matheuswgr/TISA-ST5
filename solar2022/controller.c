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
#include <pthread.h>
#include <semaphore.h>

#define FALHA 1

#define	TAM_MEU_BUFFER	1000

int isCollectorPumpOn;
int isRecirculatorPumpOn;
int isBoilerHeaterOn;
int isWaterInletOn;
int isWaterOutletOn;

float boiler_level;
float boiler_temperature;
float plumbing_temperature;
float colector_temperature;

float boiler_level_reference;
float boiler_temperature_reference;
float plumbing_temperature_reference;

pthread_mutex_t sensorValuesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t referenceValuesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t screenMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t tSense;
pthread_t tControlBoilerLevel;
pthread_t tControlBoilerTemperature;
pthread_t tControlPlummingTemperature;
pthread_t tReportState;
pthread_t tUpdateReference;

sem_t* sensingControlRendevouzSemaphore;

int local_socket;

struct sockaddr_in destination_address;

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

    char value[6];
    int i;

    for(i = 13; i < 19; i++)
    {
        value[i-13] = response[i];
    }

    return (float)atof(value);
}

float read_boiler_temperature(int local_socket, struct sockaddr_in destination_address)
{
   char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "tempboiler");
	nrec = recebe_mensagem(local_socket, response, 1000);

    char value[6];
    int i;

    for(i = 12; i < 18; i++)
    {
        value[i-12] = response[i];
    }

    return (float)atof(value);
}

float read_plumbing_temperature(int local_socket, struct sockaddr_in destination_address)
{
    char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "tempcanos");
	nrec = recebe_mensagem(local_socket, response, 1000);

    char value[6];
    int i;

    for(i = 11; i < 17; i++)
    {
        value[i-11] = response[i];
    }

    return (float)atof(value);
}

float read_colector_temperature(int local_socket, struct sockaddr_in destination_address)
{
    char response[1000];
	int nrec;

	envia_mensagem(local_socket, destination_address, "tempcoletor");
	nrec = recebe_mensagem(local_socket, response, 1000);

    char value[6];
    int i;

    for(i = 13; i < 18; i++)
    {
        value[i-13] = response[i];
    }

    return (float)atof(value);
}

void turn_on_colector_pump(int local_socket, struct sockaddr_in destination_address)
{   
    isCollectorPumpOn = 1;
	envia_mensagem(local_socket, destination_address, "bombacoletor 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_recirculation_pump(int local_socket, struct sockaddr_in destination_address)
{
    isRecirculatorPumpOn = 1;
	envia_mensagem(local_socket, destination_address, "bombacirculacao 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_boiler_heater(int local_socket, struct sockaddr_in destination_address)
{
    isBoilerHeaterOn = 1;
	envia_mensagem(local_socket, destination_address, "aquecedor 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_water_inlet(int local_socket, struct sockaddr_in destination_address)
{
    isWaterInletOn = 1;
	envia_mensagem(local_socket, destination_address, "valvulaentrada 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_on_water_outlet(int local_socket, struct sockaddr_in destination_address)
{
    isWaterOutletOn = 1;
	envia_mensagem(local_socket, destination_address, "valvulaesgoto 1");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_colector_pump(int local_socket, struct sockaddr_in destination_address)
{
    isCollectorPumpOn = 0;
	envia_mensagem(local_socket, destination_address, "bombacoletor 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_recirculation_pump(int local_socket, struct sockaddr_in destination_address)
{
    isRecirculatorPumpOn = 0;
	envia_mensagem(local_socket, destination_address, "bombacirculacao 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_boiler_heater(int local_socket, struct sockaddr_in destination_address)
{
    isBoilerHeaterOn = 0;
	envia_mensagem(local_socket, destination_address, "aquecedor 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_water_inlet(int local_socket, struct sockaddr_in destination_address)
{
    isWaterInletOn = 0;
	envia_mensagem(local_socket, destination_address, "valvulaentrada 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void turn_off_water_outlet(int local_socket, struct sockaddr_in destination_address)
{
    isWaterOutletOn = 0;
	envia_mensagem(local_socket, destination_address, "valvulaesgoto 0");
    char response[1000];
	int nrec;
    nrec = recebe_mensagem(local_socket, response, 1000);
}

void sense()
{
    while(1)
    {
        struct timespec computationBegging;
        clock_gettime(CLOCK_MONOTONIC,&computationBegging);    
        
        int periodNs = 30000000;
        
        struct timespec computationFinish;
        struct timespec sleepTime;

        long computationTimeNanoseconds = 0;

        //printf("Sensing\n");
        
        pthread_mutex_lock(&sensorValuesMutex);
        
        boiler_level = read_boiler_level(local_socket, destination_address);
        boiler_temperature = read_boiler_temperature(local_socket, destination_address);
        plumbing_temperature = read_plumbing_temperature(local_socket, destination_address);
        colector_temperature = read_colector_temperature(local_socket, destination_address);
        
        pthread_mutex_unlock(&sensorValuesMutex);

        int semaphoreValue = 0;
        sem_getvalue(sensingControlRendevouzSemaphore, &semaphoreValue);

        //printf("Semaphore Value: %d\n", semaphoreValue);

        if(semaphoreValue <= 0)
        {
            sem_post(sensingControlRendevouzSemaphore);
            sem_post(sensingControlRendevouzSemaphore);
            sem_post(sensingControlRendevouzSemaphore);
        }

        clock_gettime(CLOCK_MONOTONIC,&computationFinish);

        computationTimeNanoseconds = computationFinish.tv_sec*1000000000 - computationBegging.tv_sec*1000000000 + computationFinish.tv_nsec- computationBegging.tv_nsec;

        sleepTime.tv_nsec = periodNs - computationTimeNanoseconds;
        sleepTime.tv_sec = 0;

        nanosleep(&sleepTime, NULL);
    }
}

void initialize()
{
    isCollectorPumpOn = 0;
    isRecirculatorPumpOn = 0;
    isBoilerHeaterOn = 0;
    isWaterInletOn = 0;
    isWaterOutletOn = 0;

    boiler_level_reference = -1;
    boiler_temperature_reference = -1;
    plumbing_temperature_reference = -1;

    boiler_level = read_boiler_level(local_socket, destination_address);
    boiler_temperature = read_boiler_temperature(local_socket, destination_address);
    plumbing_temperature = read_plumbing_temperature(local_socket, destination_address);
    colector_temperature = read_colector_temperature(local_socket, destination_address);
}

void controlBoilerLevel()
{    
    while(1)
    {
        struct timespec computationBegging;
        clock_gettime(CLOCK_MONOTONIC,&computationBegging);    
        
        int periodNs = 30000000;
        
        struct timespec computationFinish;
        struct timespec sleepTime;

        long computationTimeNanoseconds = 0;

        float delta = 0.01;
        
        //printf("Controller 1 waiting \n");
        sem_wait(sensingControlRendevouzSemaphore);
        //printf("Controlling 1\n");
        pthread_mutex_lock(&sensorValuesMutex);
        pthread_mutex_lock(&referenceValuesMutex);

        if(boiler_level_reference != -1)
        {
            if( boiler_level < boiler_level_reference*(1-delta) )
            {
                turn_on_water_inlet(local_socket, destination_address);
            }
            else 
            if (boiler_level > boiler_level_reference*(1+delta))
            {
                turn_off_water_inlet(local_socket, destination_address);
            }

            if( boiler_level > boiler_level_reference*(1+2*delta) )
            {
                turn_on_water_outlet(local_socket, destination_address);
            }
            else 
            if( boiler_level < boiler_level_reference*(1-0.5*delta) )
            {
                turn_off_water_outlet(local_socket, destination_address);
            }
        }

        pthread_mutex_unlock(&referenceValuesMutex);
        pthread_mutex_unlock(&sensorValuesMutex);

        clock_gettime(CLOCK_MONOTONIC,&computationFinish);

        computationTimeNanoseconds = computationFinish.tv_sec*1000000000 - computationBegging.tv_sec*1000000000 + computationFinish.tv_nsec- computationBegging.tv_nsec;

        sleepTime.tv_nsec = periodNs - computationTimeNanoseconds;
        sleepTime.tv_sec = 0;

        nanosleep(&sleepTime, NULL);
    }
}

void controlBoilerTemperature()
{
    while(1)
    {
        struct timespec computationBegging;
        clock_gettime(CLOCK_MONOTONIC,&computationBegging);    
        
        int periodNs = 30000000;
        
        struct timespec computationFinish;
        struct timespec sleepTime;

        long computationTimeNanoseconds = 0;

        float delta = 0.01;

        //printf("Controller 2 waiting \n");
        sem_wait(sensingControlRendevouzSemaphore);
        //printf("Controlling 2\n");

        pthread_mutex_lock(&sensorValuesMutex);
        pthread_mutex_lock(&referenceValuesMutex);


        if(boiler_temperature_reference != -1)
        {
            if( boiler_temperature < boiler_temperature_reference*(1-delta) )
            {
                if ( colector_temperature > boiler_temperature*(1+delta) )
                {
                    turn_on_colector_pump(local_socket, destination_address);
                }
                else 
                if( boiler_temperature > boiler_temperature_reference*(1+delta) )
                {
                    turn_off_colector_pump(local_socket, destination_address);
                }
            }
            else  
            if( boiler_temperature > boiler_temperature_reference*(1+delta) )
            {
                turn_off_colector_pump(local_socket, destination_address);
            }

            if( boiler_temperature < boiler_temperature_reference*(1-delta) )
            {
                turn_on_boiler_heater(local_socket, destination_address);
            }
            else 
            if( boiler_temperature > boiler_temperature_reference*(1+delta))
            {
                turn_off_boiler_heater(local_socket, destination_address);
            }
        }

        pthread_mutex_unlock(&referenceValuesMutex);
        pthread_mutex_unlock(&sensorValuesMutex);

        clock_gettime(CLOCK_MONOTONIC,&computationFinish);

        computationTimeNanoseconds = computationFinish.tv_sec*1000000000 - computationBegging.tv_sec*1000000000 + computationFinish.tv_nsec- computationBegging.tv_nsec;

        sleepTime.tv_nsec = periodNs - computationTimeNanoseconds;
        sleepTime.tv_sec = 0;

        nanosleep(&sleepTime, NULL);
    }
}        

void controlPlummingTemperature()
{
    while(1)
    {
        struct timespec computationBegging;
        clock_gettime(CLOCK_MONOTONIC,&computationBegging);    
        
        int periodNs = 30000000;
        
        struct timespec computationFinish;
        struct timespec sleepTime;

        long computationTimeNanoseconds = 0;

        float delta = 0.01;

        //printf("Controller 3 waiting \n");
        sem_wait(sensingControlRendevouzSemaphore);
        //printf("Controlling 3\n");

        pthread_mutex_lock(&sensorValuesMutex);
        pthread_mutex_lock(&referenceValuesMutex);

        if(plumbing_temperature_reference != -1)
        {
            if (plumbing_temperature < plumbing_temperature_reference*(1-delta))
            {
                if (boiler_temperature > plumbing_temperature*(1+delta))
                {
                    turn_on_recirculation_pump(local_socket, destination_address);
                }
                else 
                if (plumbing_temperature > plumbing_temperature_reference*(1+delta))
                {
                    turn_off_recirculation_pump(local_socket, destination_address);
                }
            }
            else 
            if (plumbing_temperature > plumbing_temperature_reference*(1+delta))
            {
                turn_off_recirculation_pump(local_socket, destination_address);
            }
        }

        pthread_mutex_unlock(&referenceValuesMutex);
        pthread_mutex_unlock(&sensorValuesMutex);

        clock_gettime(CLOCK_MONOTONIC,&computationFinish);

        computationTimeNanoseconds = computationFinish.tv_sec*1000000000 - computationBegging.tv_sec*1000000000 + computationFinish.tv_nsec- computationBegging.tv_nsec;

        sleepTime.tv_nsec = periodNs - computationTimeNanoseconds;
        sleepTime.tv_sec = 0;

        nanosleep(&sleepTime, NULL);
    }
}

void reportState()
{
    while(1)
    {
        struct timespec computationBegging;
        clock_gettime(CLOCK_MONOTONIC,&computationBegging);    
        
        int periodNs = 1000000000;
        
        struct timespec computationFinish;
        struct timespec sleepTime;

        long computationTimeNanoseconds = 0;

        pthread_mutex_lock(&sensorValuesMutex);
        pthread_mutex_lock(&screenMutex);
        pthread_mutex_lock(&referenceValuesMutex);
        
        printf("Boiler Level: %f\n", boiler_level);
        if(boiler_level_reference == -1)
        {
            printf("Desired Boiler Level: not set\n\n");
        }
        else
        {
            printf("Desired Boiler Level: %f\n", boiler_level_reference);
            printf("Boiler Level Error: %f\n\n", boiler_level_reference - boiler_level);
        }

        printf("Boiler Temperature: %f\n", boiler_temperature);
        if(boiler_temperature_reference == -1)
        {
            printf("Desired Boiler Temperature: not set\n\n");
        }
        else
        {
            printf("Desired Boiler Temperature: %f\n", boiler_temperature_reference);
            printf("Boiler Temperature Error: %f\n\n", boiler_temperature_reference-boiler_temperature);
        }

        printf("Plumming Temperature: %f\n", plumbing_temperature);
        if(plumbing_temperature_reference == -1)
        {
            printf("Desired Plumming Temperature: not set\n\n");
        }
        else
        {
            printf("Desired Plumming Temperature: %f\n", plumbing_temperature_reference);
            printf("Plumming Temperature Error: %f\n\n", plumbing_temperature_reference-plumbing_temperature);
        }
        
        printf("Collector Temperature: %f\n\n", colector_temperature);

        printf("Collector Pump State: %d\n", isCollectorPumpOn);
        printf("Recirculator Pump State: %d\n", isRecirculatorPumpOn);
        printf("Boiler Heater State: %d\n", isBoilerHeaterOn);
        printf("Water Inlet State: %d\n", isWaterInletOn);
        printf("Water Outlet State: %d\n\n", isWaterOutletOn);

        pthread_mutex_unlock(&referenceValuesMutex);
        pthread_mutex_unlock(&screenMutex);
        pthread_mutex_unlock(&sensorValuesMutex);

        clock_gettime(CLOCK_MONOTONIC,&computationFinish);

        computationTimeNanoseconds = computationFinish.tv_sec*1000000000 - computationBegging.tv_sec*1000000000 + computationFinish.tv_nsec- computationBegging.tv_nsec;

        sleepTime.tv_nsec = periodNs - computationTimeNanoseconds;
        sleepTime.tv_sec = 0;

        nanosleep(&sleepTime, NULL);                
    }
}

void updateReference()
{
    char buffer[2000];
    
    while(1)
    {
        fgets(buffer,2000,stdin);
        pthread_mutex_lock(&screenMutex);

        int referenceValueOption = 0;

        while(referenceValueOption == 0)
        {
            printf("Choose the reference value you want to set: \n");
            printf("type 1 for Boiler Level \n");
            printf("type 2 for Boiler Temperature \n");
            printf("type 3 for Plumming Temperature \n");
            printf("type 4 to Cancel \n");
                            
            fgets(buffer,2000,stdin);

            referenceValueOption = atoi(buffer);

            if(referenceValueOption < 1 || referenceValueOption > 4)
            {
                printf("Invalid Option\n");
                referenceValueOption = 0;
            }
        }

        int referenceValidity = 0;
        float newReferenceValue = 0;

        while(referenceValidity == 0 && referenceValueOption != 4)
        {
            printf("Type the desired reference value (type -1 to cancel)\n");
            fgets(buffer,2000,stdin);
            newReferenceValue = (float)atof(buffer);

            if(newReferenceValue < 0)
            {
                break;
            }
            
            if(referenceValueOption == 1)
            {
                if(newReferenceValue > 0.6 || newReferenceValue < 0.0)
                {
                    printf("The desired boiler level reference should be smaller than 0.6 and greater than 0\n");
                }
                else
                {
                    referenceValidity = 1;
                    pthread_mutex_lock(&referenceValuesMutex);
                    boiler_level_reference = newReferenceValue;
                    pthread_mutex_unlock(&referenceValuesMutex);
                }
            }
            else
            if(referenceValueOption == 2)
            {
                if(newReferenceValue > 80.0 || newReferenceValue < 1.0)
                {
                    printf("The desired boiler temperature reference should be smaller than 80 and greater than 1\n");
                }
                else
                {
                    referenceValidity = 1;
                    pthread_mutex_lock(&referenceValuesMutex);
                    boiler_temperature_reference = newReferenceValue;
                    pthread_mutex_unlock(&referenceValuesMutex);
                }
            }
            else
            if(referenceValueOption == 3)
            {
                if(newReferenceValue > 80.0 || newReferenceValue < 1.0)
                {
                    printf("The desired plumming temperature reference should be smaller than 80 and greater than 1\n");
                }
                else
                {
                    referenceValidity = 1;
                    pthread_mutex_lock(&referenceValuesMutex);
                    plumbing_temperature_reference = newReferenceValue;
                    pthread_mutex_unlock(&referenceValuesMutex);
                }
            }
        }

        pthread_mutex_unlock(&screenMutex);
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

	local_socket = cria_socket_local();

	destination_address = cria_endereco_destino(argv[1], porta_destino);

    static const char *semaphoreName = "Semaphore";

    sensingControlRendevouzSemaphore = sem_open(semaphoreName, O_CREAT, 0777, 0);

    initialize();

    pthread_create(&tSense, NULL, (void *)sense, NULL);
    pthread_create(&tControlBoilerLevel, NULL, (void *)controlBoilerLevel, NULL);
    pthread_create(&tControlBoilerTemperature, NULL, (void *)controlBoilerTemperature, NULL);
    pthread_create(&tControlPlummingTemperature, NULL, (void *)controlPlummingTemperature, NULL);
    pthread_create(&tReportState, NULL, (void *)reportState, NULL);
    pthread_create(&tUpdateReference, NULL, (void *)updateReference, NULL);

    pthread_join(tSense, NULL);
    pthread_join(tControlBoilerLevel, NULL);
    pthread_join(tControlBoilerTemperature, NULL);
    pthread_join(tControlPlummingTemperature, NULL);
    pthread_join(tReportState, NULL);
    pthread_join(tUpdateReference, NULL);
}



