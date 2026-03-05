// Name : Darla Sravan Kumar
// Roll : 2401CS45

// All work presented here is independently developed by the author (Sravan)

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
    bool operand;
    bool branch;
    bool pseudo;
};

static const instruction ins_type[] = {
    {"data", 255, true, false, true},
    {"ldc", 0, true, false, false},
    {"adc", 1, true, false, false},
    {"ldl", 2, true, false, false},
    {"stl", 3, true, false, false},
    {"ldnl", 4, true, false, false},
    {"stnl", 5, true, false, false},
    {"add", 6, false, false, false},
    {"sub", 7, false, false, false},
    {"shl", 8, false, false, false},
    {"shr", 9, false, false, false},
    {"adj", 10, true, false, false},
    {"a2sp", 11, false, false, false},
    {"sp2a", 12, false, false, false},
    {"call", 13, false, true, false},
    {"return", 14, false, false, false},
    {"brz", 15, false, true, false},
    {"brlz", 16, false, true, false},
    {"br", 17, false, true, false},
    {"HALT", 18, false, false, false},
    {"SET", 254, true, false, true},
    {nullptr, 0, false, false, false}};

struct meta_data
{
    string address;
    string machine_code;
    string assembly_code;
    int label;
    int error;
};

unordered_map<string, string> Label_Table;

string hex_counter(string s)
{
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
    bool prev_space = true;
    for (char c : s)
    {
        if (c == ';')
            return new_s;
        if (c == ' ' && !prev_space)
        {
            prev_space = true;
            new_s.push_back(c);
        }
        if (c != ' ')
        {
            prev_space = false;
            new_s.push_back(c);
        }
    }
    return new_s;
}

vector<string> break_parts(string line)
{
    stringstream ss(line);
    vector<string> words;
    string word;

    while (ss >> word)
        words.push_back(word);

    return words;
}

const instruction *find_instruction(const string &word)
{
    for (int i = 0; ins_type[i].mnemonic != nullptr; i++)
    {
        if (strcmp(ins_type[i].mnemonic,
                   word.c_str()) == 0)
        {
            return &ins_type[i];
        }
    }
    return nullptr;
}

int is_a_label(unordered_map<string, string> &Label_Table, string word, string line_no)
{
    if (word.back() == ':')
    {
        string label = word;
        label.pop_back();

        if (find_instruction(label) != nullptr)
            return -1;
        if (Label_Table.find(label) != Label_Table.end())
            return -2;

        bool is_label = true;

        if (label.empty() || isdigit(label[0]))
            is_label = false;
        for (char c : label)
            if (!isalnum(c))
                is_label = false;

        if (is_label)
        {
            Label_Table[label] = line_no;
            return 1;
        }
    }
    return -3;
}

string convert_no_to_hex(string s)
{
    char *end;
    long value = strtol(s.c_str(), &end, 0);

    int flag = (*end != '\0');

    unsigned int v = value & 0xFFFFFFFF;

    stringstream ss;
    ss << uppercase << hex
       << setw(8) << setfill('0')
       << v;

    return ss.str();
}

int rewrite_label(string label, vector<string> words)
{
    label.pop_back();
    string new_string;

    if (words.size() == 2)
        new_string = convert_no_to_hex(words[1]);
    if (words.size() > 2)
        return -6;
    if (words.size() > 2)
        return -9;
    Label_Table[label] = new_string;
    return 0;
}

string convert_opcode(int n)
{
    string digits = "0123456789ABCDEF";
    string hex = "";

    hex.push_back(digits[(n >> 4) & 0xF]);
    hex.push_back(digits[n & 0xF]);

    return hex;
}

pair<int, string> convert_value(string operand)
{
    char *end;
    long value = strtol(operand.c_str(), &end, 0);

    int flag = (*end != '\0');

    unsigned int v = value & 0xFFFFFF;

    stringstream ss;
    ss << uppercase << hex << setw(6) << setfill('0') << v;

    if (flag)
    {
        if (Label_Table.find(operand) != Label_Table.end())
        {
            string s = Label_Table[operand];
            string last6 = s.substr(s.size() - 6);
            ss.str(last6);
        }
        else
            return {-8, "_ERROR"};
    }

    return {0, ss.str()};
}

pair<int, string> convert_offset(string operand, string l)
{
    if (Label_Table.find(operand) != Label_Table.end())
    {
        string s = Label_Table[operand];

        long x = stol(s, nullptr, 16);
        long y = stol(l, nullptr, 16);
        long diff = x - y - 1;

        diff &= 0xFFFFFF;

        stringstream ss;
        ss << uppercase << hex << setw(6) << setfill('0') << diff;
        return {0, ss.str()};
    }
    else
        return convert_value(operand);

    return {0, "000000"};
}

pair<int, string> convert_immedt(const instruction *inst, vector<string> words, string line_no)
{
    int op = inst->op;

    // if it has no operand
    if (!inst->operand && !inst->branch)
        if (words.size() - 1 != 0)
            return {-6, "_ERROR__"};

    // if it has an operand
    if (inst->operand || inst->branch)
    {
        if (words.size() - 1 > 1)
            return {-6, "_ERROR__"};

        if (words.size() - 1 < 1)
            return {-9, "_ERROR__"};

        else
        {
            if (inst->operand)
                return convert_value(words[1]);
            if (inst->branch)
                return convert_offset(words[1], line_no);
        }
    }

    return {0, "000000"};
}

pair<int, string> decode_instruction(vector<string> words, string line_no, int l)
{
    int e = 0;
    string code = "        ";
    const instruction *inst = find_instruction(words[0]);

    if (!inst)
        return {-5, "_ERROR__"};

    if (l == 0 && inst->op == 254)
        return {-10, "_ERROR__"};

    if (l == 1 && inst->op == 254)
    {
        if (words.size() > 2)
            return {-6, "_ERROR__"};
        if (words.size() < 2)
            return {-9, "_ERROR__"};
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

    if (opcode == "FF" && l == 0)
        e = -11;
    if (opcode == "FF" && l == 1)
        e = 8;
    if (opcode == "FE")
        e = 9;
    if (opcode != "FF" && opcode != "FE")
        e = immediate.first;

    return {e, code};
}

string print_machine(string s)
{
    string new_s = "00" + s.substr(0,6);
    return new_s;
}

void print_to_file(meta_data DATA, ofstream &out, int mode)
{
    if (mode == 2)
    {
        if(DATA.error == 9 || DATA.error == 8 || DATA.error == -11)
            out << "  " << DATA.address << "   " << print_machine(DATA.machine_code) << "   " << left << setw(20) << DATA.assembly_code << "\n";
        else
            out << "  " << DATA.address << "   " << DATA.machine_code << "   " << left << setw(20) << DATA.assembly_code << "\n";
    }

    if (mode == 3)
    {
        if(DATA.error == 9 || DATA.error == 8 || DATA.error == -11)
            out << "  " << DATA.address << "   " << print_machine(DATA.machine_code) << "   " << left << setw(20) << DATA.assembly_code;
        else
            out << "  " << DATA.address << "   " << DATA.machine_code << "   " << left << setw(20) << DATA.assembly_code;

        switch (DATA.error)
        {
        case -1:
            out << ">> ERROR   : Label cannot be instruction name\n";
            break;
        case -2:
            out << ">> ERROR   : Label conflict duplicate found\n";
            break;
        case -3:
            out << ">> ERROR   : invalid label\n";
            break;
        case -4:
            out << ">> WARNING : Empty Label\n";
            break;
        case -5:
            out << ">> ERROR   : invalid Mnemonic\n";
            break;
        case -6:
            out << ">> ERROR   : Unexpected operand\n";
            break;
        case -7:
            out << ">> ERROR   : Invalid Number\n";
            break;
        case -8:
            out << ">> ERROR   : Label not found / invalid operand\n";
            break;
        case -9:
            out << ">> ERROR   : Missing operand\n";
            break;
        case -10:
            out << ">> ERROR   : Specify the label to SET value to\n";
            break;
        case -11:
            if (DATA.label != 1)
                out << ">> WARNING : Unlabeled memory allocation data cannot be accessed via symbol\n";
            else
                out << "\n";
            break;
        default:
            out << "\n";
        }
    }
}

bool hex_checker(const string &s)
{
    if (s.size() != 8)
        return false;

    for (char c : s)
        if (!isxdigit(c))
            return false;

    return true;
}

void write_obj_file(string filename, vector<meta_data> DATA)
{
    size_t pos = filename.rfind('.');
    if (pos != string::npos)
        filename.replace(pos, string::npos, ".obj");

    ofstream file(filename, ios::binary);

    for (const auto &line : DATA)
    {
        string hex = line.machine_code;

        if (!hex_checker(hex))
            continue;

        char b1 = (char)stoi(hex.substr(6, 2), nullptr, 16); // LSB
        char b2 = (char)stoi(hex.substr(4, 2), nullptr, 16);
        char b3 = (char)stoi(hex.substr(2, 2), nullptr, 16);
        char b4 = (char)stoi(hex.substr(0, 2), nullptr, 16); // MSB

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
    size_t pos = filename.rfind('.');
    if (pos != string::npos)
        filename.replace(pos, string::npos, ".lst");

    ofstream file(filename);

    for (const auto &line : DATA)
        print_to_file(line, file, 2);

    file.close();

    cout << "Listing file   : " << filename << endl;
}

void write_log_file(string filename, vector<meta_data> DATA)
{
    size_t pos = filename.rfind('.');
    if (pos != string::npos)
        filename.replace(pos, string::npos, ".log");

    ofstream file(filename);

    for (const auto &line : DATA)
        print_to_file(line, file, 3);

    file.close();

    cout << "Log file       : " << filename << endl;
}

int main(int c, char* args[]) 
{
    if (c < 2) 
    {
        cerr << "Usage: " << args[0] << " <filename>\n";
        return 1;
    }

    string filename = args[1];

    ifstream file(filename);

    if (!file.is_open())
    {
        cout << "Failed to open file";
        return 1;
    }

    string print_no = "00000000";
    string line;
    set<string> errors;
    int e_count = 0;
    vector<meta_data> DATA;

    int loop_enable = 1;
    int parse = 1;
    int i = 0;
    int total;

    while (loop_enable)
    {

        if (parse == 1)
        {
            getline(file, line);
            stringstream ss(line);
            string word;

            line = remove_comment(line); // Remove Comments
            if (line.empty())
                continue;

            string M_code = "";
            string assembly = line;
            int label = 0;
            int error = 0;

            vector<string> words;

            if (line.find(':') != string::npos)
            {
                label = 1;

                int no_words = 0;
                size_t pos = line.find(':');

                if (line[pos - 1] == ' ')
                    label = 0;

                else
                {
                    word = line.substr(0, pos + 1);
                    words = break_parts(line.substr(pos + 1)); // Break the line into words
                    no_words = words.size();

                    error = is_a_label(Label_Table, word, print_no); // check if it is a valid Label
                    if (no_words == 0 && error > 0)
                        error = -4; // checking if its an empty label
                }
            }

            DATA.push_back({print_no, "", assembly, label, error});

            i++;

            if (error == -4)
                continue;

            if (label == 1 && words.size() > 1)
            {
                if (words[0] == "SET")
                    DATA[i].error = rewrite_label(word, words);
            }

            print_no = hex_counter(print_no);

            if (file.peek() == EOF)
            {
                parse = 2;
                total = i;
                i = 0;
            }
        }

        if (parse == 2)
        {
            if (i == total)
                break;

            string line_no = DATA[i].address;
            line = DATA[i].assembly_code;
            int error = DATA[i].error;
            int index = 0;
            string M_code;

            size_t pos = -1;
            if (DATA[i].label == 1)
                pos = line.find(':');
            auto words = break_parts(line.substr(pos + 1)); // Break the line into words

            if (error < 0)
            {
                if (error == -4)
                    DATA[i].machine_code = "        ";
                else
                    DATA[i].machine_code = "_ERROR__";
                i++;
                continue;
            }

            auto result = decode_instruction(words, line_no, DATA[i].label);

            DATA[i].error = result.first;
            DATA[i].machine_code = result.second;

            if (DATA[i].error != -4 && DATA[i].error >= -10 && DATA[i].error <= -1)
                e_count++;
            i++;
        }
    }

    write_obj_file(filename, DATA);
    write_lst_file(filename, DATA);
    if (e_count != 0)
        write_log_file(filename, DATA);

    file.close();
    return 0;
}