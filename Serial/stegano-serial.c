/* Copyright [2020]
Gavan Adrian-George, 342C1
David Anca-Iulia, 342C1
Costin Nicolae-Mircea, 341C4 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stegano-serial.h"

int main(int argc, char *argv[]) {
	image input;
	int size, i, nrPixelsModified, sizeMes;
	// Variable used to store the input text (encode)
	char *mes;
	// Variable used to store the output text (decode)
	char *outMes;

	// Case when we want to encode a text into an image
	if (strcmp(argv[1], "encode") == 0) {
		// Read image
		readInput(argv[2], &input);
		// Find the size of the text to be encoded
		size = findSize(argv[3]);
		// Check if the text can be put into the matrix (the size of the matrix is enough)
		if (size > input.height * (input.width - (input.width % 3))) {
			return -1;
		}

		mes = (char *)malloc(sizeof(char) * (size + 1));
		// Read the text
		readMes(argv[3], mes);
		// Encode the text
		encodeMes(&input, mes, size);
		// Write the output image
		writeData(argv[4], &input);
		// Free memory
		free(mes);

	// Case when we want to decode a text from an image
	} else if (strcmp(argv[1], "decode") == 0) {
		// Read image
		readInput(argv[2], &input);
		// Find the size of the message
		nrPixelsModified = calcNrOfPixels(&input);
		sizeMes = nrPixelsModified / 3;

		outMes = (char *)malloc(sizeof(char) * (sizeMes + 1));
		// Decode the text from the image
		decodeMes(&input, outMes, nrPixelsModified);
		// Write the text into the output file
		writeMes(argv[3], outMes);
		// Free memory
		free(outMes);
	}

	// Free the allocated memory for the input image
	for (int i = 0; i < input.height; i++) {
		free(input.colour[i]);
	}
	free(input.colour);
	return 0;
}
