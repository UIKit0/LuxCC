#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include "parser.h"
#include "ic.h"
#include "vm_cgen/vm_cgen.h"
#include "x86_cgen/x86_cgen.h"

/*
Driver options
    h -> print driver supported command-line options (with -v print help of called programs by the driver too)
    v -> verbose
    E -> preprocess only
    S -> compile only
    c -> compile and assemble only
passed to cc
    I
    i
    dump-tokens
    analyze
    quiet
    show-stats
    boring
    target
passed to ld
    l
    L
    entry
passed to all
    o
*/


unsigned warning_count, error_count;
int disable_warnings;
int colored_diagnostics = 1;

unsigned stat_number_of_pre_tokens;
unsigned stat_number_of_c_tokens;
unsigned stat_number_of_ast_nodes;

static void usage(FILE *f, char *program_name)
{
    fprintf(f, "USAGE: %s [OPTIONS...] <file>\n", program_name);
}

static void print_options(void)
{
    printf(
    "\nOPTIONS:\n"
    "  Short option name     Long option name        Description\n"
    "  -E,                   --preprocess            Preprocess only\n"
    "  -d,                   --dump-token            Show tokenized file\n"
    "  -a,                   --analyze               Perform static analysis only\n"
    "  -q,                   --quiet                 Disable all warnings\n"
    "  -o<file>,             --output-file<file>     Write output to <file>\n"
    "  -s,                   --show-stats            Show compilation stats\n"
    "  -b,                   --boring                Print uncolored diagnostics\n"
    "  -t<target>,           --target<mach>          Generate target code for machine <mach>\n"
    "  -I<dir>,              --angle-include<dir>    Add <dir> to the list of directories searched for #include <...>\n"
    "  -i<dir>,              --quote-include<dir>    Add <dir> to the list of directories searched for #include \"...\"\n"
    "  -h,                   --help                  Print this help\n"
    );
}

#define TARGET_X86      1
#define TARGET_VM       2
#define TARGET_DEFAULT  TARGET_X86

int main(int argc, char *argv[])
{
    static PreTokenNode *pre;
    static TokenNode *tok;
    // static ExternDecl *tree;

    /*
     * Handle command line options.
     */
    int option_index, c;
    unsigned option_flags;
    char *output_file_arg = NULL;
    int target_machine_arg = TARGET_DEFAULT;
    PreTokenNode dummy_node = { PRE_TOK_NL };
    enum {
        OPT_PREPROCESS_ONLY = 0x1,
        OPT_DUMP_TOKENS     = 0x2,
        OPT_ANALYZE         = 0x4,
        OPT_SHOW_STATS      = 0x8
    };

    struct option compiler_options[] = {
        { "preprocess",      no_argument,        NULL, 'E' },
        { "dump-tokens",     no_argument,        NULL, 'd' },
        { "analyze",         no_argument,        NULL, 'a' },
        { "quiet",           no_argument,        NULL, 'q' },
        { "output-file",     required_argument,  NULL, 'o' },
        { "help",            no_argument,        NULL, 'h' },
        { "show-stats",      no_argument,        NULL, 's' },
        { "boring",          no_argument,        NULL, 'b' },
        { "target",          required_argument,  NULL, 't' },
        { "angle-include",   required_argument,  NULL, 'I' },
        { "quote-include",   required_argument,  NULL, 'i' },
        { NULL,              0,                  NULL,  0 }
    };

    option_flags = 0;
    for (;;) {
        option_index = 0;

        c = getopt_long(argc, argv, "Edaqo:hsbt:I:i:", compiler_options, &option_index);
        if (c == -1)
            break; /* no more options */

        switch (c) {
            case 'E':
                option_flags |= OPT_PREPROCESS_ONLY;
                break;
            case 'd':
                option_flags |= OPT_DUMP_TOKENS;
                break;
            case 'a':
                option_flags |= OPT_ANALYZE;
                break;
            case 's':
                option_flags |= OPT_SHOW_STATS;
                break;
            case 'q':
                disable_warnings = 1;
                break;
            case 'b':
                colored_diagnostics = 0;
                break;
            case 'o':
                output_file_arg = optarg;
                break;
            case 't':
                if (strcmp(optarg, "x86") == 0)
                    target_machine_arg = TARGET_X86;
                else if (strcmp(optarg, "vm") == 0)
                    target_machine_arg = TARGET_VM;
                break;
            case 'I':
                add_angle_dir(optarg);
                break;
            case 'i':
                add_quote_dir(optarg);
                break;
            case 'h':
                usage(stdout, argv[0]);
                print_options();
                exit(EXIT_SUCCESS);
            case '?':
                /* by default opterr is true, getopt_long already printed a diagnostic */
                // break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        /* the input file is missing */
        usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }

    /*
     * Here begins the processing of the file.
     */
    if (target_machine_arg == TARGET_X86)
        install_macro(SIMPLE_MACRO, "__x86_32__", &dummy_node, NULL);
    else if (target_machine_arg == TARGET_VM)
        install_macro(SIMPLE_MACRO, "__LuxVM__", &dummy_node, NULL);

    pre = preprocess(argv[optind]);
    if (option_flags & OPT_PREPROCESS_ONLY) {
        PreTokenNode *p;

        for (p = pre; p != NULL; p = p->next) {
            if (!p->deleted || p->token==PRE_TOK_NL)
                printf("%s ", p->lexeme);
        }
        // exit(EXIT_SUCCESS);
        goto done;
    }

    tok = lexer(pre);
    if (option_flags & OPT_DUMP_TOKENS) {
        TokenNode *p;

        for (p = tok; p != NULL; p = p->next) {
            printf("%s:%d:%-3d =>   token: %-15s lexeme: `%s'\n", p->src_file, p->src_line,
            p->src_column, token_table[p->token*2], p->lexeme);
        }
        exit(EXIT_SUCCESS);
    }

    // tree = parser(tok);
    parser(tok);

    if (option_flags & OPT_ANALYZE)
        exit(EXIT_SUCCESS);

    if (error_count == 0) {
        FILE *fp;

        fp = (output_file_arg == NULL) ? stdout : fopen(output_file_arg, "wb");
        if (target_machine_arg == TARGET_X86)
            x86_cgen(fp);
        else if (target_machine_arg == TARGET_VM)
            vm_cgen(fp);
        if (output_file_arg != NULL)
            fclose(fp);
    } else {
        return 1;
    }
done:
    if (option_flags & OPT_SHOW_STATS) {
        printf("\n=> '%u' preprocessing tokens were created (aprox)\n", stat_number_of_pre_tokens);
        printf("=> '%u' C tokens were created (aprox)\n", stat_number_of_c_tokens);
        printf("=> '%u' AST nodes were created (aprox)\n", stat_number_of_ast_nodes);
    }
    // printf("%d warning and %d error generated\n", warning_count, error_count);

	return 0;
}