#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

// chunk size will be 4096
#define CHUNK_SIZE 16
// world will start by just having 512 chunks = 32768 bytes
// height is 128
#define CHUNK_DIM 8

typedef struct World {
    Grid grid;
};

typedef struct Grid {
    Chunk chunks[CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];
};

typedef struct Chunk {
    char blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
};

// if I were to make chunks the way that I used to then we could have up to
// 100663296 faces = 100 million faces = 603979776 vertices = 604 million vertices = 7247757312 bytes = 7 GB
// that is probably not feasible
// but how can I do the memory management required to 
// I think I will start less ambitious and see how I can scale up once I have things working
//
// so what I want to see if I can make happen is when we change blocks, we change just one byte per block
// and then a compute shader will generate the vertices for each chunk
// and then that data is passed to the rendering pipeline

#endif
