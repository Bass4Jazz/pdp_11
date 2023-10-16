#include "pdp_11.h"
#define BREAK 0111111
#define NO_PARAMS 0 
#define HAS_DD 1 
#define HAS_SS 2
#define HAS_NN 4 
#define HAS_R 8 
#define HAS_XX 16 
static char find = 0;

Command command[] = {
    {0170000, 0060000, "add", HAS_SS | HAS_DD, do_add}, 
    {0170000, 0010000, "mov", HAS_SS | HAS_DD, do_mov},
    {0170000, 0110000, "movb", HAS_SS | HAS_DD, do_mov}, 
    {0177000, 0077000, "sob", HAS_R | HAS_NN, do_sob},  
    {0177700, 0005000, "clr", HAS_DD, do_clear},     
    {0177700, 0000300, "set_flag", NO_PARAMS, do_set_flag}, 
    {0177400, 0000400, "br", HAS_XX, do_br}, 
    {0177400, 0001400, "beq", HAS_XX, do_beq},
    {0177400, 0100000, "bpl", HAS_XX, do_bpl}, 
    {0177700, 0005700, "tst", HAS_DD, do_tst}, 
    {0177700, 0105700, "tstb", HAS_DD, do_tst}, 
    {0177000, 0004000, "jsr", HAS_R | HAS_DD, do_jsr}, // <---------------------
    {0177770, 0000200, "rts", HAS_R, do_rts}, // <---------------------
    {0177777, 0000000, "halt", NO_PARAMS, do_halt},
    {BREAK, BREAK, "break", NO_PARAMS, NULL} // Ловушка! Встретив, - выходим из цикла!
};

void b_write (address adr, byte val)
{
    if(adr < 8) 
    {
        if(!(val >> 7))
        {
            reg[adr] = val;
            return;
        }
        reg[adr] = (0377 << 8) | val;
    }
    mem[adr] = val;
    if(adr == ODATA && log_level == LOG_DEFAULT)
        printf("%c", val);
}

byte b_read (address adr)
{
    return mem[adr];
}

void w_write (address adr, word val)
{
    if(adr < 8)
    {
        reg[adr] = val;
        return;
    }
    mem[adr] = (byte)val;
    mem[adr + 1] = (byte)(val >> 8);
}

word w_read (address adr)
{
    word w;
    if(adr < 8) 
        return w = reg[adr];
    w = mem[adr + 1] << 8;
    return w |=  mem[adr];
}

void load_data(FILE* the_stream)
{
    word adr, size; 
    byte data;                        
    while(fscanf(the_stream, "%hx%hx", &adr, &size) == 2)      
        for(address i = adr; i < adr + size; i++)
        {
            fscanf(the_stream, "%hhx", &data);
            b_write (i, data);            
        }
}

void load_file(char* file_name)
{
    FILE* the_stream;

    if(file_name)
    {
        the_stream = fopen(file_name, "r");
        if(!the_stream)
        {
            perror(file_name);
            exit(1);
        }
    }
    else
        the_stream = stdin;    
    load_data(the_stream);
    fclose(the_stream);   
}

void trace(int level, const char* fmt, ...)
{
    if(level >= log_level)
    {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
}

int set_log_level(int level)
{
    int prev = log_level;
    log_level = level;
    
    return prev;
}

void do_halt(void)
{
    if(log_level == LOG_DEFAULT)
        putchar('\n');
    reg_dump();
    exit(0);
}

void do_add(void)
{
    w_write(dd.adr, ss.val + dd.val);
    set_NZ(); 
    set_C('a'); 
}

void do_mov(void) 
{
    if(byte_cmd) 
        b_write(dd.adr, ss.val);
    else
        w_write(dd.adr, ss.val);
    set_NZ(); 
}

void do_sob(void)
{
    reg[rp] -= 1;
    if(reg[rp] > 0)
        pc -= nn << 1;
    trace(LOG_TRACE, "R%o %o", rp, pc);
}

void do_clear(void)
{
    if(dd.adr < 8)
        reg[dd.adr] = 0;
    else
        w_write(dd.adr, 0);
}

void chek_set_st(char name, byte state, byte* flag) 
{
    if(name == 'A')
    {
        if(state)
            trace(LOG_TRACE, " SCC\n", name);
        else
            trace(LOG_TRACE, " CCC\n", name);
        flag_N = flag_Z = flag_V = flag_C = state;
        return;
    }
    if(state)
        trace(LOG_TRACE, " SE%c\n", name);
    else
        trace(LOG_TRACE, " CL%c\n", name);
    *flag = state;
}

void do_br(void) 
{
    pc += (signed char)xx << 1;
    trace(LOG_TRACE, "%o", pc); 
}

void do_beq(void) 
{
   
    if(flag_Z)
        do_br();
    else
        trace(LOG_TRACE, "%o", pc + (xx << 1)); 
}

void do_bpl(void) 
{
    if (flag_N == 0)
        do_br();
    else
        trace(LOG_TRACE, "%o", (signed char)pc + (xx << 1)); 
}

void do_tst(void) 
{
    set_NZ();
    flag_V = flag_C = 0;
}

void set_NZ(void) 
{
    if(dd.adr < 8 || !byte_cmd)
    {            
        //flag_N = reg[dd.adr] >> 017;
        //flag_Z = reg[dd.adr] ? 0 : 1;
        flag_N = w_read(dd.adr) >> 017; 
        flag_Z = w_read(dd.adr) ? 0 : 1; 
        return;
    }   
    flag_N = b_read(dd.adr) >> 07;
    flag_Z = b_read(dd.adr) ? 0 : 1;        
}

void set_C(char op) 
{
    if(op == 'a')
    {
        if(((ss.val + dd.val) > 65535) || 
           ((ss.val + dd.val) < -32768))
        {
            flag_C = 1;
            return;
        }
        flag_C = 0;
        return;
    }
}

void do_set_flag(void) 
{
    enum {n = 010, z = 04, v = 02, c = 01, all = 017};
    byte state = set_fl >> 4 & 1;

    switch(set_fl & 017)
    {
        case n:
            chek_set_st('N', state, &flag_N);
            break;
        case z:
            chek_set_st('Z', state, &flag_Z);
            break;
        case v:
            chek_set_st('V', state, &flag_V);
            break;
        case c:
            chek_set_st('C', state, &flag_C);
            break;
        case all:
            chek_set_st('A', state, NULL);
            break;
    }
}

void do_jsr(void) //<------------------------------
{
    sp -= 2;
    w_write(sp, reg[rp]);
    w_write(rp, pc);
    pc = dd.adr;
}

void do_rts(void) // <----------------
{
    pc = w_read(rp);
    w_write(rp, w_read(sp));
    sp += 2;
}

void run(void)
{
    pc = 01000;
    while(1)
    {
        word w = w_read(pc);
        trace(LOG_TRACE, "%06o %06o: ", pc, w);
        pc += 2;        
        for(int i = 0; command[i].opcode != BREAK; i++)
        {
            if((w & command[i].mask) == command[i].opcode)
            {                
                byte_cmd = command[i].opcode >> 017; 
                trace(LOG_TRACE, "%s ", command[i].name);
                if(command[i].params & HAS_SS)
                    ss = get_mr(w >> 6);
                if(command[i].params & HAS_DD)
                    dd = get_mr(w);
                if(command[i].params & HAS_NN) 
                    nn = w & 077;
                if(command[i].params & HAS_R)
                { 
                    if(command[i].opcode == 0000200) // <---------------------
                        rp = w & 7;
                    else
                        rp = w >> 6 & 7;
                }
                if(command[i].opcode == 0000300) 
                    set_fl = w & 037;
                if(command[i].params & HAS_XX)  
                    xx = w & 0377;                   
                command[i].do_command();
                find = 1;
            }            
        }
        trace(LOG_TRACE, "\n");
        find ? find = 0 : trace(LOG_TRACE, "unknown \n");
    }
}

void reg_dump(void)
{
    trace(LOG_TRACE, "\n");
    for(int i = 0; i < 8; i++)
        trace(LOG_TRACE, "r[%o]:%o ", i, reg[i]);        
    trace(LOG_TRACE, "\n");
    trace(LOG_TRACE, "THE END!\n");
}

Arg get_mr(word w)
{
    Arg res;
    int r = w & 7;          
    int m = (w >> 3) & 7;
    switch (m) 
    {
    // мода 0, Rn
        case 0:
            res.adr = r;        
            res.val = reg[r];   
            trace(LOG_TRACE, "R%d ", r);
            break;
        // мода 1, (Rn)
        case 1:
            res.adr = reg[r];           
            res.val = w_read(res.adr);  
            trace(LOG_TRACE, "(R%d) ", r);
            break;
    // мода 2, (Rn)+ или #3
        case 2:              
            if(byte_cmd && r <= 5) 
            {    
                res.adr = reg[r];           
                res.val = b_read(res.adr);
                reg[r] += 1;
            }
            else if(byte_cmd)
            {
                res.adr = reg[r];           
                res.val = b_read(res.adr);
                reg[r] += 2;
            }
            else
            {
                res.adr = reg[r];           
                res.val = w_read(res.adr);
                reg[r] += 2;                        
            }
            if (r == 7)               
                trace(LOG_TRACE, "#%o ", res.val);
            else
                trace(LOG_TRACE, "(R%d)+ ", r);
            break;
    // мода 3, @(Rn)+ или @#202
        case 3:
            res.adr = reg[r];
            res.adr = w_read(res.adr);
            res.val = w_read(res.adr);
            reg[r] += 2;
            if(r == 7)
            {
                if(res.adr ==  OSTAT) 
                    trace(LOG_TRACE, "@#OSTAT ");
                else if(res.adr ==  ODATA)
                    trace(LOG_TRACE, "@#ODATA ");
                else  
                    trace(LOG_TRACE, "@#%o ", res.adr);
            }
            else
                trace(LOG_TRACE, "@(R%d)+ ", r);
            break;
    // мода 4, -(Rn)
        case 4:
            if(byte_cmd && r <= 5) 
            {
                reg[r] -= 1;
                res.adr = reg[r];
                res.val = b_read(res.adr);
            }
            else if(byte_cmd)
            {
                reg[r] -= 2;               
                res.adr = reg[r];
                res.val = b_read(res.adr);
            }
            else
            {
                reg[r] -= 2;               
                res.adr = reg[r];
                res.val = w_read(res.adr);
            }
            trace(LOG_TRACE, "-(R%d) ", r);
            break;
    // мода 5, @-(Rn)
        case 5:
            reg[r] -= 2;                
            res.adr = reg[r];
            res.adr = w_read(res.adr);
            res.val = w_read(res.adr);
            trace(LOG_TRACE, "@-(R%d) ", r);
            break;
    // мода 6, X(Rn)
        case 6:
            signed short x = w_read(pc); //<----------------
            pc += 2;
            res.adr = reg[r];
            res.adr += x;
            res.val = w_read(res.adr);
            if(r == 7)
                trace(LOG_TRACE, "%o ", res.adr);
            else
                trace(LOG_TRACE, "%o(R%d) ", x, r);
            break;
    // мода 7, @X(Rn)
        /*case 7:
            signed short x = w_read(pc); //<----------------
            pc += 2;
            res.adr = reg[r];
            res.adr += x;
            res.adr = w_read(res.adr);
            res.val = w_read(res.adr);
            if(r == 7)
                trace(LOG_TRACE, "@%o ", res.adr);
            else
                trace(LOG_TRACE, "%o@(R%d) ", x, r);
            break;*/
    // мы еще не дописали другие моды
        default:
            trace(LOG_TRACE, "Mode %d not implemented yet!\n", m);
            exit(1);
    }

    return res;
}