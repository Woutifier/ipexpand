#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>

const char *argp_program_version = "ipexpand 0.1";
const char *argp_program_bug_address = "<w.b.devries@utwente.nl>";

static char doc[] = "ipexpand -- expand csv file with ip addresses to /24's\r\nExpects an ip address in the first column and a subnet in the second";
static char args_doc[] = "inputfile";

static struct argp_option options[] = {
    {"seperator", 's', "SEPERATOR", 0, "Use specified seperator for CSV input"},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output"},
    {"singlecolumn", 'c', 0, 0, "Read IP AND Subnet from the first column (format: ip/subnet)"},
    {0}
};

struct arguments {
    char *args[1];
    char *seperator;    
    char *output_file;
    bool singlecolumn;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch(key) {
        case 'o':
            arguments->output_file = arg;
            break;
        case 's':
            if (strlen(arg) > 1)
                argp_usage(state);
            arguments->seperator = arg;
            break;
        case 'c':
            arguments->singlecolumn = true;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
                argp_usage(state);
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 1)
                argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

static char* uint_to_ip(unsigned int ipstr) {
    char* ip = (char*)malloc(sizeof(char)*16);
    sprintf(ip, "%d.%d.%d.%d", ipstr>>24&0xff, ipstr>>16&0xff, ipstr>>8&0xff, ipstr&0xff);
    return ip;
}

static unsigned int ip_to_uint(char* ipstr) {
    unsigned int ip = 0;
    char* first = strtok(ipstr,".");
    char* second = strtok(NULL,".");
    char* third = strtok(NULL,".");
    char* fourth = strtok(NULL,".");
    ip |= atoi(first) << 24;
    ip |= atoi(second) << 16;
    ip |= atoi(third) << 8;
    ip |= atoi(fourth);
    return ip;
}

static int processline(char* line, char* seperator, bool singlecolumn) {

    char* ip = strtok(line,seperator);
    int subnet;
    char* remainder;
    
    if (singlecolumn ) {
        remainder = strtok(NULL,seperator);
        strtok(ip,"/");
        subnet = atoi(strtok(NULL,"/"));
    } else {
        subnet = atoi(strtok(NULL,seperator));
        remainder = strtok(NULL,"");
    }
    remainder[strcspn(remainder,"\r\n")] = 0;
    unsigned int lowerlimit = ip_to_uint(ip)&0xffffffff<<(32-subnet);

    if (subnet < 24) {
        unsigned int upperlimit = lowerlimit | 0xffffffff>>subnet;
        for (unsigned int i = lowerlimit; i<=upperlimit; i+=0x100) {
            char* newip = uint_to_ip(i);
            printf("%s%s24%s%s\r\n",newip,singlecolumn ? "/" : seperator,seperator,remainder);
            free(newip);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct arguments arguments;
    
    //Default values for arguments
    arguments.output_file = "-";
    arguments.seperator = ",";
    arguments.singlecolumn = false;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    FILE *inputFile;
    inputFile = fopen(arguments.args[0], "r");
    if (inputFile == NULL) {
        perror("Error");
        return -1;
    }
    
    char buffer[1000];

    while (!feof(inputFile)) {
        if (fgets(buffer, 1000, inputFile) != NULL) {
            processline(buffer,arguments.seperator,arguments.singlecolumn);
        }
    }
    fclose(inputFile);
    return 0;
}
