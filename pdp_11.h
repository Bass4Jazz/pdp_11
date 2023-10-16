#ifndef PDP_11_H
#define PDP_11_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>

#define MEMSIZE (64 * 1024)
#define pc reg[7]
#define sp reg[6] //<----------------------
#define LOG_TRACE 0
#define LOG_DEFAULT 1
#define OSTAT 0177564 
#define ODATA 0177566 

typedef unsigned char byte;
typedef unsigned short word;
typedef word address;
extern word reg[];
extern byte mem[];
extern int log_level;
typedef struct 
{
    word mask;
    word opcode;
    char* name;
    char params;
    void (*do_command)();
} Command;
typedef struct {
    word val;     
    address adr;    
} Arg;
extern Arg ss;
extern Arg dd;
extern byte nn; 
extern byte rp;
extern byte xx;  
extern byte byte_cmd; 
extern word set_fl; 
extern byte flag_N; 
extern byte flag_Z; 
extern byte flag_V; 
extern byte flag_C;

void test_mem(void);
void mem_dump(address adr, int size);
void reg_dump(void);
void b_write (address adr, byte val);
byte b_read (address adr);
void w_write (address adr, word val);
word w_read (address adr);
void load_data(FILE* the_stream);
void load_file(char* file_name);
void trace(int level, const char* fmt, ...);
int set_log_level(int level);
void do_halt(void); 
void do_add(void); 
void do_mov(void);
void do_sob(void); 
void do_clear(void);
void do_set_flag(void);  
void do_br(void);
void do_beq(void);
void do_bpl(void);
void do_tst(void);
void do_jsr(void);
void do_rts(void);
void set_NZ(void);
void set_C(char);
void run(void);
Arg get_mr(word w);

#endif