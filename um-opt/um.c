/* 
 * um.c file
 * Implementation of the universal machine
 *         by: Alexander Koudijs and
 *            Nikola Telkedzhiev
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <mem.h>

enum commands { C_MOVE, SEG_LOAD, SEG_STORE, ADD, MULT, DIV, NAND, 
                HALT, MAP_SEG, UNMAP_SEG, OUTPUT, INPUT, LOAD_PROG, LOAD_VAL};

typedef struct segment{
        uint32_t* arr;
        int32_t size;
}segment;

/*
 *
 * Improvements:
 * We merged all the function from all the files we previously had into
 * main. Instead of using bitpack we manually shift the bits. Another area
 * of improvement, is the use of an ordered series of else-if statements
 * instead of a single switch statement.
 *
 * Architecture:
 * The segmented memory is represented as an array of structs which hold 
 * another dynamic array of uint32s (to store words) and also uin32 to 
 * store the size of each segment.
 *
 */
int main(int argc, const char *argv[]) 
{
        //check for correct input
        assert(argc == 2);
        const uint32_t INIT_MEM_SIZE = 1000; //better to use local than
                                                //global variables
        const char *fname = argv[1]; //get file name
        int c1, c2, c3, c4;
        struct stat st;
        stat(fname, &st);
        size_t fsize = st.st_size / 4; //size of file
        FILE *fp = fopen(fname, "rb");        
        c1 = getc(fp);
        uint32_t i_prog = 0;
        segment *segments = calloc(INIT_MEM_SIZE, sizeof(segment));
        segments[0].arr = calloc(fsize, 4);
        segments[0].size = fsize;
        while(c1 != EOF) {                //get chars from file
                c2 = getc(fp);
                assert(c2 != EOF);
                c3 = getc(fp);
                assert(c3 != EOF);
                c4 = getc(fp);
                assert(c4 != EOF);
                segments[0].arr[i_prog] = (segments[0].arr[i_prog] | c1) << 24;
                segments[0].arr[i_prog] = segments[0].arr[i_prog] | (c2 << 16); 
                segments[0].arr[i_prog] = segments[0].arr[i_prog] | (c3 << 8); 
                segments[0].arr[i_prog] = segments[0].arr[i_prog] | c4; 
                i_prog++;
                c1 = getc(fp);
        }
        fclose (fp);
        uint32_t *available_segments = 
                malloc(INIT_MEM_SIZE * sizeof(uint32_t));
        uint32_t i_seg = 1;
        uint32_t size_seg = INIT_MEM_SIZE;
        int32_t size_avail = INIT_MEM_SIZE;
        int32_t i_avail = -1;
        uint32_t registers[8];  //registers
        for (unsigned i = 7; i--;) registers[i] = 0; //init registers 
        uint32_t program_counter = 0;
        uint32_t opcode; //different variables for later use
        uint32_t reg_a;
        uint32_t reg_b;
        uint32_t reg_c;
        uint32_t word;
        while(1) {
                word = segments[0].arr[program_counter];
                opcode = word >> 28;
                reg_a = (word << 23) >> 29;
                reg_b = (word << 26) >> 29;
                reg_c = (word << 29) >> 29;
                if (opcode == LOAD_VAL) {
                        registers[(word << 4) >> 29] = (word << 7) >> 7;
                        program_counter++;
                } else if(opcode == SEG_LOAD) { //SEGMENT_LOAD
                        registers[reg_a] = 
                                segments[registers[reg_b]].
                                arr[registers[reg_c]];
                        program_counter++;
                } else if (opcode == SEG_STORE)  {//SEGMENT_STORE
                        segments[registers[reg_a]].arr[registers[reg_b]] = 
                                registers[reg_c];
                        program_counter++;
                } else if (opcode == NAND) { //NAND:
                        registers[reg_a] = ~((registers[reg_b]) & 
                                             (registers[reg_c]));
                        program_counter++;
                } else if (opcode == ADD)  {//ADDITION:
                        registers[reg_a] = (registers[reg_b] +
                                            registers[reg_c]);
                        program_counter++;
                } else if (opcode == LOAD_PROG) {
                        if (registers[reg_b] != 0) {
                                free(segments[0].arr);
                                segments[0].arr = calloc(segments
                                                         [registers[reg_b]]
                                                         .size, 
                                                         sizeof(uint32_t));
                                memcpy(segments[0].arr, segments[registers
                                                                 [reg_b]].arr,
                                       segments[registers[reg_b]].size 
                                       * sizeof(uint32_t));
                                segments[0].size = segments[registers[reg_b]].
                                        size;
                        }
                        program_counter = registers[reg_c];    
                } else if (opcode == C_MOVE) { // CONDITIONAL_MOVE
                        if(registers[reg_c] != 0) {
                                registers[reg_a] = registers[reg_b];
                        }
                        program_counter++;
                 } else if (opcode == MAP_SEG) {  //MAP_SEGMENT:        
                        if (i_avail == -1) {
                                if(i_seg == size_seg) {
                                        segment *temp = calloc((size_seg * 14) 
                                                               / 10, 
                                                             sizeof(segment));
                                        memcpy(temp, segments, size_seg 
                                               * sizeof(segment));
                                        free(segments);
                                        segments = temp;
                                        size_seg = (size_seg * 14) / 10;
                                }
                                segments[i_seg].arr = calloc(registers[reg_c],
                                                            sizeof(uint32_t));
                                segments[i_seg].size = registers[reg_c];
                                registers[reg_b] = i_seg;
                                i_seg++;
                        } else {
                                uint32_t i_temp = available_segments[i_avail];
                                segments[i_temp].arr = calloc(registers[reg_c],
                                                             sizeof(uint32_t));
                                segments[i_temp].size = registers[reg_c];
                                registers[reg_b] = i_temp;
                                i_avail--;
                        }
                        program_counter++;
                } else if (opcode == UNMAP_SEG) { //UMMAP_SEGMENT
                        if ((size_avail - 2) == i_avail) { ///look at -2
                                uint32_t *temp2 = calloc(((size_avail * 14) 
                                                          / 10), 4);
                                memcpy(temp2, available_segments, size_avail 
                                       * sizeof(uint32_t));
                                free(available_segments);
                                available_segments = temp2;
                                size_avail = (size_avail * 14) / 10;
                        }
                        free(segments[registers[reg_c]].arr);
                        segments[registers[reg_c]].size = -1;
                        i_avail++;
                        available_segments[i_avail] = registers[reg_c];        
                        program_counter++;
                } else if (opcode == DIV)  {//DIVISION:
                        registers[reg_a] = registers[reg_b] 
                                / registers[reg_c];
                        program_counter++;
                } else if (opcode == MULT) { //MULT
                        registers[reg_a] = registers[reg_b] *
                                registers[reg_c];
                        program_counter++;
                } else if (opcode == OUTPUT) { //OUTPUT
                        assert(registers[reg_c] <= 255);
                        putchar((char)registers[reg_c]);
                        program_counter++;
                } else if (opcode == INPUT) { //INPUT
                        int c = getchar();
                        if (c == EOF)   { // when the end of input is reached
                                registers[reg_c] = ~0; // a full 32-bit word 
                                //(every bit is 1)
                        } else {
                                registers[reg_c] = (uint32_t) c;
                        }
                        program_counter++; 
                } else if (opcode == HALT) { //HALT:
                        for(unsigned i = 0; i < size_seg; i++) {
                                if(segments[i].size != -1) {
                                        free(segments[i].arr);
                                }
                        }
                        free(segments);
                        free(available_segments);
                        return 0;
                }
            }
        return 0;
}

