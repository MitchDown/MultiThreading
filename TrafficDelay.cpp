#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <queue>

using namespace std;


struct carData
{
        int carID;
        time_t arriveTime;
};
ofstream carWriter;
ofstream flagWriter;
int numCars;
int maxCars;
int createdCars;
int nextID;
bool carThreadsActive;
bool northSouth; //letting north cars through when true, south when false                                      
queue <carData> northCars;
queue <carData> southCars;
sem_t carSem;
pthread_mutex_t carHolderLock;
int pthread_sleep(int seconds);
void* NCarThread(void* args);
void* SCarThread(void* args);
void* FlaggerThread(void* args);
void RoadBlockHandler();

int main(int argc, char* argv[])
{
    carThreadsActive = true;
    numCars = 0;
    createdCars = 0;
    maxCars = atoi(argv[1]);
    nextID = 1;
    northSouth = true;
    sem_init(&carSem, 0, 0);
    srand(time(0));
    pthread_mutex_init(&carHolderLock, NULL);
    cout << "fuck the files\n";
    carWriter.open("car.log");
    carWriter << "carID    direction    arrival-time    start-time    end-time\n";
	flagWriter.open("flagperson.log");
	flagWriter << "Time    State\n";
	RoadBlockHandler();
}

//This function copied from Project3.pdf by Dr. Zhu, it is not my work.
int pthread_sleep(int seconds)
{
	pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
	{
		return -1;
	}
    if(pthread_cond_init(&conditionvar,NULL))
	{
		return -1;
	}
	//When to expire is an absolute time, so get the current time and add
	//it to our delay time
	timetoexpire.tv_sec = (unsigned int)time(NULL) + seconds; 
	timetoexpire.tv_nsec = 0;
	return pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
}

void RoadBlockHandler()
{
	pthread_t nCar;
	pthread_t sCar;
	pthread_t flagger;
	pthread_create(&nCar, NULL, &NCarThread, NULL);
	pthread_create(&sCar, NULL, &SCarThread, NULL);
	pthread_create(&flagger, NULL, &FlaggerThread, NULL);
}

void* NCarThread(void* args)
{
        int carChance = 0;
        while(carThreadsActive)
        {
                carChance = (rand() % 10);
                if(carChance < 8)
                {
                        carData* newCar = new carData;
                        newCar->arriveTime = time(0);
                        pthread_mutex_lock(&carHolderLock);
                        newCar->carID = nextID;
                        northCars.push(*newCar);
                        sem_post(&carSem);
                        nextID++;
                        numCars++;
                        createdCars++;
                        if(createdCars > maxCars)
                          carThreadsActive = false;
                        pthread_mutex_unlock(&carHolderLock);
                }
                else
                        pthread_sleep(20);
        }
        return NULL;
}


void* SCarThread(void* args)
{
        int carChance = 0;
        while(carThreadsActive)
        {
                carChance = (rand() % 10);
                if(carChance < 8)
                {
                        carData* newCar = new carData;
                        newCar->arriveTime = time(0);
                        pthread_mutex_lock(&carHolderLock);
                        newCar->carID = nextID;
                        southCars.push(*newCar);
                        sem_post(&carSem);
                        nextID++;
                        numCars++;
                        createdCars++;
                        if(createdCars > maxCars)
                          carThreadsActive = false;
                        pthread_mutex_unlock(&carHolderLock);
                }
                else
                        pthread_sleep(20);
        }
        return NULL;
}

void* FlaggerThread(void* args)
{
        bool flaggerActive = true;
        int tempNumCars;
        carData tempCar;
        tempCar.carID = 0;
        tempCar.arriveTime = time(0);
        char carDirection;
        while(createdCars < maxCars || tempNumCars > 0)
        {
                pthread_mutex_lock(&carHolderLock);
                tempNumCars = numCars;
                pthread_mutex_unlock(&carHolderLock);
                if(tempNumCars == 0)
                {
                        flaggerActive = false;
                        flagWriter << time(0) << "    sleep\n";
                }
                sem_wait(&carSem);
                if(!flaggerActive)
                {
                        flaggerActive = true;
                        flagWriter << time(0) << "    woken-up\n";
                }
                pthread_mutex_lock(&carHolderLock);
                if(northSouth)
                {

					if(northCars.empty())
                        northSouth = false;
                    else
                    {
                        tempCar = northCars.front();
                        northCars.pop();
                    }
                }
                else
                {
                        if(southCars.empty())
                                northSouth = true;
                        else
                        {
                               tempCar = southCars.front();
                               southCars.pop();
                        }
                }
                numCars--;
                pthread_mutex_unlock(&carHolderLock);
                if(northSouth)
                        carDirection = 'N';
                else
                        carDirection = 'S';
                carWriter << tempCar.carID << "    " << carDirection << "    " << tempCar.arriveTime << "    " << time(0) <<\
 "    " << time(0) + 1 << endl;
                pthread_sleep(1);

	}
	flagWriter.close();
	carWriter.close();
	return NULL;
}