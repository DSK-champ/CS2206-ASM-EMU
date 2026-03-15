/* 
    Name      : Darla Sravan Kumar
    Roll      : 2401CS45
    Course    : Computer Architecture CS2206
    Project   : Two Pass Assembler & Emulator for SIMPLEX Instruction Set
    File Name : emu.cpp -- emulator
    Language  : C++
    Compile   : g++ emu.cpp -o emu
    Useage    : ./emu <filename>.obj  -trace -bfrafr -memdump

    All work presented here is independently developed by the author (Sravan)
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <string>
#include <sstream>
using namespace std;

// Emulated machine memory (byte-addressable, word-aligned access)
vector<char> memory;

// CPU registers
uint32_t s  = 0;   // number of words in the loaded program (code segment size)
uint32_t A  = 0;   // accumulator register
uint32_t B  = 0;   // secondary register (holds previous A in stack ops)
uint32_t PC = 0;   // program counter (word address of next instruction)
uint32_t SP = 0;   // stack pointer   (word address)

bool flag_trace   = false;  // -trace   → write per-instruction trace to .trace file
bool flag_bfrafr  = false;  // -bfrafr  → write before/after register state to .bfrafr file
bool flag_memdump = false;  // -memdump → dump full memory at HALT to .memdump file

// Output file streams 
ofstream trace_file;  
ofstream result_file;  
ofstream bfrafr_file;   

struct instruction
{
    const char *mnemonic; 
    int op;              
    bool operand;   // is it supposed to have an operand?
    bool branch;    // is it a branch instruction?
    bool pseudo;    // is it a pseudo instruction? 
};

// Complete ISA table 
static const instruction ins_type[] = {
    {"data",   255, true,  false, true },
    {"ldc",    0,   true,  false, false},
    {"adc",    1,   true,  false, false},
    {"ldl",    2,   true,  false, false},
    {"stl",    3,   true,  false, false},
    {"ldnl",   4,   true,  false, false},
    {"stnl",   5,   true,  false, false},
    {"add",    6,   false, false, false},
    {"sub",    7,   false, false, false},
    {"shl",    8,   false, false, false},
    {"shr",    9,   false, false, false},
    {"adj",    10,  true,  false, false},
    {"a2sp",   11,  false, false, false},
    {"sp2a",   12,  false, false, false},
    {"call",   13,  true,  true,  false},
    {"return", 14,  false, false, false},
    {"brz",    15,  true,  true,  false},
    {"brlz",   16,  true,  true,  false},
    {"br",     17,  true,  true,  false},
    {"HALT",   18,  false, false, false},
    {"SET",    254, true,  false, true },
    {nullptr,  0,   false, false, false}  
};

void write_to_files(const string &text)
{
    cout << text;
    if (flag_trace && trace_file.is_open())
        trace_file << text;
}

// Words inside the code segment (addr < s) whose low byte is 0xFF are treated
// as sign-extended data words (the opcode byte is used as a sign extension).
uint32_t get_word(uint32_t addr)
{
    // Guard against reads beyond the allocated memory buffer
    if (addr * 4 + 3 >= (uint32_t)memory.size())
    {
        cout << "Memory read out of bounds at word address " << hex << addr << "\n";
        exit(1);
    }

    uint32_t val = 0;
    memcpy(&val, &memory[addr * 4], 4);  // copy 4 bytes into val (little-endian)

    // If this address is in the code segment and looks like a data word,
    // sign-extend the upper 24 bits 
    if (addr < s && (val & 0xFF) == 0xFF)
    {
        int32_t signed_val = (int32_t)val >> 8;
        return (uint32_t)signed_val;
    }

    return val;
}


void set_word(uint32_t addr, uint32_t val)
{
    // Guard against writes beyond the allocated memory buffer
    if (addr * 4 + 3 >= (uint32_t)memory.size())
    {
        cout << "Memory write out of bounds at word address " << hex << addr << "\n";
        exit(1);
    }
    memcpy(&memory[addr * 4], &val, 4);  // copy 4 bytes from val into memory
}

// Disassemble a raw 32-bit instruction word to a human-readable string 
string get_decoded(uint32_t instr)
{
    uint8_t opcode  = (instr >> 0) & 0xFF;
    int32_t operand = (instr >> 8) & 0x00FFFFFF;

    // Sign-extend the 24-bit operand to a full 32-bit signed integer
    if (operand & 0x800000)
        operand |= 0xFF000000;

    // search the ISA table looking for a matching opcode
    for (int i = 0; ins_type[i].mnemonic != nullptr; i++)
    {
        if (ins_type[i].op == opcode)
        {
            string out = ins_type[i].mnemonic;   // out is the output assembly code string
            if (ins_type[i].operand)    // append operand only if the instruction uses one
            {
                out += " ";
                out += to_string(operand);
            }
            return out;
        }
    }
    // Opcode not found in the table
    return "unknown(" + to_string((int)(instr & 0xFF)) + ")";
}

uint32_t instr_count = 0;

void load_program(const char *filename)
{
    ifstream file(filename, ios::binary | ios::ate);  // open at end to measure size
    if (!file)
    {
        cout << "Error: cannot open file '" << filename << "'\n";
        exit(1);
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    // Allocate memory: program bytes + extra space for runtime stack
    memory.resize(size + 100000, 0);

    if (!file.read(memory.data(), size))
    {
        cout << "Error: failed to read file\n";
        exit(1);
    }

    SP = 0x270f;              // initialise stack pointer to a safe default address
    s  = (uint32_t)(size / 4); // record how many 32-bit words the program occupies
}

void setup_output_files(const string &input_name)
{
    // Strip ".obj" extension to derive the base name for output files
    string base_name = input_name;
    size_t dot_pos   = input_name.rfind(".obj");
    if (dot_pos != string::npos)
        base_name = input_name.substr(0, dot_pos);

    if (flag_trace)
    {
        string trace_name = base_name + ".trace";
        trace_file.open(trace_name.c_str());
        if (!trace_file)
        {
            cout << "Error: cannot open '" << trace_name << "'\n";
            exit(1);
        }
        cout << "Trace file     : " << trace_name << "\n";
    }

    if (flag_memdump)
    {
        string result_name = base_name + ".memdump";
        result_file.open(result_name.c_str());
        if (!result_file)
        {
            cout << "Error: cannot open '" << result_name << "'\n";
            exit(1);
        }
        cout << "Memory dump    : " << result_name << "\n";
    }

    if (flag_bfrafr)
    {
        string bfrafr_name = base_name + ".bfrafr";
        bfrafr_file.open(bfrafr_name.c_str());
        if (!bfrafr_file)
        {
            cout << "Error: cannot open '" << bfrafr_name << "'\n";
            exit(1);
        }
        cout << "Before/After   : " << bfrafr_name << "\n";
    }

    cout << "\n";
}

void write_result()
{
    // Dump the first 1,000,000 words of memory in hex, one word per line
    if (flag_memdump && result_file.is_open())
    {
        for (uint32_t addr = 0; addr < 1000000; addr++)
        {
            uint32_t val = 0;
            memcpy(&val, &memory[addr * 4], 4);
            result_file << right << hex << setw(8) << setfill('0') << addr << "  "
                        << right << hex << setw(8) << setfill('0') << val  << "\n";
        }
    }

    // Close every output file that was opened
    if (flag_trace   && trace_file.is_open()) trace_file.close();
    if (flag_memdump && result_file.is_open())  result_file.close();
    if (flag_bfrafr  && bfrafr_file.is_open())  bfrafr_file.close();
}

string build_row(uint32_t count, uint32_t instr, const string &as_str, uint32_t instr_pc, uint32_t a, uint32_t b, uint32_t sp)
{
    ostringstream oss;
    oss << setfill(' ')
        << right << setw(6) << dec << count                    << " | "
        << right << hex << setw(8) << setfill('0') << instr    << " | "
        << setfill(' ') << left    << setw(14)     << as_str   << " | "
        << right << hex << setw(8) << setfill('0') << instr_pc << " | "
        << right << hex << setw(8) << setfill('0') << a        << " | "
        << right << hex << setw(8) << setfill('0') << b        << " | "
        << right << hex << setw(8) << setfill('0') << sp       << " |"
        << "\n";
    return oss.str();
}

// Columns: [before PC/A/B/SP] | [instruction count/raw/mnemonic] | [after PC/A/B/SP]
string build_bfrafr_row(uint32_t bPC, uint32_t bA, uint32_t bB, uint32_t bSP,
                        uint32_t count, uint32_t instr, const string &as_str,
                        uint32_t aPC, uint32_t aA, uint32_t aB, uint32_t aSP)
{
    ostringstream oss;
    oss << right << hex << setw(8) << setfill('0') << bPC   << " | "
        << right << hex << setw(8) << setfill('0') << bA    << " | "
        << right << hex << setw(8) << setfill('0') << bB    << " | "
        << right << hex << setw(8) << setfill('0') << bSP   << " || "
        << right << setw(6) << dec << setfill(' ') << count << " | "
        << right << hex << setw(8) << setfill('0') << instr << " | "
        << setfill(' ') << left    << setw(14)     << as_str<< " || "
        << right << hex << setw(8) << setfill('0') << aPC   << " | "
        << right << hex << setw(8) << setfill('0') << aA    << " | "
        << right << hex << setw(8) << setfill('0') << aB    << " | "
        << right << hex << setw(8) << setfill('0') << aSP   << " |"
        << "\n";
    return oss.str();
}


void print_bfrafr_header()
{
    if (!flag_bfrafr || !bfrafr_file.is_open()) return;

    ostringstream oss;
    oss << setfill(' ')
        << "           ------- before -------            "
        << "            --- instruction ---         "
        << "           ------- after  -------\n"
        << left  << setw(8)  << "PC"    << " | "
        << left  << setw(8)  << "A"     << " | "
        << left  << setw(8)  << "B"     << " | "
        << left  << setw(8)  << "SP"    << " || "
        << right << setw(6)  << "N"     << " | "
        << left  << setw(8)  << "IN"    << " | "
        << left  << setw(14) << "AS"    << " || "
        << left  << setw(8)  << "PC"    << " | "
        << left  << setw(8)  << "A"     << " | "
        << left  << setw(8)  << "B"     << " | "
        << left  << setw(8)  << "SP"    << " |"
        << "\n";
    // Separator line whose width matches the header
    string sep(oss.str().size() / 2, '-');
    sep += "\n";
    bfrafr_file << oss.str();
    bfrafr_file << sep;
}

void process_instr(uint32_t instr, uint32_t instr_pc)
{
    uint8_t opcode  = (instr >> 0) & 0xFF;
    int32_t operand = (instr >> 8) & 0x00FFFFFF;

    // Sign-extend 24-bit operand → 32-bit signed integer
    if (operand & 0x800000)
        operand |= 0xFF000000;

    // For HALT, keep the current count; for all other instructions, increment it
    uint32_t current_count = (opcode == 18 ? instr_count : ++instr_count);
    string   as_str        = get_decoded(instr); // assembly string

   
    uint32_t bPC = instr_pc, bA = A, bB = B, bSP = SP;  // (needed for .bfrafr output)

    // building the row for trace file and terminal output
    write_to_files(build_row(current_count, instr, as_str, instr_pc, A, B, SP));

    // Execute the instruction 
    switch (opcode)
    {
    case 0:  B = A;      A = (uint32_t)operand;                              break; // ldc  
    case 1:  A = (uint32_t)((int32_t)A + operand);                           break; // adc 
    case 2:  B = A;      A = get_word((uint32_t)((int32_t)SP + operand));    break; // ldl 
    case 3:  set_word((uint32_t)((int32_t)SP + operand), A);       A = B;    break; // stl 
    case 4:  A = get_word((uint32_t)((int32_t)A + operand));                 break; // ldnl 
    case 5:  set_word((uint32_t)((int32_t)A + operand), B);                  break; // stnl
    case 6:  A = B  + A;                                                     break; // add  
    case 7:  A = B  - A;                                                     break; // sub
    case 8:  A = B << A;                                                     break; // shl 
    case 9:  A = B >> A;                                                     break; // shr  
    case 10: SP = (uint32_t)((int32_t)SP + operand);                         break; // adj  
    case 11: SP = A;   A = B;                                                break; // a2sp 
    case 12: B = A;    A = SP;                                               break; // sp2a 
    case 13: B = A;    A = PC;    PC = (uint32_t)((int32_t)PC + operand);    break; // call 
    case 14: PC = A;              A = B;                                     break; // return 
    case 15: if (A == 0)          PC = (uint32_t)((int32_t)PC + operand);    break; // brz  
    case 16: if ((int32_t)A < 0)  PC = (uint32_t)((int32_t)PC + operand);    break; // brlz 
    case 17: PC = (uint32_t)((int32_t)PC + operand);                         break; // br   
    case 255: A = (uint32_t)((int32_t)(instr) >> 8);                         break; // data 
    case 254: break;
    case 18: // HALT
    {
        write_to_files("\nHALT reached.\n");
        // Record the final before/after entry (registers don't change on HALT)
        if (flag_bfrafr && bfrafr_file.is_open())
            bfrafr_file << build_bfrafr_row(bPC, bA, bB, bSP, current_count, instr, as_str, PC, A, B, SP);
        write_result();
        exit(0);
    }

    default:
        cout << "Unknown opcode: " << hex << (int)opcode << "\n";
        exit(1);
    }

    // Record the before/after register state for every non-HALT instruction
    if (flag_bfrafr && bfrafr_file.is_open())
        bfrafr_file << build_bfrafr_row(bPC, bA, bB, bSP, current_count, instr, as_str, PC, A, B, SP);
}

// Print the column header + separator for the instruction trace table
void print_table_header()
{
    ostringstream oss;
    oss << setfill(' ')
        << right << setw(6)  << "#"  << " | "
        << left  << setw(8)  << "IN" << " | "
        << left  << setw(14) << "AS" << " | "
        << left  << setw(8)  << "PC" << " | "
        << left  << setw(8)  << "A"  << " | "
        << left  << setw(8)  << "B"  << " | "
        << left  << setw(8)  << "SP" << " |"
        << "\n";
    string sep(oss.str().size() - 1, '-');
    sep += "\n";
    write_to_files(oss.str());
    write_to_files(sep);
}

// Main fetch-decode-execute loop
// Runs until HALT is encountered (process_instr calls exit on HALT).
void start_execution()
{
    print_table_header();
    print_bfrafr_header();

    while (true)
    {
        // Ensure PC hasn't gone past the end of allocated memory
        if (PC * 4 + 3 >= (uint32_t)memory.size())
        {
            cout << "PC out of bounds: " << hex << PC << "\n";
            exit(1);
        }

        uint32_t instr_pc = PC;             // remember fetch address before advancing PC
        uint32_t instr    = get_word(PC);   // fetch instruction word
        PC++;                               // advance PC (PC-relative branches add to this)

        process_instr(instr, instr_pc);     // decode, execute, trace
    }
}

void print_usage(const char *prog)
{
    cout << "Usage: " << prog << " <file.obj> [options]\n\n"
         << "Options (at least one required):\n"
         << "  -trace    Instruction trace log       → <file>.trace\n"
         << "  -bfrafr   Register state before/after → <file>.bfrafr\n"
         << "  -memdump  Full memory dump at HALT    → <file>.memdump\n\n"
         << "Examples:\n"
         << "  " << prog << " program.obj -trace\n"
         << "  " << prog << " program.obj -memdump\n"
         << "  " << prog << " program.obj -trace -bfrafr -memdump\n";
}

int main(int c, char *args[])
{
    if (c < 3)
    {
        print_usage(args[0]);
        return 1;
    }

    string filename = args[1];

    for (int i = 2; i < c; i++)
    {
        string arg = args[i];
        if      (arg == "-trace")   flag_trace   = true;
        else if (arg == "-bfrafr")  flag_bfrafr  = true;
        else if (arg == "-memdump") flag_memdump = true;
        else
        {
            cout << "Error: unknown option '" << arg << "'\n\n";
            print_usage(args[0]);
            return 1;
        }
    }

    // At least one output mode must be selected
    if (!flag_trace && !flag_bfrafr && !flag_memdump)
    {
        cout << "Error: no output option specified.\n\n";
        print_usage(args[0]);
        return 1;
    }

    setup_output_files(filename);   
    load_program(filename.c_str()); 
    start_execution();             

    return 0;
}