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

/* Vars globales */
int numprocs = 0;
int bes_paralelo = 0;
int rank, namelen;  /* variables que usa MPI */
long niter = 1000; /* numero de iteraciones fijo para calcular */
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
  double* xr = (double*) malloc(numprocs*2*sizeof(double));
  int source_tag;
  MPI_Status stat;

  printf("[%s] [%s] Proceso ESCLAVO %d de %d\n",timestamp(),processor_name,rank,numprocs);
  MPI_Send(&suma,1,MPI_DOUBLE,masterproc,req_tag,MPI_COMM_WORLD);
  printf("ESCLAVO Recien enviado mensaje %f, %d\n",suma,masterproc); 
  n=numprocs*2;
  MPI_Recv(xr, n,MPI_DOUBLE,masterproc,MPI_ANY_TAG, MPI_COMM_WORLD,&stat);
  source_tag = stat.MPI_TAG;
  printf(" ESCLAVO, antes del while, sourcetag=%d\n",source_tag);
  while (source_tag == compute_tag)
  {
    suma = 0;
    for (i=0;i<(numprocs-1);i++)
    {
       printf("Dentro del for del esclavo, para sumar\n");
       ind = i*2;
       x=xr[ind];
       y=xr[ind+1];
       z= x*x + y*y;
       if (z<=1)
          suma = suma + z;
    }

    MPI_Send(&suma,1,MPI_DOUBLE,masterproc,req_tag,MPI_COMM_WORLD);
    printf("ESCLAVO recien envie la suma %f\n",suma);
    MPI_Recv(xr, n, MPI_DOUBLE, masterproc, source_tag, MPI_COMM_WORLD, &stat);
  }
  /* reduce_add(&suma, Pgroup) */
}

void proceso_maestro()
{
  /* Mapear */
  double suma=0, sumesclavo, x, y;
  unsigned long ind, numesclavos;
  MPI_Status stat;
  int source=0;

  double* rands = (double*) malloc(2*numprocs*sizeof(double));
  srand(SEMILLA);
  numesclavos = numprocs - 1;

  if (numesclavos==0)
  {
    printf("[%s] Abortando proceso MASTER, no hay esclavos disponibles",timestamp());
    return;
  }

  printf("[%s] [%s] Proceso MASTER %d de %d\n",timestamp(),processor_name,rank,numprocs);

  for (i=0;i<(niter/numprocs);i++)
  {
    printf("MAESTRO iteracion No %lu de %lu\n\n",i, (niter/numprocs));
    for (j=1;j<numprocs;j++)  /* Va a repartir solo a los esclavos, proceso 1 en delante */
    {
       ind = (2*(j-1));
       x=(double)rand()/RAND_MAX; /* valor de X */
       printf("x=%f ",x);
       rands[ind] = x;
       y=(double)rand()/RAND_MAX; /* valor de Y */
       printf("y=%f \n",y);
       rands[ind+1]=y;
    }
    MPI_Recv(&sumesclavo,1,MPI_DOUBLE,MPI_ANY_SOURCE,req_tag,MPI_COMM_WORLD,&stat);
    if (stat.MPI_TAG == req_tag)
    {
       printf("MASTER Recibe suma parcial %f, tag %d, fuente %d\n",sumesclavo, stat.MPI_TAG, stat.MPI_SOURCE);
       suma+=sumesclavo;
       source = stat.MPI_SOURCE;
       MPI_Send(rands,sizeof(rands),MPI_DOUBLE,source,compute_tag,MPI_COMM_WORLD);
       printf("MASTER envia el arreglo rands, tamano %d\n",sizeof(rands));
    }
  }

  /* Tumbar a todos los slaves */
  for (i=1;i<numprocs;i++)
  {
     printf("Mandando tumbar los procesos\n");
     MPI_Recv(&sumesclavo,1,MPI_DOUBLE,i,req_tag,MPI_COMM_WORLD,&stat);
     MPI_Send(&sumesclavo,1,MPI_DOUBLE,i,stop_tag,MPI_COMM_WORLD);
  }
  /* Reducir */
  /* reduce_add(&suma, Pgroup) */
  printf("El valor de Pi en proceso paralelo = %f\n",suma);
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

