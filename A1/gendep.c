#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N_DEFAULT 10

void bsort ( int *A, int n )
{
   int i, j, t;

   if (n <= 1) return;
   for (j=n-2; j>=0; --j) {
      for (i=0; i<=j; ++i) {
         if (A[i] > A[i+1]) {
            t = A[i];
            A[i] = A[i+1];
            A[i+1] = t;
         }
      }
   }
}

int main ( int argc, char *argv[] )
{
   int n, *P, *R, **A, *B, i, j, k, t;
   FILE *fp;

   srand((unsigned int)time(NULL));
   n = (argc == 1) ? N_DEFAULT : atoi(argv[1]);

   P = (int *)malloc(n*sizeof(int));
   R = (int *)malloc((n + 1)*sizeof(int));
   for (i=0; i<n; ++i) P[i] = i+1;
   for (i=n-1; i>0; --i) {
      j = rand() % (i + 1);
      t = P[i]; P[i] = P[j]; P[j] = t;
   }
   for (i=0; i<n; ++i) R[P[i]] = i;

   A = (int **)malloc(n * sizeof(int *));
   for (i=0; i<n; ++i) {
      A[i] = (int *)malloc(n * sizeof(int));
      for (j=0; j<n; ++j) A[i][j] = 0;
      for (j=i+1; j<n; ++j) {
         if (rand() % 3 == 0) A[i][j] = 1;
      }
   }

   B = (int *)malloc(n*sizeof(int));
   fp = (FILE *)fopen("foodep.txt", "w");
   fprintf(fp, "%d\n", n);
   for (i=1; i<=n; ++i) {
      t = R[i];
      k = 0;
      for (j=0; j<n; ++j) if (A[t][j]) B[k++] = P[j];
      bsort(B,k);
      fprintf(fp, "%d:", P[t]);
      for (j=0; j<k; ++j) fprintf(fp, " %d", B[j]);
      fprintf(fp, "\n");
   }
   fclose(fp);

   for (i=0; i<n; ++i) free(A[i]);
   free(A);
   free(B);
   free(P);
   free(R);

   exit(0);
}
