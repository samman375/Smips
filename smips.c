// COMP1521 Assignment 2: emulator for small subset of MIPS
// by Samuel Thorley (z5257239) 2020 Trimester 2


#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>

#define MAX_BYTES 32
#define MAX_LINES 1000
#define N_REGISTERS 32

void print_program(char *bytes, int index);
int print_output(int registers[], char *bytes, int PC);
char *byte_instr(char *bytes);
uint32_t first_bytes(char *input);
uint32_t last_bytes(char *input);
int s_bytes(char *input);
int t_bytes(char *input);
int d_bytes(char *input);
int16_t I_bytes(char *input);


int main(int argc, char *argv[]) {
    
    // Check filename provided
    // If more than one file provided, only first read
    if (argc < 2) {
        printf("Smips: please provide a valid filename\n");
        exit(1);
    }

    // Exits if incorrect file provided
    char *file = argv[1];
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        printf("No such file or directory: '%s'\n", file);
        exit(1);
    }

    // Instructions read from file into "string"
    // String has extra byte to store possible '\n'
    // All instructions stored in 2D array "inputs" indexed by "input_num"
    // '\n' removed before instruction added to "inputs"
    // Exits program if invalid instruction provided
    char string[MAX_BYTES + 1];
    char inputs[MAX_LINES][MAX_BYTES];
    int input_num = 0;
    while (fgets(string, MAX_BYTES, f) != NULL) {
        int i = 0;
        while (string[i] != '\0' && string[i] != '\n') {
            i++;
        }
        if (string[i] == '\n') {
            string[i] = '\0';
        }
        char *syscall = "c";
        if (byte_instr(string) == NULL && strcmp(string, syscall) != 0) {
            // String converted to uint32_t to match reference format
            char *c;
            long l = strtol(string, &c, 16);
            uint32_t code = (uint32_t)l;
            printf("%s:%d: invalid instruction code: %08X\n", file, input_num + 1, code);
            exit(1);
        }
        strcpy(inputs[input_num], string);
        input_num++;
    }

    fclose(f);

    // Values of 32 registers stored in array "registers"
    int32_t registers[N_REGISTERS] = {0};

    printf("Program\n");

    for (int line = 0; line < input_num; line++) {
        print_program(inputs[line], line);
    }

    printf("Output\n");

    int PC = 0;
    // Execution stops if PC == -1
    while (PC < input_num && PC != -1) {
        int temp = PC;
        PC = print_output(registers, inputs[PC], PC);
        if (PC == temp) {
            PC++;
        }
    }

    printf("Registers After Execution\n");

    // Different print statements used to match layout of autotests
    for (int j = 0; j < N_REGISTERS; j++) {
        if (registers[j] != 0 && j < 10) {
            printf("$%d  = %d\n", j, registers[j]);
        } else if (registers[j] != 0) {
            printf("$%d = %d\n", j, registers[j]);
        }
    }

    return 0;
}



// Given program number and string with bytes, prints instruction
void print_program(char *bytes, int index) {
    char *syscall = "c";
    if (strcmp(bytes, syscall) == 0) {
        printf ("%3d: syscall\n", index);
    } else {
        if (first_bytes(bytes) == 0x00000000 || (strcmp(byte_instr(bytes), "mul ") == 0)) {
            printf (
                "%3d: %s $%d, $%d, $%d\n", 
                index, byte_instr(bytes), d_bytes(bytes), s_bytes(bytes), t_bytes(bytes)
            );
        } else if ((strcmp(byte_instr(bytes), "beq ") == 0) || (strcmp(byte_instr(bytes), "bne ") == 0)) {
            printf (
                "%3d: %s $%d, $%d, %d\n", 
                index, byte_instr(bytes), s_bytes(bytes), t_bytes(bytes), I_bytes(bytes)
            );
        } else if (strcmp(byte_instr(bytes), "lui ") == 0) {
            printf (
                "%3d: %s $%d, %d\n", 
                index, byte_instr(bytes), t_bytes(bytes), I_bytes(bytes)
            );
        } else {
            printf (
                "%3d: %s $%d, $%d, %d\n", 
                index, byte_instr(bytes), t_bytes(bytes), s_bytes(bytes), I_bytes(bytes)
            );
        }
    }
}

// Changes register values given array of register values and string of bytes
// Prints output if instruction is syscall
// Returns program counter
// PC == -1 if error occurred
// $0 always set to 0
int print_output(int32_t registers[], char *bytes, int PC) {
    int s = s_bytes(bytes);
    int t = t_bytes(bytes);
    int d = d_bytes(bytes);
    int I = I_bytes(bytes);

    if (strcmp(bytes, "c") == 0) {
        // Syscall: Output required
        if (registers[2] == 1) {
            // Print integer
            printf("%d", registers[4]);
        } else if (registers[2] == 10) {
            // exit 0
            PC = -1;
        } else if (registers[2] == 11) {
            // Print character
            printf("%c", registers[4]);
        } else {
            // invalid syscall
            printf("Unknown system call: %d\n", registers[2]);
            PC = -1;
        }
    } else if (strcmp(byte_instr(bytes), "add ") == 0) {
        registers[d] = registers[s] + registers[t];
    } else if (strcmp(byte_instr(bytes), "sub ") == 0) {
        registers[d] = registers[s] - registers[t];
    } else if (strcmp(byte_instr(bytes), "and ") == 0) {
        registers[d] = registers[s] & registers[t];
    } else if (strcmp(byte_instr(bytes), "or  ") == 0) {
        registers[d] = registers[s] | registers[t];
    } else if (strcmp(byte_instr(bytes), "slt ") == 0) {
        if (registers[s] < registers[t]) {
            registers[d] = 1;
        } else {
            registers[d] = 0;
        }
    } else if (strcmp(byte_instr(bytes), "mul ") == 0) {
        registers[d] = registers[s] * registers[t];
    } else if (strcmp(byte_instr(bytes), "beq ") == 0) {
        if (registers[s] == registers[t]) {
            PC += I;
        }
    } else if (strcmp(byte_instr(bytes), "bne ") == 0) {
        if (registers[s] != registers[t]) {
            PC += I;
        }
    } else if (strcmp(byte_instr(bytes), "addi") == 0) {
        registers[t] = registers[s] + I;
    } else if (strcmp(byte_instr(bytes), "slti") == 0) {
        registers[t] = (registers[s] < I);
    } else if (strcmp(byte_instr(bytes), "andi") == 0) {
        registers[t] = registers[s] & I;
    } else if (strcmp(byte_instr(bytes), "ori ") == 0) {
        registers[t] = registers[s] | I;
    } else if (strcmp(byte_instr(bytes), "lui ") == 0) {
        registers[t] = I << 16;
    }
    registers[0] = 0;
    return PC;
}

// Given string with first 6 bytes only, returns corresponding instruction
// Returns NULL if unknown instruction or syscall provided
// Instructions 4 chars long + '\0' to match reference output
char *byte_instr(char *bytes) {
    char *instruction = NULL;

    if (first_bytes(bytes) == 0x00000000) {
        // look at last bytes
        if (last_bytes(bytes) == 0x00000020) {
            return instruction = "add ";
        } else if (last_bytes(bytes) == 0x00000022) {
            return instruction = "sub ";
        } else if (last_bytes(bytes) == 0x00000042) {
            return instruction = "and ";
        } else if (last_bytes(bytes) == 0x00000025) {
            return instruction = "or  ";
        } else if (last_bytes(bytes) == 0x0000002a) {
            return instruction = "slt ";
        }
    } else if (first_bytes(bytes) == 0x70000000) {
        return instruction = "mul ";
    } else if (first_bytes(bytes) == 0x10000000) {
        return instruction = "beq ";
    } else if (first_bytes(bytes) == 0x14000000) {
        return instruction = "bne ";
    } else if (first_bytes(bytes) == 0x20000000) {
        return instruction = "addi";
    } else if (first_bytes(bytes) == 0x28000000) {
        return instruction = "slti";
    } else if (first_bytes(bytes) == 0x30000000) {
        return instruction = "andi";
    } else if (first_bytes(bytes) == 0x34000000) {
        return instruction = "ori ";
    } else if (first_bytes(bytes) == 0x3c000000) {
        return instruction = "lui ";
    }
    return instruction;
}

// Given string, captures first 6 bytes
uint32_t first_bytes(char *input) {
    char* c;
    long l = strtol(input, &c, 16);
    uint32_t bytes = (uint32_t)l;
    bytes &= 0xfc000000;

    return bytes;
}

// Given string, captures last 6 bytes
uint32_t last_bytes(char *input) {
    char* c;
    long l = strtol(input, &c, 16);
    uint32_t bytes = (uint32_t)l;
    bytes &= 0x0000003f;

    return bytes;
}

// Given string, returns number corresponding to $s register
int s_bytes(char *input) {
    char* c;
    long l = strtol(input, &c, 16);
    int s = (int)l;
    s &= 0x03e00000;
    s >>= 21;

    return s;
}

// Given string, returns number corresponding to $t register
int t_bytes(char *input) {
    char* c;
    long l = strtol(input, &c, 16);
    int t = (int)l;
    t &= 0x001f0000;
    t >>= 16;

    return t;
}

// Given string, returns number corresponding to $d register
int d_bytes(char *input) {
    char* c;
    long l = strtol(input, &c, 16);
    int d = (int)l;
    d &= 0x0000f800;
    d >>= 11;

    return d;
}

// Given string, returns bits corresponding to I
int16_t I_bytes(char *input) {
    char* c;
    long l = strtol(input, &c, 16);
    l <<= 16;
    l >>= 16;
    int16_t I = (int16_t)l;

    return I;
}