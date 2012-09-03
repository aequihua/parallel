#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

#define SEMILLA 35791246

/* Version paralela (MASTER y ESCLAVO) del programa que calcula Pi usando metodos Montecarlo */
/* Esta version usa Handshaking */
/*  Autor: Arturo Equihua
    Fecha:  30 de agosto de 2012  */

/* Constantes */
const int compute_tag=1;
const int stop_tag=2;
const int masterproc = 0;
const int req_tag = 3;
const int allprocessors = 99;
const int anyprocessor = 100;

/* Vars globales */
int numprocs = 0;
int bes_paralelo = 0;
int rank, namelen;  /* variables que usa MPI */
long niter = 1000000; /* numero de iteraciones fijo para calcular */
char processor_name[MPI_MAX_PROCESSOR_NAME];
unsigned long i,j;

/* Funciones globales */
char *timestamp()
{
    time_t clock = time(NULL);
    return (ctime(&clock));
}

void init_mpi()
{
    int nr = 0;
    int stat = 0;
    char ***c;

    MPI_Init(&nr, c); /* Se hace inicializacion para parametros vacios */
    stat = MPI_Comm_size(MPI_COMM_WORLD, &numprocs);  /* cuantos procesos hay */

    if (stat==MPI_SUCCESS)
        bes_paralelo=1;
    else
        bes_paralelo=0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  /* que proceso soy yo */
    MPI_Get_processor_name(processor_name, &namelen);  /* como me llamo yo */

    printf("[%s] [%s] Proceso MASTER no. %d de %d\n", timestamp(),processor_name, rank, numprocs);
}

void end_mpi()
{
    MPI_Finalize();
}

/* funciones principales	*/
void  proceso_secuencial()
{
   /* version secuencial que se invoca si el programa detecta que no esta en un cluster */
   double x,y;
   unsigned long count=0; /* # of points in the 1st quadrant of unit circle */
   double z;
   double pi;

   printf("[%s] Inicia calculo...\n",timestamp());

   /* inicializar */
   srand(SEMILLA);
   count=0;

   /* ciclo */   
   for ( i=0; i<niter; i++) {
      x = (double)rand()/RAND_MAX;
      y = (double)rand()/RAND_MAX;
      z = x*x+y*y;
      if (z<=1) count++;   /* va contando los puntos que satisfacen x-cuadrada + y-cuadrada <= 1 */
   }
	  
   /* Como los puntos van sumando pi sobre 4, pi vale  count por 4 */
   pi=(double)count/niter*4;
   printf("[%s] Termina calculo...\n",timestamp());

  /* desplegar resultado */
   printf("Valor encontrado de Pi = %f", pi);
}

void proceso_esclavo()
{
  double suma=0;
  double x,y,z;
  unsigned long ind;
  int n;
  double xr[numprocs*2];
  int source_tag;
  MPI_Status stat;
  /* send(Pmaster, req_tag) */
  MPI_Send(&suma,1,MPI_DOUBLE,masterproc,req_tag,MPI_COMM_WORLD);
 /* recv(xr, &n, Pmaster, source_tag) */
  n=numprocs*2;
  MPI_Recv(&xr, n,MPI_DOUBLE,masterproc,MPI_ANY_TAG, MPI_COMM_WORLD,&stat);
  source_tag = stat.MPI_TAG;
  while (source_tag == compute_tag)
  {
    for (i=0;i<numprocs;i++)
    {
       ind = i*2;
       x=xr[ind];
       y=xr[ind+1];
       z= x*x + y*y;
       if (z<=1)
          suma = suma + z;
    }

    /* send(Pmaster, req_tag) */
    MPI_Send(&suma,1,MPI_DOUBLE,masterproc,req_tag,MPI_COMM_WORLD);
    /* recv(xr, &n, Pmaster, source_tag) */
    MPI_Recv(&xr, n, MPI_DOUBLE, masterproc, source_tag, MPI_COMM_WORLD, &stat);
  }
  /* reduce_add(&suma, Pgroup) */
}

void proceso_maestro()
{
  /* Mapear */
  double rands[2*numprocs];
  double suma;
  unsigned long ind;
  MPI_Status stat;
  int source=0;

  srand(SEMILLA);

  for (i=0;i<(niter/numprocs);i++)
  {
    for (j=0;j<numprocs;j++)
    {
       ind = (2*j);
       rands[ind]=rand()/RAND_MAX; /* valor de X */
       rands[ind+1]=rand()/RAND_MAX; /* valor de Y */
    }
    MPI_Recv(&suma,1,MPI_DOUBLE,MPI_ANY_SOURCE,req_tag,MPI_COMM_WORLD,&stat);
    if (stat.MPI_TAG == req_tag)
    {
       source = stat.MPI_SOURCE;
       MPI_Send(rands,numprocs*2,MPI_DOUBLE,source,compute_tag,MPI_COMM_WORLD);
    }
  }

  /* Tumbar a todos los slaves */
  for (i=1;i<numprocs;i++)
  {
     MPI_Recv(&suma,1,MPI_DOUBLE,i,req_tag,MPI_COMM_WORLD,&stat);
     MPI_Send(&suma,1,MPI_DOUBLE,i,stop_tag,MPI_COMM_WORLD);
  }
  /* Reducir */
  suma = 0;
  /* reduce_add(&suma, Pgroup) */
}

int main()
{
   init_mpi();

   if (bes_paralelo)
  	 if (rank==0)
  	 {
             proceso_maestro();
  	 }
   	else
  	{
     	     proceso_esclavo();
	 }
   else
      proceso_secuencial();
 
   end_mpi();
   
 }

