#include "pdp_11.h"

int log_level;
word reg[8];
byte mem[MEMSIZE];
Arg ss, dd;
word set_fl; 
byte nn, rp, xx, byte_cmd = 0, flag_N, flag_Z, flag_V, flag_C;  

int main(int argc, char** argv)
{    
    char ch_arg;
    b_write(OSTAT, 0377); 
    
    while ((ch_arg = getopt (argc, argv, "dt")) != -1)
    {    
        switch (ch_arg)
        {        
            case 't':
                log_level = LOG_TRACE;
                break;
            case 'd':
                log_level = LOG_DEFAULT;
                break;
        }
    }         
    
    load_file(argv[optind]);
    run();
    
    return 0;    
}