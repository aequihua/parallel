#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#define SEMILLA 35791246

/* Version secuencial del programa que calcula Pi usando metodos Montecarlo */
/*  Autor: Arturo Equihua
    Fecha:  30 de agosto de 2012  */

/* Constantes */
const int max = 10;
	
/* Vars globales */

/* Funciones globales */
char *timestamp()
{
    char buf[256];
    time_t clock = time(NULL);
    strcpy(buf,ctime(&clock));
    buf[strlen(buf)-1]='\0';
    return (buf);
}

/* funciones principales	*/
float valorPI(unsigned long niter)
{
   double x,y;
   unsigned long i,count=0; /* # of points in the 1st quadrant of unit circle */
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
   return (pi);
}

int main()
{
   unsigned long niter = 100000000;

   printf("[%s] Valor de pi es %f...\n",timestamp(), valorPI(niter));
   
    return (0);
}
