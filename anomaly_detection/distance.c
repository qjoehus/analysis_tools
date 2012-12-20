/* Compile: gcc -lm distance.c */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*The algorithm should be fine for more than 2 dimensions,
 * but the output-formatting is not.. Also, the word-lengt
 * limits the value-domain. */

#define TEST_CASES 4
#define TEST_CASE_SIZE 10

#define DATA_DIMENSIONS 2
#define DISTANCE_GRANULARITY 1
#define LARGE_INTEGER 0x7fffffff
#define FILENAME_OUT "in.txt"
#define FILENAME_IN  "out.tex"

typedef struct distance_data_s{
   int id;
   int* points;
   int euclidean_distance_smallest;
   int euclidean_distance_largest;
}distance_data_t;

typedef struct distance_s{
   char filename_in[30];
   char filename_out[30];
   int dimensions;
   int data_size;
   int distance_granularity;
   int euclidean_distance_largest_max;
   int euclidean_distance_largest_min;
   int euclidean_distance_smallest_max;
   int euclidean_distance_smallest_min;
   int distance_smallest_spread;
   int distance_largest_spread;
   int distance_concentration;
   double distance_concentration_factor;
   int* euclidean_distances_smallest;
   double* distance_weight;
   distance_data_t* data;
}distance_t;

void
distance_test_prime(distance_t* state, int test_case, int data_points)
{
   int i;
   int dimension_0[TEST_CASES][TEST_CASE_SIZE] =
      {{0,3,2,3,7,8,9,5,4,8},{0,3,2,3,7,8,9,5,8,8},{0,0,0,0,1,1,1,1,2,2},{0,0,0,0,1,1,1,1,2,8}};
   int dimension_1[TEST_CASES][TEST_CASE_SIZE] =
      {{0,3,5,7,5,3,2,5,7,8},{0,3,1,0,5,6,9,7,7,8},{0,1,2,3,0,1,2,3,1,2},{0,1,2,3,0,1,2,3,1,8}};
   if(test_case >= 0 && test_case < TEST_CASES)
   {
      for(i=0; i < data_points && i < sizeof(dimension_0)/sizeof(int) && i < sizeof(dimension_1)/sizeof(int);i++)
      {
         state->data[i].points[0] = dimension_0[test_case][i];
         state->data[i].points[1] = dimension_1[test_case][i];
      }
   }
}

void
distance_init(distance_t* state, int test_case)
{
   int i;

   state->euclidean_distance_largest_max = 0;
   state->euclidean_distance_largest_min = LARGE_INTEGER;
   state->euclidean_distance_smallest_max = 0;
   state->euclidean_distance_smallest_min = LARGE_INTEGER;
   state->data_size = TEST_CASE_SIZE;
   state->dimensions = DATA_DIMENSIONS;
   state->distance_granularity = DISTANCE_GRANULARITY;
   state->euclidean_distances_smallest = NULL;
   state->distance_weight = NULL;
   state->data = calloc(state->data_size, sizeof(distance_data_t));
   for(i=0; i < state->data_size; i++)
   {
      state->data[i].points = calloc(state->dimensions, sizeof(int));
      state->data[i].euclidean_distance_smallest = LARGE_INTEGER;
      state->data[i].euclidean_distance_largest = 0;
   }
   sprintf(state->filename_in,FILENAME_IN);
   sprintf(state->filename_out,FILENAME_OUT);

   distance_test_prime(state, test_case, state->data_size);
}

int
euclidean_distance(int* A, int* B, int dimensions, int distance_granularity){
   int i;
   double sum = 0.0;

   for(i=0; i < dimensions; i++)
   {
      sum += (double)((A[i] - B[i])*(A[i] - B[i])*distance_granularity);
   }
   return (int)sqrt(sum);
}

void
update_if_smaller(int* a, int v)
{
   if(v < *a)
   {
      *a = v;
   }
}

void
update_if_larger(int* a, int v)
{
   if(v > *a)
   {
      *a = v;
   }
}

void
distance_calculate(distance_t* state)
{
   int i,j;
   int ed, zeros;
   
   for(i=0; i < state->data_size; i++)
   {
      for(j=i+1; j < state->data_size; j++)
      {
         ed = euclidean_distance(state->data[i].points,state->data[j].points,
                                 state->dimensions, state->distance_granularity);
         update_if_larger(&(state->data[i].euclidean_distance_largest), ed);
         update_if_larger(&(state->data[j].euclidean_distance_largest), ed);
         update_if_smaller(&(state->data[i].euclidean_distance_smallest), ed);
         update_if_smaller(&(state->data[j].euclidean_distance_smallest), ed);
      }
      update_if_larger(&(state->euclidean_distance_largest_max), state->data[i].euclidean_distance_largest);
      update_if_smaller(&(state->euclidean_distance_largest_min), state->data[i].euclidean_distance_largest);
      update_if_larger(&(state->euclidean_distance_smallest_max), state->data[i].euclidean_distance_smallest);
      update_if_smaller(&(state->euclidean_distance_smallest_min), state->data[i].euclidean_distance_smallest);
   }
   state->euclidean_distances_smallest =
      calloc(state->euclidean_distance_smallest_max-state->euclidean_distance_smallest_min+1, sizeof(int));
   for(i=0; i < state->data_size; i++)
   {
      state->euclidean_distances_smallest[state->data[i].euclidean_distance_smallest-
                                          state->euclidean_distance_smallest_min]++;
   }

   state->distance_smallest_spread =
      state->euclidean_distance_smallest_max-state->euclidean_distance_smallest_min;

   state->distance_largest_spread =
      state->euclidean_distance_largest_max-state->euclidean_distance_smallest_min;

   state->distance_concentration =
      abs(state->distance_smallest_spread-state->distance_largest_spread);

   state->distance_concentration_factor =
      ((double)state->distance_smallest_spread)/state->distance_largest_spread;
   
   state->distance_weight =
      calloc(state->euclidean_distance_smallest_max-state->euclidean_distance_smallest_min+1, sizeof(double));

   for(i=0; i < state->euclidean_distance_smallest_max-state->euclidean_distance_smallest_min+1; i++)
   {
      state->distance_weight[i] = ((double)state->euclidean_distances_smallest[i])/state->distance_concentration;
      printf("(%2d, %2d) = %2.2lf\n",
             state->euclidean_distances_smallest[i],
             state->distance_concentration,
             ((double)state->euclidean_distances_smallest[i])/state->distance_concentration);
   }
}

void
distance_analyze(distance_t* state)
{

}

void
graph_init(FILE* fp)
{
   fprintf(fp, "%s\n", "start of new graph");
}

void
graph_fini(FILE* fp)
{
   fprintf(fp, "%s\n", "end of new graph");
}

void
graph_plot(FILE* fp, int* array, int dimensions)
{
   int i;

   //preamble to new data-point
   for(i=0; i < dimensions; i++)
   {
      printf("%2d ",array[i]);
      fprintf(fp,"%2d ",array[i]);
   }
   //postamble to new data-point
   fprintf(fp, "\n");
   printf(" - ");
}

void
distance_prnt(distance_t* state)
{
   FILE* fp;
   int i;
   
   printf("Smallest spread:       %2d\n",state->distance_smallest_spread);
   printf("Largest spread:        %2d\n",state->distance_largest_spread);
   printf("Concentration:         %2d\n",state->distance_concentration);
   printf("Concentration-factor:  %2.2lf\n",state->distance_concentration_factor);
   for(i=0; i < state->euclidean_distance_smallest_max-state->euclidean_distance_smallest_min+1; i++)
   {
      printf("Distance-weight %2d: %2.2lf\n",state->euclidean_distances_smallest[i], state->distance_weight[i]);
   }
   
   fp = fopen(state->filename_out, "wt");
   graph_init(fp);
   for(i=0; i < state->data_size; i++)
   {
      graph_plot(fp,state->data[i].points, state->dimensions);
   }
   printf("\n");
   graph_fini(fp);
   graph_init(fp);
   graph_plot(fp,state->euclidean_distances_smallest,
              state->euclidean_distance_smallest_max-state->euclidean_distance_smallest_min + 1);
   printf("\n");
   graph_fini(fp);
   fclose(fp);
}

void
distance_exit(distance_t* state)
{
   int i;

   if(state->euclidean_distances_smallest != NULL)
      free(state->euclidean_distances_smallest);
   if(state->distance_weight != NULL)
      free(state->distance_weight);
   for(i=0; i < state->data_size; i++)
   {
      free(state->data[i].points);
   }
   free(state->data);
}

int main (void){
   distance_t state;
   int i;

   for(i=0; i < TEST_CASES; i++)
   {
      distance_init(&state, i);
      distance_calculate(&state);
      distance_analyze(&state);
      distance_prnt(&state);
      distance_exit(&state);
   }
   printf("Exiting...\n");
   return 0;
}
