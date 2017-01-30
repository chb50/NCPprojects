#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
//may need stdlib.h

#define FILE_SIZE 10485764 //10MB + 4 Bytes for padding

//sizeof gives the number of bytes of a type. strlen gives the length of the string

//can try reading the file into a buff of size 1, char by char
int findString(FILE* fp, char* iString) {

	int copy = 1; //flag that states if we found a copy
	int copyCount = 0;

	//finds size of file
	fseek(fp, 0, SEEK_END); //set pointer to end of file
	int size = ftell(fp); //get file size
	assert(size < FILE_SIZE);
	fseek(fp, 0, SEEK_SET); //set pointer back to begining of file
	char* fBuff = malloc(size);
	int elemRead = fread(fBuff, 1, size, fp);
	fBuff[elemRead] = 0; //assign terminating char where last char has been read

	// printf("File Contents:\n");
	//printf("%s\n", fBuff);

	//printf("%i\n", (int)strlen(iString));

	int i = 0;
	while(fBuff[i] != '\0') {
		if(tolower(fBuff[i]) == tolower(iString[0])) {
			int j = 1;
			for(; j < strlen(iString); ++j) {
				if(tolower(fBuff[i + j]) != tolower(iString[j])) {
					copy = 0;
					break;
				}
			}
			if(copy == 1) {
				copyCount++;
			}
		}
		i++;
		copy = 1;
	}

	free(fBuff);

	return copyCount;
	//if elemRead is less than the acutal number of elements in the file, then we have a problem
}

int main(int argc, char* argv[]) {

	FILE* fp = fopen(argv[1], "r");

	if(fp == NULL) {
		perror("./p05");
		exit(1);
	}

	int i = 0;
	int countArray[argc - 2]; //each string must have an associated cout to it
	i = 0;
	for(;i < argc-2; ++i) {
		countArray[i] = findString(fp, argv[i+2]); //remember, first arguement is executable file name, the second is the file we are reading
		//printf("The count for string %s is %i.\n", argv[i+2], countArray[i]);
		printf("%i\n", countArray[i]);
		rewind(fp); //set pointer back to beginning of file
	}

	fclose(fp);
	//use an array of the size of each arguement which will save the matched chars
	//if entire array matches the arguement, then we have a match within the file


	// int i = 1; //first arguement is file name
	// for(; i < argc; ++i) { //find for every arguement

	// }

	return 0;
}
