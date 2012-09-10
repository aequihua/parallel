#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

#define SEMILLA 35791246

/* Version paralela (MASTER y ESCLAVO) del programa que calcula Pi usando metodos Montecarlo */
/* Esta version usa Handshaking */
/*  Autor: Arturo Equihua - Equipo NP3
    Fecha:  30 de agosto de 2012  */

/* Constantes */
const int compute_tag=1;
const int stop_tag=2;
const int masterproc = 0;
const int req_tag = 3;
unsigned long niter = 100000000; /* numero de iteraciones fijo para calcular */
unsigned long bucket = 1000000; /* numero de elementos a procesar por esclavo */

/* Vars globales */
int numprocs = 0;
int bes_paralelo = 0;
int rank, namelen;  /* variables que usa MPI */
char processor_name[MPI_MAX_PROCESSOR_NAME];
unsigned long i,j;

/* Funciones globales */
char *timestamp()
{
    char buf[256];
    time_t clock = time(NULL);
    strcpy(buf,ctime(&clock));
    buf[strlen(buf)-1]='\0';
    return (buf);
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
  double x,y,z;
  unsigned long ind;
  unsigned long n,count=0;
  double* xr = (double*) malloc(bucket*2*sizeof(double));
  int source_tag;
  MPI_Status stat;

  printf("[%s] [%s] Proceso ESCLAVO %d de %d\n",timestamp(),processor_name,rank,numprocs);
  MPI_Send(&count,1,MPI_UNSIGNED_LONG,masterproc,req_tag,MPI_COMM_WORLD);
  n=bucket*2;
  MPI_Recv(xr, n,MPI_DOUBLE,masterproc,MPI_ANY_TAG, MPI_COMM_WORLD,&stat);
  source_tag = stat.MPI_TAG;
  while (source_tag == compute_tag)
  {
    count=0;
    ind = 0;
    for (i=0;i<bucket;i++)
    {
       x=xr[ind];
       ind++;
       y=xr[ind];
       ind++;
       z= x*x + y*y;
       if (z<=1)
        { count++; 
        }
    }

    MPI_Send(&count,1,MPI_UNSIGNED_LONG,masterproc,req_tag,MPI_COMM_WORLD);
    MPI_Recv(xr, n, MPI_DOUBLE, masterproc,MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    source_tag = stat.MPI_TAG;
  }
}

void proceso_maestro()
{
  /* Mapear */
  double x, y, pi;
  unsigned long count, cntesclavo;
  unsigned long ind, numesclavos;
  MPI_Status stat;
  int source=0;

  double* rands = (double*) malloc(2*bucket*sizeof(double));
  srand(SEMILLA);
  numesclavos = numprocs - 1;
  count = 0;

  if (numesclavos==0)
  {
    printf("[%s] Abortando proceso MASTER, no hay esclavos disponibles",timestamp());
    return;
  }

  printf("[%s] [%s] Proceso MASTER %d de %d\n",timestamp(),processor_name,rank,numprocs);

  for (i=0;i<(niter/bucket);i++)
  {
    ind = 0;
    for (j=0;j<bucket;j++)  /* Va a repartir solo a los esclavos, proceso 1 en delante */
    {
       x=(double)rand()/RAND_MAX; /* valor de X */
       rands[ind] = x;
       ind++;
       y=(double)rand()/RAND_MAX; /* valor de Y */
       rands[ind]=y;
       ind++;
    }
    MPI_Recv(&cntesclavo,1,MPI_UNSIGNED_LONG,MPI_ANY_SOURCE,req_tag,MPI_COMM_WORLD,&stat);
    if (stat.MPI_TAG == req_tag)
    {
       count+=cntesclavo;
       source = stat.MPI_SOURCE;
       MPI_Send(rands,sizeof(rands),MPI_DOUBLE,source,compute_tag,MPI_COMM_WORLD);
    }
  }

  /* Tumbar a todos los slaves */
  for (i=1;i<numprocs;i++)
  {
     MPI_Recv(&cntesclavo,1,MPI_UNSIGNED_LONG,i,req_tag,MPI_COMM_WORLD,&stat);
     MPI_Send(&cntesclavo,1,MPI_UNSIGNED_LONG,i,stop_tag,MPI_COMM_WORLD);
  }
  /* Reducir */
  
  pi = (double) count/niter;
  printf("[%s] El valor de Pi en proceso paralelo = %f\n",timestamp(),pi*4.0);
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

