#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int openFile(char *file, int regArray[], int codeLines[]);
char *getHexInstruct(int bitPattern, int regArray[]);
char *firstHalfInstruct(int bitPattern, int regArray[]);
char *secondHalfInstruct(int bitPattern, int regArray[]);
int16_t hexToDecimal(int bitPattern, char instructPos);
void calculateOutput(int codeLines[], int regArray[], int limit);
int hexConvert(char hex);
void printRegisters(int regArray[]);

#define NUMREGS 32
#define LINELIMIT 1000

int main(int argc, char *argv[]) {

    // Array storing the values in each of the registers
    int regArray[NUMREGS] = {0};
    // Array storing the bitpattern at each line
    int codeLines[LINELIMIT];
    
    if (argc == 2) {
        printf("Program\n");
        char *argFile = argv[1];
        int limit = openFile(argFile, regArray, codeLines);
        printf("Output\n");
        calculateOutput(codeLines, regArray, limit);
        printf("Registers After Execution\n");
        printRegisters(regArray);
    }
    else {
        printf("INVALID AMOUNT OF ARGUMENTS PASSED IN\n");
        exit(-1);
    }
}

// This Function is used to print the values in each array after program execution
void printRegisters(int regArray[]) {

    // i used as index value in while loop
    int i = 0;
    
    // Loops through value of each register and prints value if they are not zero
    while (i < NUMREGS) {
        if (regArray[i] != 0) {
            if (i < 10) {
                printf("$%d  = %d\n", i, regArray[i]);
            }
            else {
                printf("$%d = %d\n", i, regArray[i]);
            }
        }
        i++;
    }
}

// This function goes through each line of the code and executes them appropriately -> including ensuring it follows through loops
void calculateOutput(int codeLines[], int regArray[], int limit) {

    // Reinitialise register array to 0 to make sure no values are carried over from hex conversion to mips
    for (int j = 0; j < NUMREGS; j++) {
        regArray[j] = 0;
    }
    // Mask used to check for branch instructions
    int mask = 0xFC000000;

    // loop to go through each code line and execute
    int i = 0;
    while ( i < limit) {

        // Variables required are set before any logic occurs
        int bitPattern = codeLines[i];
        int s = hexToDecimal(bitPattern, 's');
        int t = hexToDecimal(bitPattern, 't');
        int16_t imm = hexToDecimal(bitPattern, 'i');

        getHexInstruct(bitPattern, regArray);
        // BNE logic
        if ((bitPattern & mask) == 0x14000000) {
            if ( regArray[s] != regArray[t]) {
                // i is set to imm - 1 since we increment at the end
                i += imm - 1;
            }
        }
        // BEQ logic
        else if ((bitPattern & mask) == 0x10000000) {
            if (regArray[s] == regArray[t]) {
                // i is set to imm - 1 since we increment at the end
                i += imm - 1;
            }
        }
        // Syscall logic
        else if (bitPattern == 0xc) {
            // Logic for exit
            if (regArray[2] == 10) {
                break;
            }
            // Logic for Integer
            if (regArray[2] == 1) {
                printf("%d", regArray[4]);
            }
            // Logic for character
            else if (regArray[2] == 11) {
                putchar(regArray[4]);
            }
            // Logic for unknown syscall
            else {
                printf("Unknown system call: %d\n", regArray[2]);
                break;
            }
        }
        i++;
    }
}

// This function Opens file and does the initial conversion from hex to mips
int openFile(char *file, int regArray[], int codeLines[]) {
    
    // Opens file for Reading
    FILE *hexFile = fopen(file, "r");
    // Array used to store each line from the file
    char line[9];

    // If error opening file
    if (hexFile == NULL) {
        printf("ERROR, COULD NOT OPEN THE FILE\n");
        exit(-1);
    }
    // Keeps track of Line number so it can be stored in codelines array
    int lineNum = 0;

    // While loop to scan until end of file
    while (fscanf(hexFile, "%s", line) != EOF) {

        // Variables used to store each line as a long int bit pattern
        char *endptr;
        int bitPattern = strtol(line, &endptr, 16);
        
        // Mask used to determine which print pattern to print with which variables
        int mask = 0xFC000000; 

        // Each element in codelines is set to the converted bitpattern
        codeLines[lineNum] = bitPattern;

        // The relevant instructions are derived from the passed in bitpattern and results are updated in regArray
        char *instruction = getHexInstruct(bitPattern, regArray);

        // Logic to determine which Print statement should be executed
        if (bitPattern == 0xc) {
            printf ("%3d: %s\n", lineNum, instruction);
        }
        else if ((bitPattern & mask) == 0x14000000 || (bitPattern & mask) == 0x10000000 ) {
            printf("%3d: %s $%d, $%d, %d\n", lineNum, instruction, hexToDecimal(bitPattern, 's'), hexToDecimal(bitPattern, 't'), hexToDecimal(bitPattern, 'i'));
        }
        else if ((bitPattern & mask) == 0x3c000000) {
            printf("%3d: %s $%d, %d\n", lineNum, instruction, hexToDecimal(bitPattern, 't'), hexToDecimal(bitPattern, 'i'));
        }
        else if ((bitPattern & mask) == 0x70000000) {
            printf("%3d: %s $%d, $%d, $%d\n", lineNum, instruction, hexToDecimal(bitPattern, 'd'), hexToDecimal(bitPattern, 's'), hexToDecimal(bitPattern, 't'));
        }
        else if (mask & bitPattern){
            printf("%3d: %s $%d, $%d, %d\n", lineNum, instruction, hexToDecimal(bitPattern, 't'), hexToDecimal(bitPattern, 's'), hexToDecimal(bitPattern, 'i'));
        }
        else {
            printf("%3d: %s $%d, $%d, $%d\n", lineNum, instruction, hexToDecimal(bitPattern, 'd'), hexToDecimal(bitPattern, 's'), hexToDecimal(bitPattern, 't'));
        }
        lineNum++;
    }

    // Close File
    fclose(hexFile);

    // returns the total amount of lines needed to be executed later on for calculate
    return lineNum;

}

// Function to determine which instruction should be executed depending on bitpattern
char *getHexInstruct(int bitPattern, int regArray[]) {

    // mask to determine if Instruction uses immediate or not
    int mask = 0xFC000000;

    if (mask & bitPattern) {
        return secondHalfInstruct(bitPattern, regArray);
    }
    else {
        return firstHalfInstruct(bitPattern, regArray);
    }
}

// Function used to execute logic for bit patterns not containing immediate
char *firstHalfInstruct(int bitPattern, int regArray[]) {

    // mask used to determine which exact mips instruction should be executed
    int mask = 0x7FF;

    // Logic used to determine which Instruction

    // add instruction
    if ((bitPattern & mask) == 0x20) {
        if (hexToDecimal(bitPattern, 'd') != 0)
            regArray[hexToDecimal(bitPattern, 'd')] = regArray[hexToDecimal(bitPattern, 's')] + regArray[hexToDecimal(bitPattern, 't')];
        return "add ";
    }
    // sub instruction
    else if ((bitPattern & mask) == 0x22) {
        if (hexToDecimal(bitPattern, 'd') != 0)
            regArray[hexToDecimal(bitPattern, 'd')] = regArray[hexToDecimal(bitPattern, 's')] - regArray[hexToDecimal(bitPattern, 't')];
        return "sub ";
    }
    // and instruction
    else if ((bitPattern & mask) == 0x24) {
        if (hexToDecimal(bitPattern, 'd') != 0)
            regArray[hexToDecimal(bitPattern, 'd')] = regArray[hexToDecimal(bitPattern, 's')] & regArray[hexToDecimal(bitPattern, 't')];
        return "and ";
    }
    // or instruction
    else if ((bitPattern & mask) == 0x25) {
        if (hexToDecimal(bitPattern, 'd') != 0)
            regArray[hexToDecimal(bitPattern, 'd')] = regArray[hexToDecimal(bitPattern, 's')] | regArray[hexToDecimal(bitPattern, 't')];
        return "or ";
    }
    // slt instruction
    else if ((bitPattern & mask) == 0x2a) {
        if (regArray[hexToDecimal(bitPattern, 's')] < regArray[hexToDecimal(bitPattern, 't')]) {
            if (hexToDecimal(bitPattern, 'd') != 0)
                regArray[hexToDecimal(bitPattern, 'd')] = 1;
        }
        else {
            if (hexToDecimal(bitPattern, 'd') != 0)
                regArray[hexToDecimal(bitPattern, 'd')] = 0;
        }
        return "slt ";
    }
    // syscall instruction
    else if ((bitPattern & 0xc) == 0xc) {
        return "syscall";
    }
    // Invalid instruction
    else {
        printf("invalid instruction code: %08X\n", bitPattern);
        exit(0);
    }
}

// Function used to execute logic for bit patterns containing immediate
char *secondHalfInstruct(int bitPattern, int regArray[]) {

    // mask used to determine which exact instruction should be executed
    int mask = 0xFC000000;

    //Logic used to determine which instruction

    // beq instruction
    if ((bitPattern & mask) == 0x10000000) {
        return "beq ";
    }
    // bne instruction
    else if ((bitPattern & mask) == 0x14000000) {
        return "bne ";
    }
    // addi instruction
    else if ((bitPattern & mask) == 0x20000000) {
        if (hexToDecimal(bitPattern, 't') != 0)
            regArray[hexToDecimal(bitPattern, 't')] = regArray[hexToDecimal(bitPattern, 's')] + (int16_t) hexToDecimal(bitPattern, 'i');
        return "addi";
    }
    // slti instruction
    else if ((bitPattern & mask) == 0x28000000) {
        if (regArray[hexToDecimal(bitPattern, 's')] < hexToDecimal(bitPattern, 'i')) {
            if (hexToDecimal(bitPattern, 't') != 0)
                regArray[hexToDecimal(bitPattern, 't')] = 1;
        }
        else {
            if (hexToDecimal(bitPattern, 't') != 0)
                regArray[hexToDecimal(bitPattern, 't')] = 0;
        }
        return "slti ";
    }
    // andi instruction
    else if ((bitPattern & mask) == 0x30000000) {
        if (hexToDecimal(bitPattern, 't') != 0)
            regArray[hexToDecimal(bitPattern, 't')] = regArray[hexToDecimal(bitPattern, 's')] & hexToDecimal(bitPattern, 'i');
        return "andi ";
    }
    // ori instruction
    else if ((bitPattern & mask) == 0x34000000) {
        if (hexToDecimal(bitPattern, 't') != 0)
            regArray[hexToDecimal(bitPattern, 't')] = regArray[hexToDecimal(bitPattern, 's')] | hexToDecimal(bitPattern, 'i');
        return "ori ";
    }
    // lui instruction
    else if ((bitPattern & mask) == 0x3c000000) {
        if (hexToDecimal(bitPattern, 't') != 0)
            regArray[hexToDecimal(bitPattern, 't')] = (int) hexToDecimal(bitPattern, 'i') << 16;
        return "lui ";
    }
    // mul instruction
    else if ((bitPattern & mask) == 0x70000000) {
        if (hexToDecimal(bitPattern, 't') != 0)
            regArray[hexToDecimal(bitPattern, 'd')] = regArray[hexToDecimal(bitPattern, 's')] * regArray[hexToDecimal(bitPattern, 't')];
        return "mul ";
    }
    // Invalid Instruction
    else {
        printf("invalid instruction code: %08X\n", bitPattern);
        exit(0);
    }
}

// Function used to convert hex bits to decimal notation
int16_t hexToDecimal(int bitPattern, char instructPos) {

    // logic used to extract exact hex bits depending on which value needs to be extracted
    if (instructPos == 's') {
        return (bitPattern >> 21) & 0x1F;
    }
    else if (instructPos == 't') {
        return (bitPattern >> 16) & 0x1F;
    }
    else if (instructPos == 'd') {
        return (bitPattern >> 11) & 0x1F;
    }
    else {
        return bitPattern & 0xFFFF;
    }
    
}
