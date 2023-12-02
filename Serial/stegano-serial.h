/* Copyright [2020]
Gavan Adrian-George, 342C1
David Anca-Iulia, 342C1
Costin Nicolae-Mircea, 341C4 */
#ifndef HOMEWORK_H
#define HOMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Write the output image
void writeData(const char * fileName, image *img) {
	int i,j;

	// Open / Create file
	FILE *out = fopen(fileName, "wb");
	if (out == NULL) {
        return;
	}

	// Write the type of image
	fprintf(out, "%s\n", img->type);

	// Write width and height
	fprintf(out, "%d %d\n", img->width, img->height);

	// Write maximum value of a channel of a pixel
	fprintf(out, "%d\n", img->maxv);

	// Write the matrix
	for (i = 0; i < img->height; i++) {
		for (j = 0; j < img->width; j++) {
			// Write the colours of a pixel
			fwrite(&img->colour[i][j], 3 * sizeof(unsigned char), 1, out);
		}
	}

	// Close file
	fclose(out);
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
void convertBinaryToChar(char *mes, char tempMes[9], int pos) {
	char *start = &tempMes[0];
	int total = 0;
	char c;

	while (*start) {
 		total *= 2;
 		if (*start++ == '1') total += 1;
	}

	c = total;
	mes[pos] = c;
}

// Function used to convert a binary string into a character
// Used by the encode function, it returns the character
char convertBinaryToChar2(char tempMes[9]) {
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
	//outMes = (char *)malloc(sizeof(char) * 9);

	convertCharToBinary(p, outMes);

	if (mesParity == 0 && parity == 1) {
		outMes[7] = '0';
	} else if (mesParity == 1  && parity == 0) {
		outMes[7] = '1';
	}

	p = convertBinaryToChar2(outMes);

	//free(outMes);
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

// Function used to write the output message into the file
void writeMes(const char * fileName, char *mes) {

	FILE *out = fopen(fileName, "wt");
	if (out == NULL) {
        return;
	}

	fprintf(out, "%s", mes);
	fclose(out);
}

// Function used to encode the input text into the image
void encodeMes(image *img, char *mes, int size) {
	int i, j, indLine, indCol;
	char *outMes;

	outMes = (char *)malloc(sizeof(char) * 9);

	indLine = 0, indCol = 0;

	for (i = 0; i < size; i++) {
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

// Function used to decode the text from the image
void decodeMes(image *img, char *mes, int nrPixels) {
	int i, j;
	int stop = 0, indexChar = 0;
	int pos = 0;
	char tempMes[9];

	for (i = 0; i < img->height; i++) {
		for (j = 0; j < img->width - (img->width % 3); j++) {
			if (img->colour[i][j].R % 2 == 0) {
				tempMes[indexChar++] = '0';
			} else {
				tempMes[indexChar++] = '1';
			}

			if (img->colour[i][j].G % 2 == 0) {
				tempMes[indexChar++] = '0';
			} else {
				tempMes[indexChar++] = '1';
			}

			if (img->colour[i][j].B % 2 == 0 && ((i * (img->width - (img->width % 3)) + j) % 3 != 2)) {
				tempMes[indexChar++] = '0';
			} else if (((i * (img->width - (img->width % 3)) + j) % 3 != 2)) {
				tempMes[indexChar++] = '1';
			} else {
				if (img->colour[i][j].B % 2 == 1) {
					stop = 1;
					break;
				}
			}

			if (indexChar > 7) {
				tempMes[indexChar] = '\0';
				convertBinaryToChar(mes, tempMes, pos);
				pos++;
				indexChar = 0;
			}
		}
		if (stop == 1) {
			convertBinaryToChar(mes, tempMes, pos);
			pos++;
			mes[pos] = '\0';
			break;
		}
	}
}
#endif /* HOMEWORK_H */
