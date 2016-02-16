#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>

const char *argp_program_version = "ipexpand 1.0";
const char *argp_program_bug_address = "<w.b.devries@utwente.nl>";

static char doc[] = "ipexpand -- expand csv file with ip addresses to /24's\r\nExpects an ip address in the first column and a subnet in the second";
static char args_doc[] = "Input";

static struct argp_option options[] = {
    {"seperator", 's', "SEPERATOR", 0, "Use specified seperator for CSV input"},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output"},
    {0}
};

struct arguments {
    char *args[1];
    char *seperator;
    char *output_file;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch(key) {
        case 'o':
            arguments->output_file = arg;
            break;
        case 's':
            arguments->seperator = arg;
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

static int processline(char* line, char* seperator) {

    char* ip = strtok(line,seperator);
    int subnet = atoi(strtok(NULL,seperator));
    char* rest = strtok(NULL,""); 
    rest[strcspn(rest,"\r\n")] = 0;
     
    unsigned int intip = ip_to_uint(ip);
    char* ipconv = uint_to_ip(intip);

    if (subnet < 24) {
        unsigned int upperlimit = intip | 0xffffffff>>subnet;
        for (unsigned int i = intip; i<=upperlimit; i+=0x100) {
            printf("%s,24,%s\r\n", uint_to_ip(i),rest);
        }
    }

    free(ipconv);

    return 0;
}

int main(int argc, char **argv)
{
    struct arguments arguments;
    arguments.output_file = "-";
    arguments.seperator = ",;";

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
            processline(buffer,arguments.seperator);
        }
    }
    fclose(inputFile);
    return 0;
}
