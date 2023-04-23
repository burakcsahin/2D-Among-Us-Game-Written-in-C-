// Write your full name: Burak Can Sahin, write your KU ID: 76824
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define USE_VISUALIZATION true  
#define VIS_EMPTY_CELL_TEXT " "
//if you set USE_VISUALIZATION to true, visual mode for bonus part works and also generates filename_vis.txt kind of named output additionaly
//to the normal output txts. If you set it to false, it disables bonus part. Use cat filename_vis.txt command to test it.


//hope everything works well
int N, iteration;
int dead_astro_count, dead_imp_count;
int astro_count, imp_count;
enum game_status {Defeat, Victory, Continue};
enum game_status g_stat;

typedef struct Person {
  bool is_alive;
  int x, y;
}Astronaut, Impostor;


struct People {
  struct Astronaut ** astronauts;
  struct Impostor ** impostors;
};

void initialize();
void print_error(const char* error);
void free_people(struct People* pPeople);
struct Person** parse_from_coordinates(const char* coordinates, int max_count);

void print_state(struct People* people);

void move_astronauts(Astronaut** asronauts);
Impostor* find_impostor(Impostor** impostors, int x, int y);
bool kill_target(Impostor* imp, Astronaut* with_out_ast, Astronaut** astronauts);
bool witness(Impostor* imp, Astronaut* with_out_ast, Astronaut** astronauts);

void next_state(struct People* people);
void update_game_status(struct People* people);
#if USE_VISUALIZATION
char* create_cell_text(struct People* people, int x, int y);
#endif

void write_a_file(char* in_fname, struct People* state, int cur_iteration);

void initialize(){
//when initializing for the first time, it will reset all variables.
N = 0;
iteration = 0;
astro_count = 0;
imp_count = 0;
dead_astro_count = 0;
dead_imp_count = 0;
g_stat = Continue;

}

void print_error(const char* message)
{
//just in case if sth strange happens
  if(!message){
  return;
  }

  printf("ERROR: %s\n", message);

}

void free_people(struct People* pPeople)
{
   if(!pPeople /*is null*/){
     return;
   } 

   int i;
   if(pPeople->astronauts /* if not null */){
     for(i = 0; i<astro_count; i++){
        Astronaut* item = pPeople->astronauts[i];
	if(item /* is not null */){
	  free((void*)item); //free it
	}
     
     }
     
     //free the whole array
     free((void*)pPeople->astronauts);
   
   }

   if(pPeople-> impostors /* is not null */){
     for(i = 0; i < imp_count; i++){
        Impostor* item = pPeople->impostors[i];
	if(item /* is not null */){
	   free((void*)item); //free it too
	}
     }
     
     //free the whole array 
     free((void*)pPeople->impostors);
   
   }

   //now, free the entire Person array here
   free((void*)pPeople);


}

struct Person** parse_from_coordinates(const char* coordinates, int max_count)
{
     if(!coordinates || (strlen(coordinates) == 0)){
       print_error("'coordinates' must not be null or empty.");
       return 0;
     }
     
     if(max_count <= 0){
       print_error("Invalid count.");
       return 0;
     
     }
     //again checked for unpleasent cases/data

     int resultSize = max_count * sizeof(struct Person*);
     struct Person** result = (struct Person**)malloc(resultSize);
     if(!result){
       print_error("Out of memory.");
       return 0;
       //check out of memo condition
     }
     //resseting mem
    memset((void*)result, 0, resultSize);

    for (int i = 0; i < max_count; i++){
        struct Person* item = (struct Person*)malloc(sizeof(struct Person));
	if(item){
	  *(result + i) = item; //syntactic sugar for pointers
	}
    
    }

    int nMaxX = 2*N;
    int nMaxY = 2*N;
    int nPos = 0, x = 0, y = 0, ndx = 0, nCount = 0;
    bool isReady = false;
    int nStrLen = (int)strlen(coordinates);
    char szBuffer[128];

    //the below part is for how I tried and hopefully managed to do the part of coordinate reading stuff (e.g reading a,b & c,d & with respect to commas spaces and &s)

    for(int i =0; i<= nStrLen; i++){
       char ch = coordinates[i];
       if(ch == ','){
         //obtain x
	 szBuffer[nPos++] = '\0'; /* end of str */
	 nPos = 0;  /* reset pos */
	 x = atoi(szBuffer);
       }

       else if (ch == '&' || ch =='\0' /* eof str */){
         //obtain y
	 szBuffer[nPos++] = '\0'; //eof str
         nPos =0; //res pos
	 y = atoi(szBuffer);
	 isReady = true;
       }
       else{
         szBuffer[nPos++] = ch;
       }

       if(isReady)
       {
         isReady = false; //enabling us to wait for the next ready
         ndx = nCount++;
         if(ndx >= max_count){
	   break; //a mechanism to check if it is out of bounds
	 }	 
         
	 //parameter setting
	 struct Person* person = *(result + ndx); //again syntacic sugar, same ase result[ndx]
	 if(person && (x <= nMaxX) && (y <= nMaxY)){
	   person->is_alive = true;
	   person->x = x;
	   person->y = y;
	   
	 }
       }
    
    }

    return result;

}

struct People* read_from_file(const char* filename)
{
    //in this function I wrote every comment like it is an educational material. I did not know the file io in C, and I hope it works.
    
    if (!filename || (strlen(filename) == 0)) {
        print_error("'filename' must not be null or empty.");
        return 0; /* NULL */
	//again checking just in case
    }

    int ndxLine = 0;
    struct People *result = 0; /* NULL */
    // It should be of sufficient size.
    char line[1024], astro_coords[1024], imp_coords[1024];

    // reset the array content.
    memset((void*)astro_coords, 0, sizeof(astro_coords));
    memset((void*)imp_coords, 0, sizeof(imp_coords));

    // open the file as read-only. use the 'r' flag.
    FILE* pf = fopen(filename, "r");
    if (!pf) {
        print_error("The file could not be opened for reading.");
        return 0; /* NULL */
	//just in case
    }

    // Read the file line by line. Note: fgets don't strip the terminating \n.
    while (fgets(line, sizeof(line), pf)) {
        // 'line' contains the information of a single line read.
        switch (ndxLine++)
        {
        case 0: /* Astronaut count */
            astro_count = atoi(line); /* Converting from char* to integer. (Like string to numeric) */
            break;
        case 1: /* Impostor count */
            imp_count = atoi(line);
            break;
        case 2: /* Dimension parameter */
            N = atoi(line);
            break;
        case 3: /* Step count */
            iteration = atoi(line);
            break;
        case 4: /* Astronaut coordinates */
            // Copy the memory contents.
            memcpy((void*)astro_coords, (const void*)line, strlen(line));
            break;
        case 5: /* Impostor coordinates */
            // Copy the memory contents.
            memcpy((void*)imp_coords, (const void*)line, strlen(line));
            break;
        }
    }
    // IMPORTANT: After the process is completed, close the file.
    fclose(pf);

    // Check if all parameters read. Just in case
    if (astro_count <= 0) {
        print_error("'astro_count' is out of bounds.");
        return 0; /* NULL */
    }
    if (imp_count <= 0) {
        print_error("'imp_count' is out of bounds.");
        return 0; /* NULL */
    }
    if (N < 5) {
        print_error("The 'N' value must be greater than '4'.");
        return 0; /* NULL */
    }
    if (iteration <= 0) {
        print_error("'iteration' is out of bounds.");
        return 0; /* NULL */
    }
    if (astro_coords[0] == 0 /* strlen(astro_coords) == 0 */) {
        print_error("'Astronaut coordinates' is not specified.");
        return 0; /* NULL */
    }
    if (imp_coords[0] == 0 /* strlen(imp_coords) == 0 */) {
        print_error("'Impostor coordinates' is not specified.");
        return 0; /* NULL */
    }

    // Allocate memory for the return parameter. (for 'Dynamic memory alocation')
    int result_size = sizeof(struct People);
    result = (struct People*)malloc(result_size);
    if (!result /* is null */) {
        print_error("Out of memory.");
        return 0; /* NULL */
    }
    
    result->astronauts = (Astronaut**)parse_from_coordinates(astro_coords, astro_count);
    result->impostors = (Impostor**)parse_from_coordinates(imp_coords, imp_count);

    // It should not be 'null'.
    if (!result->astronauts /* is null */ || !result->impostors /* is null */) {
        print_error("Out of memory.");
        free_people(result);
        return 0; /* NULL */
    }

    return result;
}
//
#if USE_VISUALIZATION

char* create_cell_text(struct People* people, int x, int y)
{
    // check parameters
    if (!people
        || (astro_count <= 0)
        || (imp_count <= 0)
        || !people->astronauts
        || !people->impostors
        || (x < 0)
        || (y < 0))
    {
        return 0; /* NULL */
    }

    int i;
    char szBuffer[128];
    // clear
    memset((void*)szBuffer, 0, sizeof(szBuffer));

    // astronauts
    if (people->astronauts)
    {
        for (i = 0; i < astro_count; i++) {
            Astronaut* item = people->astronauts[i];
            if (!!item /* is not null*/ && item->is_alive && (item->x == x) && (item->y == y) && !((item->x == N) && (item->y == N))) {
                if (szBuffer[0] == 0) {
                    strcpy(szBuffer, "A");
                }
                else {
                    strcat(szBuffer, ", A");
                }
            }
        }
    }

    if (people->impostors)
    {
        for (i = 0; i < imp_count; i++) {
            Impostor* item = people->impostors[i];
            if (!!item /* is not null*/ && item->is_alive && (item->x == x) && (item->y == y) && !((item->x == N) && (item->y == N))) {
                if (szBuffer[0] == 0) {
                    strcpy(szBuffer, "I");
                }
                else {
                    strcat(szBuffer, ", I");
                }
            }
        }
    }

    if (szBuffer[0] == 0) {
        if (x == N && y == N) /* Table center */ {
            strcpy(szBuffer, "ER");
        }
        else {
            strcpy(szBuffer, VIS_EMPTY_CELL_TEXT);
        }
    }

    size_t nSize = (strlen(szBuffer) + 1) * sizeof(char);
    char* pResult = (char*)malloc(nSize);
    if (pResult) {
        strcpy(pResult, szBuffer);
    }

    return pResult;
}

void write_a_file(char* in_fname, struct People* people, int cur_iteration)
{
    if (!in_fname || (strlen(in_fname) == 0)) {
        print_error("'in_fname' must not be null or empty.");
        return;
    }

    if (!people) {
        print_error("'people' must not be null.");
        return;
    }

    char szLeft[1024];
    char szRight[16];
    char szOutFileName[1024];
    const char* pLast = in_fname + strlen(in_fname);
    const char* pFind = strrchr(in_fname, '.');
    if (pFind /* is file extension exists */) {
        // Keep the location. (without extension)
        memcpy((void*)szLeft, (const void*)in_fname, (pFind - in_fname));
        szLeft[pFind - in_fname] = '\0'; /* eof string */
        // Get extension
        memcpy((void*)szRight, (const void*)pFind, (pLast - pFind));
        szRight[pLast - pFind] = '\0'; /* eof string */
    }
    else {
        // default
        strcpy(szLeft, in_fname);
        strcpy(szRight, ".txt");
    }
    // generate output file name
    sprintf(szOutFileName, "%s_out_%d_vis%s", szLeft, cur_iteration, szRight);

    int ndx = 0, nTmp;
    int nMaxTextLen = 0;
    int nColumns = (2 * N + 1);
    int nRows = (2 * N + 1);
    int nCells = nColumns * nRows;
    int nTableSize = nCells * sizeof(char*);
    char* pCellText = 0;
    char** ppTable = (char**)malloc(nTableSize);
    if (!ppTable) {
        print_error("Out of memory.");
        return;
    }
    // clear
    memset((void*)ppTable, 0, nTableSize);

    FILE* pf = fopen(szOutFileName, "w");
    if (!pf) {
        print_error("The file could not be opened for writing.");
        return;
    }

    // Set all cells to 'O'.
    for (int y = 0; y < nRows; y++) {
        for (int x = 0; x < nColumns; x++) {
            ndx = y * nRows + x;
            pCellText = create_cell_text(people, x, y);
            if (pCellText) {
                *(ppTable + ndx) = pCellText;
                nTmp = strlen(pCellText);
                if (nTmp > nMaxTextLen) {
                    nMaxTextLen = nTmp;
                }
            }
        }
    }

    int nSepH = (nColumns * (nMaxTextLen + 1)) + 1;
    int nRemain;
    char szSepH[1024];
    char szEmptyCell[32];
    memset((void*)szSepH, '-', nSepH);
    szSepH[nSepH] = '\0'; /* eof string */
    memset((void*)szEmptyCell, ' ', nMaxTextLen);
    szEmptyCell[nMaxTextLen] = '\0'; /* eof string */

    fprintf(pf, "%s\n", szSepH);
    for (int y = 0; y < nRows; y++) {
        for (int x = 0; x < nColumns; x++) {
            ndx = y * nRows + x;
            pCellText = *(ppTable + ndx);

            if (pCellText) {
                fprintf(pf, "|%s", pCellText);
                nRemain = nMaxTextLen - (int)strlen(pCellText);
            }
            else {
                fprintf(pf, "|%s", szEmptyCell);
                nRemain = nMaxTextLen - (int)strlen(szEmptyCell);
            }
            for (int i = 0; i < nRemain; i++) {
                fprintf(pf, " ");
            }
        }
        fprintf(pf, "|\n%s\n", szSepH);
    }

    // close file
    fclose(pf);

    // free
    for (int i = 0; i < nCells; i++) {
        char* pCell = *(ppTable + i);
        if (pCell) {
            free(pCell);
        }
    }
    free(ppTable);
}

#else // !USE_VISUALIZATION

void write_a_file(char* in_fname, struct People* people, int cur_iteration)
{
    if (!in_fname || (strlen(in_fname) == 0)) {
        print_error("'in_fname' must not be null or empty.");
        return;
    }

    if (!people) {
        print_error("'people' must not be null.");
        return;
    }

    char szLeft[1024];
    char szRight[16];
    char szOutFileName[1024];
    const char* pLast = in_fname + strlen(in_fname);
    const char* pFind = strrchr(in_fname, '.');
    if (pFind /* is file extension exists */) {
        // Keep the location. (without extension)
        memcpy((void*)szLeft, (const void*)in_fname, (pFind - in_fname));
        szLeft[pFind - in_fname] = '\0'; /* eof string */
        // Get extension
        memcpy((void*)szRight, (const void*)pFind, (pLast - pFind));
        szRight[pLast - pFind] = '\0'; /* eof string */
    }
    else {
        // default
        strcpy(szLeft, in_fname);
        strcpy(szRight, ".txt");
    }
    // generate output file name
    sprintf(szOutFileName, "%s_out_%d%s", szLeft, cur_iteration, szRight);

    int i;
    FILE* pf = fopen(szOutFileName, "w");
    if (!pf) {
        print_error("The file could not be opened for writing.");
        return;
    }

    fprintf(pf, "%d\n", dead_imp_count);
    fprintf(pf, "%d\n", imp_count - dead_imp_count);
    fprintf(pf, "%d\n", dead_astro_count);
    fprintf(pf, "%d\n", astro_count - dead_astro_count);
    switch (g_stat)
    {
    case Defeat:
        fprintf(pf, "Defeat\n");
        break;
    case Victory:
        fprintf(pf, "Victory\n");
        break;
    case Continue:
        fprintf(pf, "Continue\n");
        break;
    default:
        fprintf(pf, "Unknown\n");
        break;
    }

    if (people->astronauts) {
        for (i = 0; i < astro_count; i++) {
            Astronaut* item = people->astronauts[i];
            if (item->is_alive) {
                fprintf(pf, "%d, %d, Alive", item->x, item->y);
            }
            else {
                fprintf(pf, "%d, %d, Dead", item->x, item->y);
            }

            if (i < (astro_count - 1)) {
                fprintf(pf, " & ");
            }
        }
        fprintf(pf, "\n");
    }

    if (people->impostors) {
        for (i = 0; i < imp_count; i++) {
            Impostor* item = people->impostors[i];
            if (item->is_alive) {
                fprintf(pf, "%d, %d, Alive", item->x, item->y);
            }
            else {
                fprintf(pf, "%d, %d, Dead", item->x, item->y);
            }

            if (i < (imp_count - 1)) {
                fprintf(pf, " & ");
            }
        }
        fprintf(pf, "\n");
    }

    fclose(pf);
}

#endif // !USE_VISUALIZATION

void print_state(struct People* people)
{
	//just did the same thing with my write_to_file I changed fprintfs to printfs
	printf("%d\n", dead_imp_count);
	printf("%d\n", (imp_count - dead_imp_count));
	printf("%d\n", dead_astro_count);
	printf("%d\n", (astro_count - dead_astro_count));

	switch(g_stat)
	{
		case Defeat:
			printf("State: Defeat\n");
		        break;
		case Victory:
			printf("State: Victory\n");
			break;
		case Continue:
			printf("State: Continue\n");
			break;
		default:
			printf("State: Unknown\n");
			break;
	}
        int i;
	int j;
        if(people->astronauts){
	  for(i = 0; i < astro_count; i++){
	     Astronaut* item = people->astronauts[i];
	     if (item->is_alive){
	        printf("%d, %d, Alive", item->x, item->y);
	        }
	     else{
		printf("%d, %d, Dead", item->x, item->y);
	        }
             if(i < (astro_count - 1)){
	        printf(" & ");
	        }
	     }
	  printf("\n");
	
	}

	if(people->impostors){
	  for(j = 0; j < imp_count; j++){
	     Impostor* item = people->impostors[j];
	     if(item->is_alive){
	       printf("%d, %d, Alive", item->x, item->y);
	       }
	     else{
	       printf("%d, %d, Dead", item->x, item->y);
	     }
	     if(j < (imp_count -1)){
	       printf(" & ");
	     }
	  }
	  printf("\n");
	}
	
	printf("\n"); //added an extra blank line for readability
}

void move_astronauts(Astronaut** astronauts)
{
    for(int i = 0; i < astro_count; i++){
       Astronaut* item = astronauts[i];
       if(item->is_alive)
       {
         if(item->y < N){
           item->y++;
	 }
	 else if(item->y > N){
	   item->y--;
	 }
	 else if(item->x < N){
	   item->x++;
	 }
	 else if(item->x > N){
	   item->x--;
	 }
       
       
       
       }
    }
}

Impostor* find_impostor(Impostor** impostors, int x, int y)
{
   for (int i =0; i < imp_count; i++){
       Impostor* imp = impostors[i];
       if (imp->is_alive && imp->x == x && imp->y == y){
          return imp;
       }
   }

   return 0; //return null if you cant find an imp
}

bool kill_target(Impostor* imp, Astronaut* with_out_ast, Astronaut** astronauts)
{
     for(int i = 0; i < astro_count; i++){
        Astronaut* ast = astronauts[i];
	if (!ast->is_alive || (ast == with_out_ast)){
	   continue;
	}

	if ((imp->y == ast->y && abs(imp->x - ast->x) == 1) || (imp->x == ast->x && abs(imp->y - ast->y) == 1)){
	   
           return false; //do not kill if there are neighbors
	}
     
     }

     return true; //kil'em

}

bool witness(Impostor* imp, Astronaut* with_out_ast, Astronaut** astronauts)
{
  for (int i = 0; i < astro_count; i++){
      Astronaut* ast = astronauts[i];
      if (!ast->is_alive || (ast == with_out_ast)){
         continue;
      }

      int n1 = abs(imp->x - ast->x);
      int n2 = abs(imp->y - ast->y);
      if (n1 == 1 && n1 == n2){
         return true; //checking for diagonal witnesses, and if the rule is satisfied witness it
      }

  
  }

  return false;

}

void next_state(struct People* people)
{
     move_astronauts(people->astronauts);
}

void update_game_status(struct People* people)
{
    for (int i = 0; i < astro_count; i++) {
        Astronaut* astronaut = people->astronauts[i];
        if (astronaut->is_alive)
        {
            Impostor* impostor = find_impostor(people->impostors, astronaut->x, astronaut->y);
            if (impostor) {
                if (kill_target(impostor, astronaut, people->astronauts)) {
                    astronaut->is_alive = false;
                    dead_astro_count++;
                    if (witness(impostor, astronaut, people->astronauts)) {
                        impostor->is_alive = false;
                        dead_imp_count++;
                    }
                }
            }
        }
    }

    int ast_alive_count = astro_count - dead_astro_count;
    int imp_alive_count = imp_count - dead_imp_count;
    if (ast_alive_count <= 0) {
        g_stat = Defeat;
    }
    else if (imp_alive_count <= 0) {
        g_stat = Victory;
    }
    else {
        int nCenterCount = 0;
        for (int i = 0; i < astro_count; i++) {
            Astronaut* ast = people->astronauts[i];
            if (ast->is_alive && ast->x == N && ast->y == N) {
                nCenterCount++;
            }
        }

        if (nCenterCount == ast_alive_count) {
            g_stat = Victory;
        }
    }
}

int main(int argc, char **argv)
{
  int cur_iteration = 0;
  char * filename = argv[1];
  
  initialize();

  struct People * people = read_from_file(filename);
  
  print_state(people);
  write_a_file(filename, people, cur_iteration);

  for(cur_iteration = 1; cur_iteration < iteration; ++cur_iteration){
    next_state(people);
    update_game_status(people);
    print_state(people);
    write_a_file(filename, people, cur_iteration);

    if(g_stat == Victory || g_stat == Defeat){
      break;
    }
  }

  //free the memory
  free_people(people);
  
  return 0;
} 
