#ifndef CONTENT_SERVER_CONSTANTS_H
#define CONTENT_SERVER_CONSTANTS_H

// Cmd Parser Modes
#define MODE_MS 100
#define MODE_MI 101
#define MODE_CS 102

// ContentServer information offsets
#define CS_ADDR 0
#define CS_PORT 1
#define CS_DIR 2
#define CS_DELAY 3

// Block Sizes (for raw data and for strings
#define BLOCK_SIZE 1024
#define BLOCK_RAW_SIZE 1024
#define BLOCK_STR_SIZE 1023

#define Q_MAX 256

#define MTX_RW 0
#define MTX_F 1
#define MTX_E 2
#define MTX_DONE 3

#endif //CONTENT_SERVER_CONSTANTS_H
