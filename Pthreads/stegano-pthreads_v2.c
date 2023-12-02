/* Copyright [2020] Gavan Adrian-George, 342C1 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

// Structure used for a pixel
typedef struct {
	// Red, Green, Blue
	unsigned char R,G,B;
}pixel;

// Structure used for an image
typedef struct {
	// P6
	char type[2];
	// Height and width of the image
	int height, width;
	// Maximum value of a cahannel
	int maxv;
	// The matrix of pixels
	pixel **colour;
}image;

void readInput(const char * fileName, image *img);
void writeData(const char * fileName, image *img);
int findSize(const char * fileName);
void readMes(const char * fileName, char *mes);
void convertCharToBinary(char c, char *outMes);
void convertBinaryToChar_decode(char tempMes[9], int pos);
char convertBinaryToChar_encode(char tempMes[9]);
unsigned char modifyPixelChannel(unsigned char p, int parity, int mesParity);
int calcNrOfPixels(image *img);
void writeMes(const char * fileName, char *mes);
void encodeMes(image *img, char *mes, int start, int end, int start_line);
void* threadRead(void *var);
void* threadWrite(void *var);
void* threadEncode(void *var);
void* threadDecode(void *var);


// Used to read the image to be decoded
image input;

// The input image will be read into 3
// smaller image structures. After we read a part of the initial
// matrix, this smaller image cand pe processed and after that written,
// while a thread is reading another smaller image in the same time
image input1;
image input2;
image input3;
// Height of each one of the three smaller images
int height1;
int height2;
int height3;

// The number of threads
int P;
// Variable used to store the size of the message to be encoded
int size;
// Used for decode to find the number of pixels that where modified
int nrPixelsModified;
// Used for decode to find the size of the message to be decoded
int sizeMes;
// Variable used to store the input text (encode)
char *mes;
// Variable used to store the output text (decode)
char *outMes;
// Variables used for the name of the files
const char *input_image_name;
const char *input_message_name;
const char *output_image_name;

// Semaphore used to synchronize the thred that is reading the image and the
// threads that are processing the image
sem_t mutex_read1;
sem_t mutex_read2;
sem_t mutex_read3;
// Semaphore used to synchronize the thred that is writting the image and the
// threads that are processing the image
sem_t mutex_write1;
sem_t mutex_write2;
sem_t mutex_write3;
// Barrier used to synchonize the thread that is reading the image and the
// threads processing the image
pthread_barrier_t barrier_wait_encode;


int main(int argc, char *argv[]) {
	int i;

	// Case when we want to encode a text into an image
	if (strcmp(argv[1], "encode") == 0) {

		// Take the names of the files from the input parameters
		input_image_name = argv[2];
		input_message_name = argv[3];
		output_image_name = argv[4];
		P = atoi(argv[5]);

		if (P < 5) {
			printf("To few threads. There must be at least 3 threads.\n");
		}

		// Declare thread vector and the thread id vector
		pthread_t tid[P];
		int thread_id[P];
		for(i = 0; i < P; i++) {
			thread_id[i] = i;
		}

		// Initialize the semaphores and the barrier
		sem_init(&mutex_read1, 0, 0);
		sem_init(&mutex_read2, 0, 0);
		sem_init(&mutex_read3, 0, 0);
		sem_init(&mutex_write1, 0, 0);
		sem_init(&mutex_write2, 0, 0);
		sem_init(&mutex_write3, 0, 0);
		// We use P - 1 to syncrhonize the encoding threads and the writing thread
		if (pthread_barrier_init(&barrier_wait_encode, NULL, P - 1) != 0) {
			printf("\n Barrier init has failed\n");
			return 1;
		}

		// Find the size of the text to be encoded
		size = findSize(input_message_name);

		mes = (char *)malloc(sizeof(char) * (size + 1));
		// Read the text
		readMes(argv[3], mes);

		// Create a thread for reading, one for writing and the rest for encoding
		pthread_create(&(tid[P-1]), NULL, threadRead, &(thread_id[P-1]));
		pthread_create(&(tid[P-2]), NULL, threadWrite, &(thread_id[P-2]));
		for (i = 0; i < P - 2; i++) {
			pthread_create(&(tid[i]), NULL, threadEncode, &(thread_id[i]));
		}

		for(int i = 0; i < P; i++) {
			pthread_join(tid[i], NULL);
		}

		pthread_barrier_destroy(&barrier_wait_encode);
		sem_destroy(&mutex_read1);
		sem_destroy(&mutex_read2);
		sem_destroy(&mutex_read3);
		sem_destroy(&mutex_write1);
		sem_destroy(&mutex_write2);
		sem_destroy(&mutex_write3);

		// Free memory
		free(mes);

		// Free the allocated memory for the input image
		for (i = 0; i < height1; i++) {
			free(input1.colour[i]);
		}
		free(input1.colour);
		for (i = 0; i < height2; i++) {
			free(input2.colour[i]);
		}
		free(input2.colour);
		for (i = 0; i < height3; i++) {
			free(input3.colour[i]);
		}
		free(input3.colour);

	// Case when we want to decode a text from an image
	} else if (strcmp(argv[1], "decode") == 0) {
		// Read image
		readInput(argv[2], &input);

		P = atoi(argv[4]);

		// Find the size of the message
		nrPixelsModified = calcNrOfPixels(&input);
		sizeMes = nrPixelsModified / 3;
		outMes = (char *)malloc(sizeof(char) * (sizeMes + 1));

		// Declare thread vector and the thread id vector
		pthread_t tid[P];
		int thread_id[P];
		for(i = 0; i < P; i++) {
			thread_id[i] = i;
		}

		// Decode the text from the image
		for (i = 0; i < P; i++) {
			pthread_create(&(tid[i]), NULL, threadDecode, &(thread_id[i]));
		}

		for(int i = 0; i < P; i++) {
			pthread_join(tid[i], NULL);
		}

		// Write the text into the output file
		writeMes(argv[3], outMes);

		// Free memory
		free(outMes);

		// Free the allocated memory for the input image
		for (int i = 0; i < input.height; i++) {
			free(input.colour[i]);
		}
		free(input.colour);
	}

	return 0;
}


// Thread function used by the thread that reads the input image
void* threadRead(void *var) {
	int thread_id = *(int*)var;
	int i, j;
	// Open the image file
	FILE *im = fopen(input_image_name, "rb");
	if (im == NULL) {
				exit(-1);
	}

	// We write the header of the input image into the header of each of the
	// smaller images

	// P6 -> color
	fscanf(im, "%s", input1.type);
	strcpy(input2.type, input1.type);
	strcpy(input3.type, input1.type);

	// We read the width and height of the image
	fscanf(im, "%d %d", &input1.width, &input1.height);
	input2.width = input1.width;
	input2.height = input1.height;
	input3.width = input1.width;
	input3.height = input1.height;

	// We read the max value
	fscanf(im, "%d", &input1.maxv);
	input2.maxv = input1.maxv;
	input3.maxv = input1.maxv;

	fscanf(im, "\n");

	// Check if the text can be put into the matrix (the size of the matrix is enough)
	if (size > input1.height * (input1.width - (input1.width % 3))) {
		printf("Size of message is too big.\n");
		exit(-1);
	}

	// Find the number of lines for each matrix.
	// The header from the smaller images does not have the correct hight of
	// that particular part of the whole image
	height1 = input1.height / 3;
	height2 = input1.height / 3;
	height3 = (input1.height / 3) + (input1.height % 3);

	// It ensures that the height and width of the image were read.
	pthread_barrier_wait(&barrier_wait_encode);

	// Alloc memory for the matrix of pixels
	input1.colour = (pixel **)malloc(sizeof(pixel *) * height1);
	for (i = 0; i < height1; i++)
		input1.colour[i] = (pixel *)malloc(sizeof(pixel) * input1.width);

	// Read the matrix of pixels from the file
	for (i = 0; i < height1; i++) {
		for (j = 0; j < input1.width; j++) {
			// Read the colors of a pixel
			fread(&input1.colour[i][j], 3 * sizeof(unsigned char), 1, im);
		}
	}

	// After we read the first part of the image, we realease a semaphore for
	// the corresponding amount of the encoding threads that will encode
	// this part of the image.
	for (i = 0; i < ((P - 2) / 3); i++){
		sem_post(&mutex_read1);
	}

	// Alloc memory for the matrix of pixels
	input2.colour = (pixel **)malloc(sizeof(pixel *) * height2);
	for (i = 0; i < height2; i++)
		input2.colour[i] = (pixel *)malloc(sizeof(pixel) * input2.width);

	// Read the matrix of pixels from the file
	for (i = 0; i < height2; i++) {
		for (j = 0; j < input2.width; j++) {
			// Read the colors of a pixel
			fread(&input2.colour[i][j], 3 * sizeof(unsigned char), 1, im);
		}
	}

	// After we read the second part of the image, we realease a semaphore for
	// the corresponding amount of the encoding threads that will encode
	// this part of the image.
	for (i = 0; i < ((P - 2) / 3); i++){
		sem_post(&mutex_read2);
	}

	// Alloc memory for the matrix of pixels
	input3.colour = (pixel **)malloc(sizeof(pixel *) * height3);
	for (i = 0; i < height3; i++)
		input3.colour[i] = (pixel *)malloc(sizeof(pixel) * input3.width);

	// Read the matrix of pixels from the file
	for (i = 0; i < height3; i++) {
		for (j = 0; j < input3.width; j++) {
			// Read the colors of a pixel
			fread(&input3.colour[i][j], 3 * sizeof(unsigned char), 1, im);
		}
	}

	// After we read the third part of the image, we realease a semaphore for
	// the corresponding amount of the encoding threads that will encode
	// this part of the image.
	for (i = 0; i < (((P - 2) / 3) + ((P - 2) % 3)); i++){
		sem_post(&mutex_read3);
	}

	// Close the file
	fclose(im);
}

void* threadWrite(void *var) {
	int thread_id = *(int*)var;
	int i,j;

	// After we process the first part of the image, we take a semaphore for
	// the corresponding amount of the encoding threads that encoded
	// this part of the image.
	for (i = 0; i < ((P - 2) / 3); i++){
		sem_wait(&mutex_write1);
	}

	// Open / Create file
	FILE *out = fopen(output_image_name, "wb");
	if (out == NULL) {
        exit(-1);
	}

	// Write the type of image
	fprintf(out, "%s\n", input1.type);

	// Write width and height
	fprintf(out, "%d %d\n", input1.width, input1.height);

	// Write maximum value of a channel of a pixel
	fprintf(out, "%d\n", input1.maxv);

	// Write the matrix
	for (i = 0; i < height1; i++) {
		for (j = 0; j < input1.width; j++) {
			// Write the colours of a pixel
			fwrite(&input1.colour[i][j], 3 * sizeof(unsigned char), 1, out);
		}
	}

	// After we process the second part of the image, we take a semaphore for
	// the corresponding amount of the encoding threads that encoded
	// this part of the image.
	for (i = 0; i < ((P - 2) / 3); i++){
		sem_wait(&mutex_write2);
	}

	// Write the matrix
	for (i = 0; i < height2; i++) {
		for (j = 0; j < input2.width; j++) {
			// Write the colours of a pixel
			fwrite(&input2.colour[i][j], 3 * sizeof(unsigned char), 1, out);
		}
	}

	// After we process the third part of the image, we take a semaphore for
	// the corresponding amount of the encoding threads that encoded
	// this part of the image.
	for (i = 0; i < (((P - 2) / 3) + ((P - 2) % 3)); i++){
		sem_wait(&mutex_write3);
	}

	// Write the matrix
	for (i = 0; i < height3; i++) {
		for (j = 0; j < input3.width; j++) {
			// Write the colours of a pixel
			fwrite(&input3.colour[i][j], 3 * sizeof(unsigned char), 1, out);
		}
	}

	// Close file
	fclose(out);
}

void* threadEncode(void *var) {
	int thread_id = *(int*)var;
	int i,j;
	// Used to keep the start and end position in the message vector
	// Each thread will encode the characters from start untill end - 1 position
	int start, end;
	// The start and end line of the matrix of pixel for each thread
	// Each thread will encode its characters on these lines of the pixel matrix
	int start_line, end_line;
	// The number of characters already prelucrated
	int already_prelucrated = 0;
	// The number of lines needed to encode a number of characters
	int nr_lines_needed = 0;

	// Wait until the hight and the width of the image were read
	pthread_barrier_wait(&barrier_wait_encode);

	// Find out how many characters can be encoded in the first part of the image
	int size1 = (height1 * (input1.width - (input1.width % 3))) / 3;
	if (size1 > size) {
		size1 = size;
	}

	// If the current thread is used to encode the first part of the image
	if (thread_id < ((P - 2) / 3)) {
		// Wait until the first part of the image was read
		sem_wait(&mutex_read1);

		// The number of lines needed to encode the characters
		nr_lines_needed = ceil((float)size1 / ((input1.width - (input1.width % 3)) / 3));

		// The interval of lines to be prelucrated by each thread
		start_line = ceil((double)nr_lines_needed / ((P - 2) / 3)) * thread_id;
		end_line = fmin((thread_id + 1) * ceil((double)nr_lines_needed / ((P - 2) / 3)), nr_lines_needed);

		// The portion of the input message to be encoded by each thread
		start = (start_line * (input1.width - (input1.width % 3))) / 3;
		end = (end_line * (input1.width - (input1.width % 3))) / 3;

		// Ecode the message
		encodeMes(&input1, mes, start, end, start_line);

		// After we process the first part of the image, we release the semaphore to
		// signal the write process that another encoding thread finished
		sem_post(&mutex_write1);

	}

	// Find out how many characters can be encoded in the second part of the image
	int size2 = (height2 * (input2.width - (input2.width % 3))) / 3;
	if (size2 > size - size1) {
		size2 = size - size1;
	}

	if (size2 < 0) {
		size2 = 0;
	}

	// If the current thread is used to encode the second part of the image
	if (thread_id >= ((P - 2) / 3) && thread_id < 2 * ((P - 2) / 3)) {
		// Wait for the read thread to read the second part of the image
		sem_wait(&mutex_read2);

		// This is the number of already prelucrated characters
		already_prelucrated = size1;

		// The number of lines needed to encode the characters
		nr_lines_needed = ceil((float)size2 / ((input2.width - (input2.width % 3)) / 3));

		// The interval of lines to be prelucrated by each thread
		// We use thread_id - ..... because this formulas work if the threads are
		// starting from 0
		start_line = ceil((double)nr_lines_needed / ((P - 2) / 3)) * (thread_id - ((P - 2) / 3));
		end_line = fmin((thread_id - ((P - 2) / 3) + 1) * ceil((double)nr_lines_needed / ((P - 2) / 3)), nr_lines_needed);

		// The portion of the input message to be encoded by each thread
		start = already_prelucrated + (start_line * (input2.width - (input2.width % 3))) / 3;
		end = already_prelucrated + (end_line * (input2.width - (input2.width % 3))) / 3;

		// Ecode the message
		encodeMes(&input2, mes, start, end, start_line);

		// After we process the second part of the image, we release the semaphore to
		// signal the write process that another encoding thread finished
		sem_post(&mutex_write2);
	}

	// Find out how many characters can be encoded in the first part of the image
	int size3 = (height3 * (input3.width - (input3.width % 3))) / 3;
	if (size3 > size - size1 - size2) {
		size3 = size - size1 - size2;
	}

	if (size3 < 0) {
		size3 = 0;
	}

	// If the current thread is used to encode the third part of the image
	if (thread_id >= 2 * ((P - 2) / 3) && thread_id < (P - 2)) {
		// Wait for the read thread to read the third part of the image
		sem_wait(&mutex_read3);

		// This is the number of already prelucrated characters
		already_prelucrated = size1 + size2;

		// The number of lines needed to encode the characters
		nr_lines_needed = ceil((float)size3 / ((input3.width - (input3.width % 3)) / 3));

		// The interval of lines to be prelucrated by each thread
		// We use thread_id - ..... because this formulas work if the threads are
		// starting from 0
		start_line = ceil((double)nr_lines_needed / (((P - 2) / 3) + (P - 2) % 3)) * (thread_id - (2 * ((P - 2) / 3)));
		end_line = fmin((thread_id - (2 * ((P - 2) / 3)) + 1) * ceil((double)nr_lines_needed / (((P - 2) / 3) + (P - 2) % 3)), nr_lines_needed);

		// The portion of the input message to be encoded by each thread
		start = already_prelucrated + (start_line * (input3.width - (input3.width % 3))) / 3;
		end = already_prelucrated + (end_line * (input3.width - (input3.width % 3))) / 3;

		// Ecode the message
		encodeMes(&input3, mes, start, end, start_line);

		// After we process the third part of the image, we release the semaphore to
		// signal the write process that another encoding thread finished
		sem_post(&mutex_write3);
	}
}

// Function used to encode the input text into the image
void encodeMes(image *img, char *mes, int start, int end, int start_line) {
	int i, j, indLine, indCol;
	char *outMes;

	outMes = (char *)malloc(sizeof(char) * 9);

	indLine = start_line;
	indCol = 0;

	for (i = start; i < end; i++) {
		convertCharToBinary(mes[i], outMes);
		for (j = 0; j < 8; j += 3) {
			if (outMes[j] == '0') {
				if (img->colour[indLine][indCol].R % 2 == 1) {
					img->colour[indLine][indCol].R = modifyPixelChannel(img->colour[indLine][indCol].R, 1, 0);
				}
			} else {
				if (img->colour[indLine][indCol].R % 2 == 0) {
					img->colour[indLine][indCol].R = modifyPixelChannel(img->colour[indLine][indCol].R, 0, 1);
				}
			}if (outMes[j + 1] == '0') {
				if (img->colour[indLine][indCol].G % 2 == 1) {
					img->colour[indLine][indCol].G = modifyPixelChannel(img->colour[indLine][indCol].G, 1, 0);
				}
			} else {
				if (img->colour[indLine][indCol].G % 2 == 0) {
					img->colour[indLine][indCol].G = modifyPixelChannel(img->colour[indLine][indCol].G, 0, 1);
				}
			}
			if (outMes[j + 2] == '0' && (j + 2) < 8) {
				if (img->colour[indLine][indCol].B % 2 == 1) {
					img->colour[indLine][indCol].B = modifyPixelChannel(img->colour[indLine][indCol].B, 1, 0);
				}
			} else if ((j + 2) < 8) {
				if (img->colour[indLine][indCol].B % 2 == 0) {
					img->colour[indLine][indCol].B = modifyPixelChannel(img->colour[indLine][indCol].B, 0, 1);
				}
			} else {
				if (i == size - 1) {
					if (img->colour[indLine][indCol].B % 2 == 0) {
						img->colour[indLine][indCol].B++;
					}
				} else {
					if (img->colour[indLine][indCol].B % 2 == 1) {
						img->colour[indLine][indCol].B--;
					}
				}
			}
			indCol++;

			if (indCol > img->width - (img->width % 3) - 1) {
				indCol = 0;
				indLine++;
			}
		}

	}

	free(outMes);
}

void* threadDecode(void *var) {
	int thread_id = *(int*)var;
	int i, j;
	int stop = 0, indexChar = 0;
	int pos = 0;
	char tempMes[9];

	// The number of lines that contain encoded pixels
	int nr_lines_needed = ceil((float)nrPixelsModified / (input.width - (input.width % 3)));
	// The lines to be processed by the current thread
	int start_line = ceil((double)nr_lines_needed / P) * thread_id;
	int end_line = fmin((thread_id + 1) * ceil((double)nr_lines_needed / P), nr_lines_needed);

	// Beginning position in the output message array for the current thread
	pos = start_line * ((input.width - (input.width % 3)) / 3);

	for (i = start_line; i < end_line; i++) {
		for (j = 0; j < input.width - (input.width % 3); j++) {
			if (input.colour[i][j].R % 2 == 0) {
				tempMes[indexChar++] = '0';
			} else {
				tempMes[indexChar++] = '1';
			}

			if (input.colour[i][j].G % 2 == 0) {
				tempMes[indexChar++] = '0';
			} else {
				tempMes[indexChar++] = '1';
			}

			if (input.colour[i][j].B % 2 == 0 && ((i * (input.width - (input.width % 3)) + j) % 3 != 2)) {
				tempMes[indexChar++] = '0';
			} else if (((i * (input.width - (input.width % 3)) + j) % 3 != 2)) {
				tempMes[indexChar++] = '1';
			} else {
				if (input.colour[i][j].B % 2 == 1) {
					stop = 1;
					break;
				}
			}

			if (indexChar > 7) {
				tempMes[indexChar] = '\0';
				convertBinaryToChar_decode(tempMes, pos);
				pos++;
				indexChar = 0;
			}
		}

		if (stop == 1) {
			convertBinaryToChar_decode(tempMes, pos);
			pos++;
			outMes[pos] = '\0';
			break;
		}
	}
}

// Function used to read the image
void readInput(const char * fileName, image *img) {
	int i, j;

	// Open the image file
	FILE *im = fopen(fileName, "rb");
	if (im == NULL) {
        return;
	}

	// P6 -> color
	fscanf(im, "%s", img->type);

	// We read the width and height of the image
	fscanf(im, "%d %d", &img->width, &img->height);

	// We read the max value
	fscanf(im, "%d", &img->maxv);
	fscanf(im, "\n");

	// Alloc memory for the matrix of pixels
	img->colour = (pixel **)malloc(sizeof(pixel *) * img->height);
	for (i = 0; i < img->height; i++)
		img->colour[i] = (pixel *)malloc(sizeof(pixel) * img->width);

	// Read the matrix of pixels from the file
	for (i = 0; i < img->height; i++) {
		for (j = 0; j < img->width; j++) {
			// Read the colors of a pixel
			fread(&img->colour[i][j], 3 * sizeof(unsigned char), 1, im);
		}
	}

	// Close the file
	fclose(im);
}

// Function used to write the output message into the file
void writeMes(const char * fileName, char *mes) {

	FILE *out = fopen(fileName, "wt");
	if (out == NULL) {
        return;
	}

	fprintf(out, "%s", mes);
	fclose(out);
}

// Read the input message
void readMes(const char * fileName, char *mes) {
	int c;
	size_t n = 0;

	FILE *fp = fopen(fileName, "rt");
	while ((c = fgetc(fp)) != EOF) {
		mes[n++] = (char)c;
	}

	mes[n] = '\0';
	fclose(fp);
}

// Find size of input text
int findSize(const char * fileName) {
	int size = 0;

	FILE *fp = fopen(fileName, "rb");
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	fclose(fp);
	return size;
}

// Function used to convert a character into a binary string
void convertCharToBinary(char c, char *outMes) {
	int i, index;

	index = 0;
	for (i = 7; i >= 0; --i) {
        outMes[index++] = (c & (1 << i)) ? '1' : '0';
    }

    outMes[index] = '\0';
}

// Function used to convert a binary string into a character
// Used by the decode function, it adds the character to the decoded text
void convertBinaryToChar_decode(char tempMes[9], int pos) {
	char *start = &tempMes[0];
	int total = 0;
	char c;

	while (*start) {
 		total *= 2;
 		if (*start++ == '1') total += 1;
	}

	c = total;
	outMes[pos] = c;
}

// Function used to convert a binary string into a character
// Used by the encode function, it returns the character
char convertBinaryToChar_encode(char tempMes[9]) {
	char *start = &tempMes[0];
	int total = 0;
	char c;

	while (*start) {
 		total *= 2;
 		if (*start++ == '1') total += 1;
	}

	c = total;
	return c;
}

// Function used to modify the last bit of a colour of a pixel
// parity is the pixel's colour parity and mesParity is the parity of the bit to be encoded
unsigned char modifyPixelChannel(unsigned char p, int parity, int mesParity) {
	char outMes[9];

	convertCharToBinary(p, outMes);

	if (mesParity == 0 && parity == 1) {
		outMes[7] = '0';
	} else if (mesParity == 1  && parity == 0) {
		outMes[7] = '1';
	}

	p = convertBinaryToChar_encode(outMes);

	return p;
}

// Function used to find the number of pixels modified in the processed image
// The pixels modified in order to encode the text
int calcNrOfPixels(image *img) {
	int i, j, size = 0;
	int stop = 0;

	for (i = 0; i < img->height; i++) {
		for (j = 0; j < (img->width - (img->width % 3)); j++) {
			if (img->colour[i][j].B % 2 == 1 && (i * (img->width - (img->width % 3)) + j) % 3 == 2) {
				stop = 1;
				size++;
				break;
			} else {
				size++;
			}
		}
		if (stop == 1)
			break;
	}

	return size;
}
