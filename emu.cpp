#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <string>
using namespace std;

vector<char> memory;
uint32_t s  = 0;
uint32_t A  = 0;
uint32_t B  = 0;
uint32_t PC = 0;
uint32_t SP = 0;

ofstream working_file; 
ofstream result_file;   

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

uint32_t get_word(uint32_t addr)
{
    if (addr * 4 + 3 >= (uint32_t)memory.size())
    {
        cerr << "Memory read out of bounds at word address " << hex << addr << "\n";
        exit(1);
    }
    uint32_t val = 0;
    memcpy(&val, &memory[addr * 4], 4);

    if ((val & 0xFF) == 0xFF)
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
        cerr << "Memory write out of bounds at word address " << hex << addr << "\n";
        exit(1);
    }
    memcpy(&memory[addr * 4], &val, 4);
}

void print_decoded(uint32_t instr)
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
            out += "\n";
            cout        << out;
            working_file << out;
            return;
        }
    }
    cerr << "Unknown opcode: " << hex << (int)opcode << "\n";
}

uint32_t instr_count = 0;

void load_program(const char *filename)
{
    ifstream file(filename, ios::binary | ios::ate);
    if (!file)
    {
        cerr << "Error: cannot open file '" << filename << "'\n";
        exit(1);
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    memory.resize(size + 100000, 0);

    if (!file.read(memory.data(), size))
    {
        cerr << "Error: failed to read file\n";
        exit(1);
    }

    SP = (uint32_t)(size / 4);
    s  = (uint32_t)(size / 4);
}

void setup_output_files(const string &input_name)
{
    string base_name = input_name;
    size_t dot_pos   = input_name.rfind(".obj");
    if (dot_pos != string::npos)
        base_name = input_name.substr(0, dot_pos);

    string working_name = base_name + ".working";
    string result_name  = base_name + ".result";

    working_file.open(working_name.c_str());
    if (!working_file)
    {
        cerr << "Error: cannot open '" << working_name << "'\n";
        exit(1);
    }

    result_file.open(result_name.c_str());
    if (!result_file)
    {
        cerr << "Error: cannot open '" << result_name << "'\n";
        exit(1);
    }
}

void write_result()
{
    for (uint32_t addr = 0; addr < 10000; addr++)
    {
        uint32_t val = 0;
        memcpy(&val, &memory[addr * 4], 4);
        result_file << right << hex << setw(8) << setfill('0') << addr
                    << "  "
                    << right << hex << setw(8) << setfill('0') << val
                    << "\n";
    }
    working_file.close();
    result_file.close();
}

void process_instr(uint32_t instr, uint32_t instr_pc)
{
    uint8_t opcode  = (instr >> 0) & 0xFF;
    int32_t operand = (instr >> 8) & 0x00FFFFFF;

    if (operand & 0x800000)
        operand |= 0xFF000000;

    ostringstream oss;
    oss << "PC: " << right << hex << setw(8) << setfill('0') << instr_pc  << "    "
        << "IN: " << right << hex << setw(8) << setfill('0') << instr     << "    "
        << "SP: " << right << hex << setw(8) << setfill('0') << SP        << "    "
        << "A: "  << right << hex << setw(8) << setfill('0') << A         << "    "
        << "B: "  << right << hex << setw(8) << setfill('0') << B         << "    "
        << "#"    << dec   << (opcode ==18 ? instr_count : ++instr_count) << "    ";

    cout         << oss.str();
    working_file << oss.str();

    print_decoded(instr);

    switch (opcode)
    {
    case 0: // ldc
        B = A;
        A = (uint32_t)operand;
        break;

    case 1: // adc
        A = (uint32_t)((int32_t)A + operand);
        break;

    case 2: // ldl
        B = A;
        A = get_word((uint32_t)((int32_t)SP + operand));
        break;

    case 3: // stl
        set_word((uint32_t)((int32_t)SP + operand), A);
        A = B;
        break;

    case 4: // ldnl
        A = get_word((uint32_t)((int32_t)A + operand));
        break;

    case 5: // stnl
        set_word((uint32_t)((int32_t)A + operand), B);
        break;

    case 6: // add
        A = B + A;
        break;

    case 7: // sub
        A = B - A;
        break;

    case 8: // shl
        A = B << A;
        break;

    case 9: // shr
        A = B >> A;
        break;

    case 10: // adj
        SP = (uint32_t)((int32_t)SP + operand);
        break;

    case 11: // a2sp
        SP = A;
        A  = B;
        break;

    case 12: // sp2a
        B = A;
        A = SP;
        break;

    case 13: // call
        B  = A;
        A  = PC;
        PC = (uint32_t)((int32_t)PC + operand);
        break;

    case 14: // return
        PC = A;
        A  = B;
        break;

    case 15: // brz
        if (A == 0)
            PC = (uint32_t)((int32_t)PC + operand);
        break;

    case 16: // brlz
        if ((int32_t)A < 0)
            PC = (uint32_t)((int32_t)PC + operand);
        break;

    case 17: // br
        PC = (uint32_t)((int32_t)PC + operand);
        break;

    case 18: // HALT
    {
        string halt_msg = "\nHALT reached.\n";
        cout         << halt_msg;
        working_file << halt_msg;
        write_result();
        exit(0);
        break;
    }

    case 255: // data
        A = (uint32_t)((int32_t)(instr) >> 8);
        break;

    default:
        cerr << "Unknown opcode: " << hex << (int)opcode << "\n";
        exit(1);
    }
}

void start_execution()
{
    while (true)
    {
        if (PC * 4 + 3 >= (uint32_t)memory.size())
        {
            cerr << "PC out of bounds: " << hex << PC << "\n";
            exit(1);
        }

        uint32_t instr_pc = PC;
        uint32_t instr    = get_word(PC);
        PC++;

        process_instr(instr, instr_pc);
    }
}

int main(int c, char *args[])
{
    if (c < 2)
    {
        cerr << "Usage: " << args[0] << " <filename>\n";
        return 1;
    }

    setup_output_files(args[1]);

    load_program(args[1]);

    start_execution();

    return 0;
}