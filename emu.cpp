#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <string>
#include <sstream>
using namespace std;

vector<char> memory;
uint32_t s  = 0;
uint32_t A  = 0;
uint32_t B  = 0;
uint32_t PC = 0;
uint32_t SP = 0;

// ---- flags set by CLI arguments ----
bool flag_trace  = false;   // -trace   → produce .trace   file (instruction trace)
bool flag_bfrafr = false;   // -bfrafr  → produce .bfrafr  file (before/after registers)
bool flag_memdump= false;   // -memdump → produce .memdump file (full memory dump)

ofstream working_file;   // trace output    (.trace)
ofstream result_file;    // memdump output  (.memdump)
ofstream bfrafr_file;    // before&after    (.bfrafr)

struct instruction
{
    const char *mnemonic;
    int op;
    bool operand;
    bool branch;
    bool pseudo;
};

static const instruction ins_type[] = {
    {"data",   255, true,  false, true},
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
    {"SET",    254, true,  false, true},
    {nullptr,  0,   false, false, false}
};

// write to console + trace file (only if -trace was given)
void write_to_files(const string &text)
{
    cout << text;
    if (flag_trace && working_file.is_open())
        working_file << text;
}

uint32_t get_word(uint32_t addr)
{
    if (addr * 4 + 3 >= (uint32_t)memory.size())
    {
        cout << "Memory read out of bounds at word address " << hex << addr << "\n";
        exit(1);
    }
    uint32_t val = 0;
    memcpy(&val, &memory[addr * 4], 4);

    if (addr < s && (val & 0xFF) == 0xFF)
    {
        int32_t signed_val = (int32_t)val >> 8;
        return (uint32_t)signed_val;
    }

    return val;
}

void set_word(uint32_t addr, uint32_t val)
{
    if (addr * 4 + 3 >= (uint32_t)memory.size())
    {
        cout << "Memory write out of bounds at word address " << hex << addr << "\n";
        exit(1);
    }
    memcpy(&memory[addr * 4], &val, 4);
}

string get_decoded(uint32_t instr)
{
    uint8_t opcode  = (instr >> 0) & 0xFF;
    int32_t operand = (instr >> 8) & 0x00FFFFFF;

    if (operand & 0x800000)
        operand |= 0xFF000000;

    for (int i = 0; ins_type[i].mnemonic != nullptr; i++)
    {
        if (ins_type[i].op == opcode)
        {
            string out = ins_type[i].mnemonic;
            if (ins_type[i].operand)
            {
                out += " ";
                out += to_string(operand);
            }
            return out;
        }
    }
    return "unknown(" + to_string((int)(instr & 0xFF)) + ")";
}

uint32_t instr_count = 0;

void load_program(const char *filename)
{
    ifstream file(filename, ios::binary | ios::ate);
    if (!file)
    {
        cout << "Error: cannot open file '" << filename << "'\n";
        exit(1);
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    memory.resize(size + 100000, 0);

    if (!file.read(memory.data(), size))
    {
        cout << "Error: failed to read file\n";
        exit(1);
    }

    SP = 0x270f;
    s  = (uint32_t)(size / 4);
}

void setup_output_files(const string &input_name)
{
    string base_name = input_name;
    size_t dot_pos   = input_name.rfind(".obj");
    if (dot_pos != string::npos)
        base_name = input_name.substr(0, dot_pos);

    // only open each file if its flag was given
    if (flag_trace)
    {
        string working_name = base_name + ".trace";
        working_file.open(working_name.c_str());
        if (!working_file)
        {
            cout << "Error: cannot open '" << working_name << "'\n";
            exit(1);
        }
        cout << "Trace file     : " << working_name << "\n";
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
    if (flag_memdump && result_file.is_open())
    {
        for (uint32_t addr = 0; addr < 1000000; addr++)
        {
            uint32_t val = 0;
            memcpy(&val, &memory[addr * 4], 4);
            result_file << right << hex << setw(8) << setfill('0') << addr
                        << "  "
                        << right << hex << setw(8) << setfill('0') << val
                        << "\n";
        }
    }

    if (flag_trace  && working_file.is_open()) working_file.close();
    if (flag_memdump && result_file.is_open()) result_file.close();
    if (flag_bfrafr && bfrafr_file.is_open())  bfrafr_file.close();
}

string build_row(uint32_t count, uint32_t instr, const string &as_str,
                 uint32_t instr_pc, uint32_t a, uint32_t b, uint32_t sp)
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
        << "       ------- before -------        "
        << "          --- instruction ---         "
        << "       ------- after  -------\n"
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
    string sep(oss.str().size() / 2, '-');
    sep += "\n";
    bfrafr_file << oss.str();
    bfrafr_file << sep;
}

void process_instr(uint32_t instr, uint32_t instr_pc)
{
    uint8_t opcode  = (instr >> 0) & 0xFF;
    int32_t operand = (instr >> 8) & 0x00FFFFFF;

    if (operand & 0x800000)
        operand |= 0xFF000000;

    uint32_t current_count = (opcode == 18 ? instr_count : ++instr_count);
    string   as_str        = get_decoded(instr);

    // snapshot registers BEFORE execution
    uint32_t bPC = instr_pc, bA = A, bB = B, bSP = SP;

    write_to_files(build_row(current_count, instr, as_str, instr_pc, A, B, SP));

    switch (opcode)
    {
    case 0:  B = A; A = (uint32_t)operand; break;                              // ldc
    case 1:  A = (uint32_t)((int32_t)A + operand); break;                      // adc
    case 2:  B = A; A = get_word((uint32_t)((int32_t)SP + operand)); break;    // ldl
    case 3:  set_word((uint32_t)((int32_t)SP + operand), A); A = B; break;     // stl
    case 4:  A = get_word((uint32_t)((int32_t)A + operand)); break;            // ldnl
    case 5:  set_word((uint32_t)((int32_t)A + operand), B); break;             // stnl
    case 6:  A = B + A; break;                                                 // add
    case 7:  A = B - A; break;                                                 // sub
    case 8:  A = B << A; break;                                                // shl
    case 9:  A = B >> A; break;                                                // shr
    case 10: SP = (uint32_t)((int32_t)SP + operand); break;                    // adj
    case 11: SP = A; A = B; break;                                             // a2sp
    case 12: B = A; A = SP; break;                                             // sp2a
    case 13: B = A; A = PC; PC = (uint32_t)((int32_t)PC + operand); break;    // call
    case 14: PC = A; A = B; break;                                             // return
    case 15: if (A == 0)          PC = (uint32_t)((int32_t)PC + operand); break; // brz
    case 16: if ((int32_t)A < 0)  PC = (uint32_t)((int32_t)PC + operand); break; // brlz
    case 17: PC = (uint32_t)((int32_t)PC + operand); break;                    // br
    case 255: A = (uint32_t)((int32_t)(instr) >> 8); break;                   // data

    case 18: // HALT
    {
        write_to_files("\nHALT reached.\n");
        if (flag_bfrafr && bfrafr_file.is_open())
            bfrafr_file << build_bfrafr_row(bPC, bA, bB, bSP, current_count, instr, as_str, PC, A, B, SP);
        write_result();
        exit(0);
    }

    default:
        cout << "Unknown opcode: " << hex << (int)opcode << "\n";
        exit(1);
    }

    // write before/after row for every instruction (except HALT, handled above)
    if (flag_bfrafr && bfrafr_file.is_open())
        bfrafr_file << build_bfrafr_row(bPC, bA, bB, bSP, current_count, instr, as_str, PC, A, B, SP);
}

void print_table_header()
{
    ostringstream oss;
    oss << setfill(' ')
        << right << setw(6)  << "#"      << " | "
        << left  << setw(8)  << "IN"     << " | "
        << left  << setw(14) << "AS"     << " | "
        << left  << setw(8)  << "PC"     << " | "
        << left  << setw(8)  << "A"      << " | "
        << left  << setw(8)  << "B"      << " | "
        << left  << setw(8)  << "SP"     << " |"
        << "\n";
    string sep(oss.str().size() - 1, '-');
    sep += "\n";
    write_to_files(oss.str());
    write_to_files(sep);
}

void start_execution()
{
    print_table_header();
    print_bfrafr_header();
    while (true)
    {
        if (PC * 4 + 3 >= (uint32_t)memory.size())
        {
            cout << "PC out of bounds: " << hex << PC << "\n";
            exit(1);
        }

        uint32_t instr_pc = PC;
        uint32_t instr    = get_word(PC);
        PC++;

        process_instr(instr, instr_pc);
    }
}

// ---- print usage instructions and exit ----
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
    // need at least: emulator <file.obj> <one flag>
    if (c < 3)
    {
        print_usage(args[0]);
        return 1;
    }

    string filename = args[1];

    // parse all flags from argument 2 onwards
    for (int i = 2; i < c; i++)
    {
        string arg = args[i];
        if      (arg == "-trace")   flag_trace  = true;
        else if (arg == "-bfrafr")  flag_bfrafr = true;
        else if (arg == "-memdump") flag_memdump= true;
        else
        {
            cout << "Error: unknown option '" << arg << "'\n\n";
            print_usage(args[0]);
            return 1;
        }
    }

    // must have at least one flag turned on
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