#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define NUMBER_OF_CORES 4
#define C0 0x1
#define C1 0x2
#define C2 0x4
#define C3 0x8
#define COMMON C0|C1|C2|C3

int
core_number(int id)
{
   int ret;
   switch(id)
   {
      case C0:
         ret = 0;
         break;
      case C1:
         ret = 1;
         break;
      case C2:
         ret = 2;
         break;
      case C3:
         ret = 3;
         break;
      default:
         ret = -1;
         break;
   }
   return ret;
}

/* Memory-types: */
#define INST 0x1
#define DATA 0x2

typedef enum {PHYSICAL, VIRTUAL} addressing_t;

typedef unsigned int address_t;

typedef struct{
   int       core;
   char*     name;
   int       memory_type;
   address_t strt_physical;
   address_t strt_virtual[NUMBER_OF_CORES];
   int       size;
}memory_region_t;

typedef struct
{
   int size;
   memory_region_t* region;
}memory_regions_set_t;

typedef struct allocation_s{
   address_t start;
   unsigned int count;/*Maximum depends on associativity of the cache.*/
   memory_region_t** allocators;
   struct allocation_s *next;
}allocation_t;

typedef struct{
   int           core;
   char *        name;
   int           memory_type;
   int           block_size;
   int           block_count;
   unsigned int  associativity;
   addressing_t  address_type;
   allocation_t* blocks; /*Dynamic*/
}cache_level_t;

cache_level_t cache_level[] =
{{C0,     "C0 L1I", INST     ,  32,   256, 4, VIRTUAL,  NULL},
 {C0,     "C0 L1D",      DATA,  32,   256, 4, VIRTUAL,  NULL},
 {C0,     "C0 L2 ", INST|DATA, 128,  1024, 4, PHYSICAL, NULL},
 {C1,     "C1 L1I", INST     ,  32,   256, 4, VIRTUAL,  NULL},
 {C1,     "C1 L1D",      DATA,  32,   256, 4, VIRTUAL,  NULL},
 {C1,     "C1 L2 ", INST|DATA, 128,  1024, 4, PHYSICAL, NULL},
 {C2,     "C2 L1I", INST     ,  32,   256, 4, VIRTUAL,  NULL},
 {C2,     "C2 L1D",      DATA,  32,   256, 4, VIRTUAL,  NULL},
 {C2,     "C2 L2 ", INST|DATA, 128,  1024, 4, PHYSICAL, NULL},
 {C3,     "C3 L1I", INST     ,  32,   256, 4, VIRTUAL,  NULL},
 {C3,     "C3 L1D",      DATA,  32,   256, 4, VIRTUAL,  NULL},
 {C3,     "C3 L2 ", INST|DATA, 128,  1024, 4, PHYSICAL, NULL},
 {COMMON, "   L3 ", INST|DATA, 256, 4*256, 8, PHYSICAL, NULL}};

void
cache_level_initialize(cache_level_t* cl, int memory_region_size)
{
   int i;

   cl->blocks = calloc(cl->block_count,sizeof(allocation_t));
   for(i = 0; i < cl->block_count; i++)
   {
      cl->blocks[i].allocators = calloc(memory_region_size,sizeof(memory_region_t*));
   }
}
void
cache_level_finalize(cache_level_t* cl)
{
   int i;
   for(i = 0; i < cl->block_count; i++)
   {
      free(cl->blocks[i].allocators);
   }
   free(cl->blocks);
}

address_t
cache_address_MSB_mask_calculate(cache_level_t* cl)
{
   unsigned int v = cl->block_count;
   unsigned int c;
   for (c = 0; v; c++)
      v = v << 1;
   return (cl->block_count-1) << c;
}

address_t
cache_address_LSB_mask_calculate(cache_level_t* cl)
{
   return (cl->block_count-1);
}

address_t
cache_index_calculate(address_t ca, cache_level_t* cl)
{
   address_t ret;
   address_t c,v;

   v = cl->block_size - 1;
   for (c = 0; v; c++)
   {//Count the number of significant bits in v
      v = v >> 1;
   }
   ret = (ca >> c) & ((cl->block_count-1));
   return ret;
}

int
cache_calculate_blocks(int memory_size, int block_size)
{/* Should be done with ceil() - will not compile */
   int i;
   for(i = 0; memory_size > i * block_size ; i++);
   return i;
}

memory_region_t memory_regionA[] =
{{C0       ,"A1",      DATA,0xFFFF0000,{0xFFFF0000,
                                        0x00000000,
                                        0x00000000,
                                        0x00000000},128},
 {C0       ,"A2",      INST,0xEEEE0000,{0x11110000,
                                        0x0,
                                        0x0,
                                        0x0},128},
 {C1|C2    ,"A3",      DATA,0xDDDD00FF,{0x00000000,
                                        0xFFFF0000,
                                        0xFFFF0000,
                                        0x00000000},128},
 {C0       ,"A4",      INST,0xCCCC0000,{0xAAAA0000,
                                        0x00000000,
                                        0x00000000,
                                        0x00000000},128},
 {C0       ,"A5",      INST,0xBBBB0000,{0xBBBB0000,
                                        0x00000000,
                                        0x00000000,
                                        0x00000000},128},
 {C0       ,"A6",      INST,0xAAAA0000,{0xBBBB0000,
                                        0x00000000,
                                        0x00000000,
                                        0x00000000},128}};
memory_region_t memory_regionB[] =
{{C1,      "HV_SHARED",DATA,0x10000000,{0xD0000000,
                                        0xD0000000,
                                        0xD0000000,
                                        0xD0000000},0x00004000},
 {C1,        "VM_SASE",DATA,0x10004000,{0x80000000,
                                        0x80000000,
                                        0x80000000,
                                        0x80000000},0x04000000},
 {C1,         "VM_SAS",DATA,0x14004000,{0x84000000,
                                        0x84000000,
                                        0x84000000,
                                        0x84000000},0x04000000},
 {C1,         "VM_MAS",DATA,0x18004000,{0x88000000,
                                        0x88000000,
                                        0x88000000,
                                        0x88000000},0x04000000},
 {C2,      "HV_SHARED",DATA,0x20000000,{0xD0000000,
                                        0xD0000000,
                                        0xD0000000,
                                        0xD0000000},0x00004000},
 {C2,        "VM_SASE",DATA,0x20004000,{0x80000000,
                                        0x80000000,
                                        0x80000000,
                                        0x80000000},0x04000000},
 {C2,         "VM_SAS",DATA,0x24004000,{0x84000000,
                                        0x84000000,
                                        0x84000000,
                                        0x84000000},0x04000000},
 {C2,         "VM_MAS",DATA,0x28004000,{0x88000000,
                                        0x88000000,
                                        0x88000000,
                                        0x88000000},0x04000000},
 {C3,      "HV_SHARED",DATA,0x30000000,{0xD0000000,
                                        0xD0000000,
                                        0xD0000000,
                                        0xD0000000},0x00004000},
 {C3,        "VM_SASE",DATA,0x30004000,{0x80000000,
                                        0x80000000,
                                        0x80000000,
                                        0x80000000},0x04000000},
 {C3,         "VM_SAS",DATA,0x34004000,{0x84000000,
                                        0x84000000,
                                        0x84000000,
                                        0x84000000},0x04000000},
 {C3,         "VM_MAS",DATA,0x38004000,{0x88000000,
                                        0x88000000,
                                        0x88000000,
                                        0x88000000},0x04000000}};
memory_region_t memory_regionC[] =
{{COMMON,    "VECTORS",INST,0x00000000,{0x00000000,
                                        0x00000000,
                                        0x00000000,
                                        0x00000000},0x00003000},
 {COMMON,  "razordata",DATA,0x00003000,{0x00003000,
                                        0x00003000,
                                        0x00003000,
                                        0x00003000},0x00004000},
 {COMMON,     "ramlog",DATA,0x00007000,{0x00007000,
                                        0x00007000,
                                        0x00007000,
                                        0x00007000},0x00008000},
 {COMMON,  "rpmm_root",INST,0x0000f000,{0x0000f000,
                                        0x0000f000,
                                        0x0000f000,
                                        0x0000f000},0x00001000},
 {COMMON,       "pbuf",DATA,0x00026000,{0x00026000,
                                        0x00026000,
                                        0x00026000,
                                        0x00026000},0x00100000}};
 
memory_regions_set_t memory_regions_set[] = {{sizeof(memory_regionA)/sizeof(memory_region_t), memory_regionA},
                                             {sizeof(memory_regionB)/sizeof(memory_region_t), memory_regionB},
                                             {sizeof(memory_regionC)/sizeof(memory_region_t), memory_regionC}};

void
memory_region_print(memory_region_t mr)
{
   int i, core;
   for(i=0; i<NUMBER_OF_CORES; i++)
   {
      core = core_number((mr.core)&(1<<i));
      if(core != -1)
         printf("%d",core_number((mr.core)&(1<<i)));
      else
         printf("_");

   }
   printf(" ");
   printf("%20s ",mr.name);
   printf("%d ",mr.memory_type);
   printf("%8x (",mr.strt_physical);
   for(i=0; i<NUMBER_OF_CORES; i++)
   {
      if(i != 0)
      {
         printf(" ");
      }
      printf("%8x",mr.strt_virtual[i]);
   }
   printf(") %8x",mr.size);
   printf("\n");
   return;
}

void
allocate_region(memory_region_t* mr, int mr_number, cache_level_t* cl)
{
   int i;
   address_t cache_address;
   int cache_index;
   int blocks_needed;

   if(((mr->core & cl->core) != 0) &&
      ((mr->memory_type & cl->memory_type) != 0))
   {
      blocks_needed = cache_calculate_blocks(mr->size, cl->block_size);
      printf("Region %10s needs %8d blocks in %10s\n", mr->name, blocks_needed, cl->name);
      for(i = 0; i < blocks_needed ; i++)
      {
         int block_found = 0;
         if(cl->address_type == PHYSICAL)
         {
            cache_address = (mr->strt_physical +
                             i * cl->block_size);
         }
         else
         {/* Since L3 is PHYSICAL; cl->core cannot be COMMON */
            cache_address = (mr->strt_virtual[core_number(cl->core)] +
                             i * cl->block_size);
         }
         cache_index = cache_index_calculate(cache_address, cl);
         if(cache_index >= cl->block_count)
         {
            printf("Invalid block-address!\n");
            exit(1);
         }
         cl->blocks[cache_index].allocators[mr_number] = mr;
         cl->blocks[cache_index].start = cache_address;
         cl->blocks[cache_index].count++;
         /*{//DEBUGPRINT
            char* s;
            s = (mr->memory_type == DATA)? "D" : "I";
            printf("Made allocation %s %s: %s - %8x, %x\n",
                   mr->name,
                   s,
                   cl->name,
                   cl->blocks[cache_index].start,
                   cl->blocks[cache_index].count);
           }*/
      }
   }
}

void
print_cache_level(cache_level_t* cl)
{
   int i,j;
   for(i = 0; i < cl->block_count; i++)
   {
      printf("Allocation of %s - cache-line %4d:", cl->name, i);
      for(j = 0; j < cl->associativity; j++)
      {/* Print the number of allocated ways as 'X' */
         if(j < cl->blocks[i].count)
         {
            printf("X");
         }
         else
         {
            printf("_");
         }
      }
      if(cl->blocks[i].count != 0)
      {
         printf(" by: ");
         for(j = 0; j < sizeof(memory_regions_set)/sizeof(memory_regions_set_t); j++)
         {
            if(cl->blocks[i].allocators[j] != NULL)
            {
               printf("%s, ", cl->blocks[i].allocators[j]->name);
            }
         }
      }
      printf("\n");
   }
}

void
analyze_cache_level(cache_level_t* cl)
{
   int i;
   for(i = 0; i < cl->block_count; i++)
   {
      if(cl->blocks[i].count > cl->associativity)
      {
         printf("Insufficient cache-associativity in %s (adr:%8x, count %2u)\n",
                cl->name, cl->blocks[i].start, cl->blocks[i].count);
      }
   }
}

int
main(int argc, char* argv[])
{
   int i, j, k;
   int memory_region_number;
   
   if(argc == 1)
   {
      printf("\n");
      for(i = 0; i < sizeof(memory_regions_set)/sizeof(memory_regions_set_t); i++)
      {
         for(j = 0; j < memory_regions_set[i].size; j++)
         {
            printf("ID %2d: ", i);
            memory_region_print(memory_regions_set[i].region[j]);
         }
         printf("\n");
      }
      printf("Please choose an id from the above listing and supply\n"
             "its ID as a parameter to the program on the command-line,\n"
             "e.g.: \"%s 1\".\n\n",argv[0]); 
   }
   else
   {
      memory_region_number = atoi(argv[1]);
      if(memory_region_number >= 0 && memory_region_number < sizeof(memory_regions_set)/sizeof(memory_regions_set_t)){
         for(i = 0; i < sizeof(cache_level)/sizeof(cache_level_t); i++)
         {
            cache_level_initialize(&(cache_level[i]), memory_regions_set[memory_region_number].size);
         }
         for(i = 0; i < memory_regions_set[memory_region_number].size; i++)
         {
            for(j = 0; j < sizeof(cache_level)/sizeof(cache_level_t); j++)
            {
               allocate_region(&(memory_regions_set[memory_region_number].region[i]),i, &(cache_level[j]));
            }
            printf("\n");
         }
         for(i = 0; i < sizeof(cache_level)/sizeof(cache_level_t); i++)
         {
            print_cache_level(&(cache_level[i]));
            analyze_cache_level(&(cache_level[i]));
         }   
         for(i = 0; i < sizeof(cache_level)/sizeof(cache_level_t); i++)
         {
            cache_level_finalize(&(cache_level[i]));
         }
      }
      else
      {
         printf("Invalid memory region; must be in the domain (0,%d).\n",
                sizeof(memory_regions_set)/sizeof(memory_regions_set_t)-1);
      }
   }
   return 0;
}
