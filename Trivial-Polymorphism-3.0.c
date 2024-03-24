#include <stdio.h>
#include <stdlib.h> // system & srand
#include <string.h> // strlen
#include <unistd.h> // unlink
#include <time.h> // time
#include <sys/stat.h> // stat & chmod
#include <stdbool.h>

#define PAGE 1024 // Must always be even (preferable base 2)
#define MAX_FUNCTION_NAME 20
#define MIN_FUNCTION_NAME 8

static char functionList[][MAX_FUNCTION_NAME+1] = {"function1", "function2", "newFunctionNames"};

static int functionCount = (sizeof(functionList) / sizeof((*functionList)));

void function1(){
	printf("A beautiful function!\n");
	return;
}

void function2(){
	printf("An ugly function!\n");
	return;
}

char* newFunctionNames(){
	char* mutexFunctionName = malloc(functionCount*(MAX_FUNCTION_NAME+1));
	memset(mutexFunctionName, 0, functionCount*(MAX_FUNCTION_NAME+1));
	int i;
	for (i = 0; i < functionCount; i++){
		// Generate new name.
		int k;
		int tmpLength = MIN_FUNCTION_NAME+(rand() % (MAX_FUNCTION_NAME-MIN_FUNCTION_NAME));
		printf("New function length = %d\n", tmpLength); // Was: strlen(functionList[i])
		for (k = 0; k < tmpLength; k++){
			int tmp = rand() % (26+26+10);
			if (k == 0) // Ensures no numbers at first.
				tmp = rand() % (26+26);
			
			char c=0;
			if (tmp <= 25) { // Uppercase
				c = 65 + tmp;
			} else if (tmp > 25 && tmp <= 51) { // Lowercase
				tmp = tmp - 26;
				c = 97 + tmp;
			} else { // Number
				tmp = tmp - 52;
				c = 48 + tmp;
			}
			mutexFunctionName[(i*(MAX_FUNCTION_NAME+1))+k] = c;
		}
	}
	return mutexFunctionName;
}

int main(int argc, char **argv)
{
	srand(time(NULL)); // Set random seed.
	
	printf("Program location = %s\n", argv[0]);
	
	// Mutex name
	int mutexLength = strlen(argv[0])+6+1;
	char* mutex = malloc(mutexLength);
	snprintf(mutex, mutexLength, "%s.mutex", argv[0]);
	
	// Function mutex
	char* mutexFunctionName = newFunctionNames();
	
	printf("mutex = %s\n", mutex);
	printf("function count = %d\n", functionCount);
	
	int o;
	for (o = 0; o < functionCount; o++)
	{
		printf("function = %s\n", mutexFunctionName+(o*(MAX_FUNCTION_NAME+1)));
	}
	
	// Copy program
	FILE *originFile, *mutexFile;
	originFile = fopen(argv[0], "rb"); // Original file
	if (!originFile)
		return 1;
	
	mutexFile = fopen(mutex, "wb"); // Mutex
	if (!mutexFile)
		return 2;
	
	size_t n = 0;
	
	char memoryPages[PAGE*3];
	int totalDataInPages = 0;
	
	memset(memoryPages, 0, sizeof(memoryPages));
	
	// Logic of Polymorphic
	do {
		// Switch pages around.
		if (totalDataInPages >= PAGE){
			// Write old page into file.
			if (totalDataInPages > PAGE){
				if (fwrite(memoryPages, 1, PAGE, mutexFile) != PAGE){
					if (fclose(originFile))
						return 4;
					
					if (fclose(mutexFile))
						return 5;
					// TODO: Clean me.
					return 7;
				}
				
				// Move old page into start of buffer.
				memcpy(memoryPages, memoryPages+PAGE, totalDataInPages-PAGE); // edge case if totalDataInPages is smaller than a page, not detected as last page and fails here...
				// or maybe you did a good job and it just needs to handle not needing to switch page...
				totalDataInPages -= PAGE;
				memset(memoryPages+totalDataInPages, 0, PAGE+(PAGE-totalDataInPages));
			} else { // Move old page into start of buffer for first page.
				memcpy(memoryPages, memoryPages+PAGE, totalDataInPages);
				memset(memoryPages+totalDataInPages, 0, totalDataInPages);
			}
		}
		
		// Reading a new page.
		if (totalDataInPages == 0){
			n = fread(memoryPages+PAGE, 1, PAGE, originFile);
		} else {
			n = fread(memoryPages+totalDataInPages, 1, PAGE+(PAGE-totalDataInPages), originFile);
		}
		totalDataInPages += n;
		printf("New data count: %d\nIncreased by: %lu\n", totalDataInPages, n);
		
		// TODO: Patch ELF metadata for sizes to ensure no crashing of mutex.
		
		if (n) { // if N not 0
			// Iterate over function names
			int i;
			for (i = 0; i < functionCount; i++){
				printf("Entry search = %s\n", functionList[i]);
				int j;
				int functionLength = strlen(functionList[i]);
				int totalReadData = totalDataInPages-PAGE+functionLength;
				
				for (j = 0; j < totalReadData; j++){
					if (memcmp(memoryPages+PAGE-functionLength+j, functionList[i], functionLength) == 0){
						printf("Entry found! %s\n", memoryPages+PAGE-functionLength+j);
						
						printf("Replaced by: %s\n", mutexFunctionName+(i*(MAX_FUNCTION_NAME+1)));
						
						int newFunctionLength = strlen(mutexFunctionName+(i*(MAX_FUNCTION_NAME+1)));
						int differenceOfLength = newFunctionLength-functionLength;
						int movedDataCount = totalDataInPages-PAGE-differenceOfLength-j;
						
						int *tmpBuffer;
						
						// Move surrounding data.
						tmpBuffer = malloc(movedDataCount);
						memcpy(tmpBuffer, memoryPages+PAGE+j, movedDataCount);
						memcpy(memoryPages+PAGE+j+differenceOfLength, tmpBuffer, movedDataCount);
						free(tmpBuffer);
						
						// Adjust data count.
						totalDataInPages += differenceOfLength;
						
						// Insert new data.
						memcpy(memoryPages+PAGE-functionLength+j, mutexFunctionName+(i*(MAX_FUNCTION_NAME+1)), newFunctionLength);
					}
				}
			}
			
			// Save last pages.
			if (feof(originFile) != 0){
				printf("Last page was: %d\n", totalDataInPages);
				if (fwrite(memoryPages, totalDataInPages, 1, mutexFile) != 1){ // Copy all data left.
					if (fclose(originFile))
						return 4;
					
					if (fclose(mutexFile))
						return 5;
					// TODO: Handle error properly and make a nice function for it.
					return 6;
				}
				totalDataInPages = 0;
				break;
			}
		}
	} while (n > 0);
	
	if (fclose(originFile))
		return 4;
	
	if (fclose(mutexFile))
		return 5;
	
	// Copy permissions from current executable
	struct stat mutexPermissions;
	stat(argv[0], &mutexPermissions);
	
	// Set new permissions for mutex
	if (chmod(mutex, mutexPermissions.st_mode))
		return 6;
	
	// Remove original file.
	//unlink(argv[0]);
	
	// Launch mutex using exec(What do I do about the free?)
	
	free(mutex);
	free(mutexFunctionName);
	
	// Garbadge functions
	function1();
	function1();
	function1();
	function1();
	function2();
	
	printf("Mutex created!\n");
	return 0;
}
