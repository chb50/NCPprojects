#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//may need stdlib.h

//sizeof gives the number of bytes of a type. strlen gives the length of the string

int findString(FILE* fp, char* iString) {

	int copy = 1; //flag that states if we found a copy
	int copyCount = 0;

	char fBuff[1000];

	fBuff[sizeof(fBuff) - 1] = 0; //the terminating char
	int elemRead = fread(fBuff, 1, sizeof(fBuff), fp);
	fBuff[elemRead] = 0; //assign terminating char where last char has been read

	printf("File Contents:\n");
	printf("%s\n", fBuff);

	printf("%i\n", (int)strlen(iString));

	int i = 0;
	while(fBuff[i] != '\0') {
		if(fBuff[i] == iString[0]) {
			int j = 1;
			for(; j < strlen(iString); ++j) {
				if(fBuff[i + j] != iString[j]) {
					copy = 0;
					break;
				}
			}
			if(copy == 1) {
				copyCount++;
			}
		}
		i++;
	}

	return copyCount;
	//if elemRead is less than the acutal number of elements in the file, then we have a problem
}

int main(int argc, char* argv[]) {
	char* n = argv[1];
	char fn[strlen(n) + 3]; //expand for ".c" extention
	int i = 0;
	for(; i < strlen(n); ++i) { // the -1 accounts for terminating char, which i want to move
		fn[i] = argv[1][i];
		printf("%c\n", fn[i]);
	}

	fn[strlen(n)] = '.', fn[strlen(n) + 1] = 'c', fn[strlen(n) + 2] = '\0'; //add extention

	printf("%s\n", fn);

	FILE* fp = fopen(fn, "r");

	if(fp == NULL) {
		perror("ERROR WHEN OPENING FILE\n");
		exit(1);
	}

	int countArray[argc - 2]; //each string must have an associated cout to it
	i = 0;
	for(;i < argc-2; ++i) {
		countArray[i] = findString(fp, argv[i+2]); //remember, first arguement is executable file name, the second is the file we are reading
		printf("The count for string %s is %i.\n", argv[i+2], countArray[i]);
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