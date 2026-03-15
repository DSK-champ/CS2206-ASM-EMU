/* 
    Name      : Darla Sravan Kumar
    Roll      : 2401CS45
    Course    : Computer Architecture CS2206
    Project   : Two Pass Assembler & Emulator for SIMPLEX Instruction Set
    File Name : asm.cpp -- assembler
    Language  : C++
    Compile   : g++ asm.cpp -o asm
    Useage    : ./asm <filename>.asm
    
    All work presented here is independently developed by the author (Sravan)
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <bits/stdc++.h>
using namespace std;


struct instruction
{
    // [----------24---------][--8--]
    //   operand / immediate    op

    const char *mnemonic;
    int op;
    bool operand;   // is it supposed to have an operand?
    bool branch;    // is it a branch instruction?
    bool pseudo;    // is it a pseudo instruction?
};


// its like a dictionary for this language
static const instruction ins_type[] = {
    {"data",  255, true,  false,  true},
    {"ldc",     0, true,  false, false},
    {"adc",     1, true,  false, false},
    {"ldl",     2, true,  false, false},
    {"stl",     3, true,  false, false},
    {"ldnl",    4, true,  false, false},
    {"stnl",    5, true,  false, false},
    {"add",     6, false, false, false},
    {"sub",     7, false, false, false},
    {"shl",     8, false, false, false},
    {"shr",     9, false, false, false},
    {"adj",    10, true,  false, false},
    {"a2sp",   11, false, false, false},
    {"sp2a",   12, false, false, false},
    {"call",   13, false,  true, false},
    {"return", 14, false, false, false},
    {"brz",    15, false,  true, false},
    {"brlz",   16, false,  true, false},
    {"br",     17, false,  true, false},
    {"HALT",   18, false, false, false},
    {"SET",   254, true,  false,  true},
    {nullptr,   0, false, false, false}
};

// this is a packet of data that holds all information about each line in code
struct meta_data
{
    string address;       // PC address
    string machine_code;  // translated Machine Code
    string assembly_code; // original  Assembly Code
    int label;            // is it a Label? 0/1
    int error;            // does it have errors? if negative then there is error
};

unordered_map<string, string> Label_Table;

string hex_counter(string s)
{
    // start from the rightmost digit and work backwards
    for (int i = s.size() - 1; i >= 0; i--)
    {
        if (s[i] == 'F')
            s[i] = '0';
        else
        {
            if (s[i] == '9')
                s[i] = 'A';
            else
                s[i]++;
            return s;
        }
    }
    return s;
}

string remove_comment(string s)
{
    string new_s;
    bool prev_space = true; // start as true so we skip leading spaces too
    for (char c : s)
    {
        // if we see a semicolon, STOP! everything after is a comment
        if (c == ';')
            return new_s;

        // only add ONE space in a row (squish multiple spaces into one)
        if (c == ' ' && !prev_space)
        {
            prev_space = true;
            new_s.push_back(c);
        }
        // add any non-space character normally
        if (c != ' ')
        {
            prev_space = false;
            new_s.push_back(c);
        }
    }
    return new_s;
}

// breaks the line into words
vector<string> break_parts(string line)
{
    stringstream ss(line);
    vector<string> words;
    string word;

    // keep grabbing words until the line is empty
    while (ss >> word)
        words.push_back(word);

    return words;
}

// looks up instruction in the dictionary
const instruction *find_instruction(const string &word)
{
    for (int i = 0; ins_type[i].mnemonic != nullptr; i++)
    {
        // compare the word to each instruction name we know
        if (strcmp(ins_type[i].mnemonic,
                   word.c_str()) == 0)
        {
            return &ins_type[i]; // found it! give back the info
        }
    }
    return nullptr; // if not found return null
}


int is_a_label(unordered_map<string, string> &Label_Table, string word, string line_no)
{
    if (word.back() == ':')
    {
        string label = word;
        label.pop_back();

        // labels cant have the same name as real instructions like "add:" is not allowed
        if (find_instruction(label) != nullptr)
            return -1;

        // we cant have two labels with the same name
        if (Label_Table.find(label) != Label_Table.end())
            return -2; 

        bool is_label = true;

        // labels cant start with a number and cant have non alpha numeric characters
        if (label.empty() || isdigit(label[0]))
            is_label = false;
        for (char c : label)
            if (!isalnum(c)) 
                is_label = false;

        if (is_label)
        {
            // put the label in label table
            Label_Table[label] = line_no;
            return 1; 
        }
    }
    return -3; // not a label 
}

string convert_no_to_hex(string s)
{
    char *end;
    // strtol handles 0x prefix for hex, 0 prefix for octal, plain digits for decimal
    long value = strtol(s.c_str(), &end, 0);

    // if end isnt pointing to the end of string, there were bad characters
    int flag = (*end != '\0');

    // mask to 32 bits (handles negatives nicely with two's complement wrapping)
    unsigned int v = value & 0xFFFFFFFF;

    stringstream ss;
    ss << uppercase << hex
       << setw(8) << setfill('0') 
       << v;

    return ss.str();
}

// rewrites a label's value when we see "SET" instruction
// like this label should equal THIS number instead of a memory address
int rewrite_label(string label, vector<string> words)
{
    label.pop_back(); 
    string new_string;

    // SET needs exactly one argument (the value to set the label to)
    if (words.size() == 2)
        new_string = convert_no_to_hex(words[1]);
    if (words.size() > 2)
        return -6; // too many arguments! only need one number
    if (words.size() < 2) // missing argument
        return -9;
    Label_Table[label] = new_string; 
    return 0; 
}

string convert_opcode(int n)
{
    string digits = "0123456789ABCDEF";
    string hex = "";

    hex.push_back(digits[(n >> 4) & 0xF]);    // upper nibble first 4 bits
    hex.push_back(digits[n & 0xF]);           // lower nibble last  4 bits

    return hex;
}

// it can hold signed negatives down to -2^23 or positives up to 2^24-1
static const long MIN_24 = -8388608L;   // -2^23 (smallest negative)
static const long MAX_24 =  16777215L;  //  2^24 - 1 = 0xFFFFFF (biggest positive)

// converts an operand string to a 6-character hex string (24 bits)
// if its a number, converts it directly
// if its a label name, looks it up in Label Table
pair<int, string> convert_value(string operand)
{
    char *end;
    long value = strtol(operand.c_str(), &end, 0); // try to read it as a number

    // flag = 1 means there were non-number characters (so its probably a label name)
    int flag = (*end != '\0');

    if (!flag)
    {
        // its a number - check it fits in 24 bits!
        if (value > MAX_24)
            return {-12, "_ERROR__"};  // too big! positive overflow
        if (value < MIN_24)
            return {-13, "_ERROR__"};  // too small! negative overflow
    }

    // mask to 24 bits and format as 6 hex digits
    unsigned int v = value & 0xFFFFFF;

    stringstream ss;
    ss << uppercase << hex << setw(6) << setfill('0') << v;

    if (flag)
    {
        // it wasnt a number, so it must be a label - look it up
        if (Label_Table.find(operand) != Label_Table.end())
        {
            string s = Label_Table[operand];
            string last6 = s.substr(s.size() - 6); // take the last 6 hex digits (24 bits)
            ss.str(last6);
        }
        else
            return {-8, "_ERROR__"}; // label not found
    }

    return {0, ss.str()}; 
}

pair<int, string> convert_offset(string operand, string current)
{
    if (Label_Table.find(operand) != Label_Table.end())
    {
        string target = Label_Table[operand]; 

        // offset = target_addr - current_addr - 1
        long x = stol(target,  nullptr, 16);
        long y = stol(current, nullptr, 16);
        long offset = x - y - 1;

        // check the offset fits in 24 bits too!
        if (offset > MAX_24)
            return {-12, "_ERROR__"}; 
        if (offset < MIN_24)
            return {-13, "_ERROR__"};  

        offset &= 0xFFFFFF; // mask to 24 bits

        stringstream ss;
        ss << uppercase << hex << setw(6) << setfill('0') << offset;
        return {0, ss.str()};
    }
    else
        // label wasnt found, try treating it as a literal number instead
        return convert_value(operand);

    return {0, "000000"}; // defaut
}

pair<int, string> convert_immedt(const instruction *inst, vector<string> words, string line_no)
{
    int op = inst->op;

    // instructions with NO operand shouldnt have anything after them
    if (!inst->operand && !inst->branch)
        if (words.size() - 1 != 0)
            return {-6, "_ERROR__"}; 

    // instructions WITH an operand need exactly ONE thing after them
    if (inst->operand || inst->branch)
    {
        if (words.size() - 1 > 1)
            return {-6, "_ERROR__"}; // too many operands! we only want one

        if (words.size() - 1 < 1)
            return {-9, "_ERROR__"}; // missing operand!

        else
        {
            // convert based on whether it's a regular value or a branch offset
            if (inst->operand)
                return convert_value(words[1]);
            if (inst->branch)
                return convert_offset(words[1], line_no);
        }
    }

    return {0, "000000"}; // for no-operand instructions, the field is all zeros
}

pair<int, string> decode_instruction(vector<string> words, string line_no, int l)
{
    int e = 0;
    string code = "        "; // 8 spaces = placeholder for 8 hex digits of machine code
    const instruction *inst = find_instruction(words[0]); 

    if (!inst)
        return {-5, "_ERROR__"}; // invalid mnemonic

    if (l == 0 && inst->op == 254)
        return {-10, "_ERROR__"}; // SET without a label

    // SET with a label is fine, but still check argument count
    if (l == 1 && inst->op == 254)
    {
        if (words.size() > 2)
            return {-6, "_ERROR__"}; // too many args to SET
        if (words.size() < 2)
            return {-9, "_ERROR__"}; // missing the value to SET
    }

    string opcode = convert_opcode(inst->op);
    auto immediate = convert_immedt(inst, words, line_no);

    code[0] = immediate.second[0];
    code[1] = immediate.second[1];
    code[2] = immediate.second[2];
    code[3] = immediate.second[3];
    code[4] = immediate.second[4];
    code[5] = immediate.second[5];
    code[6] = opcode[0];
    code[7] = opcode[1];

    // special cases for pseudo-instructions:
    if (opcode == "FF" && l == 0)
        e = -11; // warning: unlabeled data
    if (opcode == "FF" && l == 1)
        e = 8; // fine! its labeled data
    if (opcode == "FE")
        e = 9; // its a SET, treat it special
    if (opcode != "FF" && opcode != "FE")
        e = immediate.first;

    return {e, code};
}

// only for pseudo instructions 
string print_machine(string s)
{
    string new_s = "00" + s.substr(0,6); // take first 6 chars and prepend "00"
    return new_s;
}

void print_to_file(meta_data DATA, ofstream &out, int mode)
{
    if (mode == 2)
    {
        // in the listing file, pseudo instructions are shown with "00" prefix
        if(DATA.error == 9 || DATA.error == 8 || DATA.error == -11)
            out << "  " << DATA.address << "   " << print_machine(DATA.machine_code) << "   " << left << setw(20) << DATA.assembly_code << "\n";
        else
            out << "  " << DATA.address << "   " << DATA.machine_code << "   " << left << setw(20) << DATA.assembly_code << "\n";
    }

    if (mode == 3)
    {
        // same code display as listing file...
        if(DATA.error == 9 || DATA.error == 8 || DATA.error == -11)
            out << "  " << DATA.address << "   " << print_machine(DATA.machine_code) << "   " << left << setw(25) << DATA.assembly_code;
        else
            out << "  " << DATA.address << "   " << DATA.machine_code << "   " << left << setw(25) << DATA.assembly_code;

        // ...but then ALSO print an error message explaining what went wrong!
        switch (DATA.error)
        {
        case -1:   out << ">> ERROR   : Label cannot be instruction name\n";     break;
        case -2:   out << ">> ERROR   : Label conflict duplicate found\n";       break;
        case -3:   out << ">> ERROR   : invalid label\n";                        break;
        case -4:   out << ">> WARNING : Empty Label\n";                          break;
        case -5:   out << ">> ERROR   : invalid Mnemonic\n";                     break;
        case -6:   out << ">> ERROR   : Unexpected operand\n";                   break;
        case -7:   out << ">> ERROR   : Invalid Number\n";                       break;
        case -8:   out << ">> ERROR   : Label not found / invalid operand\n";    break;
        case -9:   out << ">> ERROR   : Missing operand\n";                      break;
        case -10:  out << ">> ERROR   : Specify the label to SET value to\n";    break;
        case -11:
            if (DATA.label != 1)  out << ">> WARNING : Unlabeled memory allocation data cannot be accessed via symbol\n";
            else                  out << "\n";   
            break;
        case -12:  out << ">> ERROR   : Positive overflow — operand exceeds 24-bit max (16777215 / 0xFFFFFF)\n";  break;
        case -13:  out << ">> ERROR   : Negative overflow — operand exceeds 24-bit signed min (-8388608)\n";      break;
        default:   out << "\n";
        }
    }
}

// checks if a string looks like a proper 8-character hex machine code (not "_ERROR__" or spaces or anything weird)
bool hex_checker(const string &s)
{
    // must be exactly 8 characters
    if (s.size() != 8)
        return false;

    // every character must be a valid hex digit (0-9 or A-F)
    for (char c : s)
        if (!isxdigit(c))
            return false;

    return true;
}

void write_obj_file(string filename, vector<meta_data> DATA)
{
    // change the file extension to .obj
    size_t pos = filename.rfind('.');
    if (pos != string::npos)
        filename.replace(pos, string::npos, ".obj");

    ofstream file(filename, ios::binary); // open in binary mode!

    for (const auto &line : DATA)
    {
        string hex = line.machine_code;

        // skip lines that dont have valid machine code (errors, empty labels, etc)
        if (!hex_checker(hex))
            continue;

        // split the 8-hex-digit code into 4 individual bytes
        // and write them LSB first (little-endian) 
        char b1 = (char)stoi(hex.substr(6, 2), nullptr, 16); // byte 0 = LSB (opcode)
        char b2 = (char)stoi(hex.substr(4, 2), nullptr, 16); // byte 1
        char b3 = (char)stoi(hex.substr(2, 2), nullptr, 16); // byte 2
        char b4 = (char)stoi(hex.substr(0, 2), nullptr, 16); // byte 3 = MSB

        file.put(b1);
        file.put(b2);
        file.put(b3);
        file.put(b4);
    }

    file.close();

    cout << "Object file    : " << filename << endl;
}

void write_lst_file(string filename, vector<meta_data> DATA)
{
    // change extension to .lst
    size_t pos = filename.rfind('.');
    if (pos != string::npos)
        filename.replace(pos, string::npos, ".lst");

    ofstream file(filename);

    for (const auto &line : DATA)
        print_to_file(line, file, 2); // mode 2 = listing format

    file.close();

    cout << "Listing file   : " << filename << endl;
}

void write_log_file(string filename, vector<meta_data> DATA)
{
    // change file extension to .log
    size_t pos = filename.rfind('.');
    if (pos != string::npos)
        filename.replace(pos, string::npos, ".log");

    ofstream file(filename);

    for (const auto &line : DATA)
        print_to_file(line, file, 3); // mode 3 = log format 

    file.close();

    cout << "Log file       : " << filename << endl;
}

// the file is read only once savind file input output time complexity
// the assembler runs in TWO PASSES over the source code:
//   PASS 1: collect all label names and their addresses
//   PASS 2: actually convert instructions to machine code (now we know all labels!)
int main(int c, char* args[]) 
{
    // need at least one argument - the filename to assemble!
    if (c < 2) 
    {
        cerr << "Usage: " << args[0] << " <filename>\n";
        return 1;
    }

    string filename = args[1];

    ifstream file(filename);

    // cant assemble a file that doesnt exist!
    if (!file.is_open())
    {
        cout << "Failed to open file";
        return 1;
    }

    string print_no = "00000000";      // current address counter, starts at zero
    string line;
    int e_count = 0;                   // count of errors
    vector<meta_data> DATA;            // our big list of all the lines we've processed

    int loop_enable = 1; // keep looping until we're done
    int parse = 1;       // which pass are we on? 1 = first pass, 2 = second pass
    int i = 0;           // index into DATA for pass 2
    int total;           // total number of lines (set when we hit EOF in pass 1)

    while (loop_enable)
    {
        // ================ PASS 1: Making the Label Table  ==================
        if (parse == 1)
        {
            getline(file, line); 
            stringstream ss(line);
            string word;

            line = remove_comment(line); 
            if (line.empty())
                continue; 

            string M_code = "";
            string assembly = line; // save original line (assembly code) for display in listing file)
            int label = 0;
            int error = 0;

            vector<string> words;

            // does this line have a colon? then it might have a label!
            if (line.find(':') != string::npos)
            {
                label = 1;

                int no_words = 0;
                size_t pos = line.find(':');

                // handle the weird edge case where someone puts a space before the colon
                // like "myLabel :" - thats not a valid label
                if (line[pos - 1] == ' ')
                    label = 0;

                else
                {
                    word = line.substr(0, pos + 1);                       // grab "labelname:" part
                    words = break_parts(line.substr(pos + 1));            // grab everything AFTER the colon
                    no_words = words.size();

                    error = is_a_label(Label_Table, word, print_no);      // is it valid?
                    if (no_words == 0 && error > 0)
                        error = -4; // label exists but nothing comes after it - empty label!
                }
            }

            // store this line's info in DATA (machine code will be filled in during pass 2)
            DATA.push_back({print_no, "", assembly, label, error});

            i++;

            // empty labels dont take up memory space in object file, ie no machine code is generated skip advancing the address counter
            if (error == -4)
                continue;

            // for SET pseudo-instructions, update the label's stored value in the table
            if (label == 1 && words.size() > 1)
            {
                if (words[0] == "SET")
                    DATA[i].error = rewrite_label(word, words);
            }

            // advance the address counter to the next slot
            print_no = hex_counter(print_no);

            // when we reach end-of-file, switch to pass 2!
            if (file.peek() == EOF)
            {
                parse = 2;
                total = i;       // remember how many lines we have
                i = 0;           // reset index to go through DATA from the beginning
            }
        }

        // ========== PASS 2: convert instructions to machine code ==========
        if (parse == 2)
        {
            // went through all lines, we're done!
            if (i == total)
                break;

            string line_no = DATA[i].address;    // address of current line
            line = DATA[i].assembly_code;        // original text of current line
            int error = DATA[i].error;           // any error from pass 1?
            int index = 0;
            string M_code;

            // find the colon position again to re-split label from instruction
            size_t pos = -1; // -1 means "no label" (will become 0 after +1, pointing at whole string)
            if (DATA[i].label == 1)
                pos = line.find(':');
            auto words = break_parts(line.substr(pos + 1)); // get just the instruction part from the line

            // if pass 1 already found an error on this line, skip decoding it
            if (error < 0)
            {
                if (error == -4)
                    DATA[i].machine_code = "        "; // empty label = no machine code at all
                else
                    DATA[i].machine_code = "_ERROR__"; // mark it as errored
                i++;
                continue;
            }

            // Decoding instruction into Machine Code!
            auto result = decode_instruction(words, line_no, DATA[i].label);

            DATA[i].error = result.first;
            DATA[i].machine_code = result.second;

            // count actual errors (not warnings)
            if (DATA[i].error != -4 && DATA[i].error >= -13 && DATA[i].error <= -1)
                e_count++;
            i++;
        }
    }

    // always write the .obj and .lst files
    write_obj_file(filename, DATA);
    write_lst_file(filename, DATA);

    // only write the .log file if there were actual errors to report!
    if (e_count != 0)
        write_log_file(filename, DATA);

    file.close();
    return 0; 
}