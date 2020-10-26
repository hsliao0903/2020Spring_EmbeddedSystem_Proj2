//  CDS 4630/CDA 5636 Embedded System Project 2
//  NAME: Hsiang-Yuan Liao
//  UFID: 4353-5341
//  SIM.cpp
//
//
//  Created by Hsiang-Yuan Liao on 3/24/20.
//  On my honor, I have neither given nor received unauthorized aid on this assignment.


#include <iostream>
#include <bitset>
#include <math.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

#define BITSIZE 32

using namespace std;


//for establishing Dictionary, instruction frequecies
class InstructionNode {
    public:
    int frequency;
    int order;
    
    //constructor
    InstructionNode(const int freq, const int index) {
        frequency = freq;
        order = index;
    }
};

// for Dictionary
class DictinaryNode {
    public:
    string instruction;
    int frequency;
    
    //constructor
    DictinaryNode(const string ins, const int freq) {
        instruction = ins;
        frequency = freq;
    }
};



//for compression output
class CompressedNode {
    public:
    string instruction;
    string option;
    string format;
    
    //constructor
    CompressedNode(const string ins) {
        option = "";
        format = "";
        instruction = ins;
    }
};

//for preferable option, "less bit"
class PreferOption {
    public:
    string result;
    string option;
    int index;
    
    //constructor
    PreferOption(const string opt, const int id) {
        option = opt;
        index = id;
    }
};

//for decompression output
class DecompressedNode {
    public:
    string option;
    string format;
    
    //constructor
    DecompressedNode(const string opt, const string fmt) {
        option = opt;
        format = fmt;
    }
};

//compression used
vector<DictinaryNode*> Dictionary;
map<string, InstructionNode*> Instruction;
vector<CompressedNode*> CompressedCode;
string option_priority[6] = {"101", "010", "011", "001", "100", "110"};


//decompression used
map<string, int> format_bitCount;  //how many bits in each option of format
vector<DecompressedNode*> DecompressedCode;
vector<string> OriginalCode;

//function declartion
void start_compression();
void compress_instruction(string ins, int index);
void start_RLE(string ins, int start, int end);
void establish_dictionary(int size, string filename);
string get_most_frequent_ins(void);
PreferOption* get_profitable_option(vector<PreferOption*> opstions);
int get_leftmost_1_bit(string bits);
void get_dict_from_compressed_code(string filename);
string get_format(string opt, string result, int dict_idx);
string bitmask_based_comp(string ins);
string mismatch_anywhere_comp(string ins);
void getDecompFormat(string filename);
int get_two_setbit_distance(bitset<BITSIZE> bits);
int is_Consecutive(bitset<BITSIZE> bits, int mismatches);
int find_end_of_repeat(string ins, int start);
int transer_bit_to_int(string bits);
void start_decompression();
void decompress_instructions(string option, string format, string previous_ins);
//print functions
void compressed_code_print(ofstream &opfile);
void decompressed_code_print(ofstream &opfile);

//main function
int main(int argc, char* argv[]) {

    if(*argv[1] == '1') { //compression
        establish_dictionary(8, "original.txt");
        start_compression();

        ofstream outputFile("./cout.txt");
        compressed_code_print(outputFile);
        outputFile.close();
    } else if(*argv[1] == '2') { //decompressing
        
        //format length for every option
        format_bitCount["000"] = 2;
        format_bitCount["101"] = 3;
        format_bitCount["010"] = 8;
        format_bitCount["011"] = 8;
        format_bitCount["001"] = 12;
        format_bitCount["100"] = 13;
        format_bitCount["110"] = 32;
        
        get_dict_from_compressed_code("compressed.txt");
        getDecompFormat("compressed.txt");
        start_decompression();
        
        ofstream outputFile("./dout.txt");
        decompressed_code_print(outputFile);
        outputFile.close();
    }

    return 0;
}


void decompressed_code_print(ofstream &opfile) {
    int i;
    for(i=0;i<OriginalCode.size();i++) {
        opfile << OriginalCode[i] << endl;
    }
}

void compressed_code_print(ofstream &opfile) {
    int count = 0, padbit, i, j;
    string str;

    for(i=0;i<CompressedCode.size();i++) {
        str = CompressedCode[i]->option + CompressedCode[i]->format;
        for(j=0;j<str.size();j++) {
            opfile << str[j];
            count++;
            
            if(count == BITSIZE) {
                count = 0;
                opfile << endl; //next line
            }
        }
    }

    padbit = 32 - count;

    for(i=0;i<padbit;i++)
        opfile << '1';

    opfile << endl;
    opfile << "xxxx" << endl; //for dictionary

    for(i=0;i<Dictionary.size();i++) {
        opfile << Dictionary[i]->instruction << endl;
    }
}

void start_compression() {
    int start = 0, end = 0, i;
    string previous_ins = "";
    
    for(i=0;i<CompressedCode.size();i++) {
        //we don't need to check RLE for the very first instruction
        if(i != 0) {
            start = i - 1;
            previous_ins = CompressedCode[i-1]->instruction;
        }

        //we can do RLE if at least two consecutive ins are the same
        if(CompressedCode[i]->instruction == previous_ins) {
            end = find_end_of_repeat(CompressedCode[i]->instruction, i);
            start_RLE(CompressedCode[i]->instruction,start,end);
            i = end;
            //go to next round
            continue;
        }
        //if we don't do RLE, than we compress it by other option
        compress_instruction(CompressedCode[i]->instruction, i);
    }
}

//compress function one instruction by instruction, go thru whole dictinary first
void compress_instruction(string ins, int index) {
    vector<PreferOption*> currOptions;
    PreferOption* prefer_option;
    
    int isConsecutive = 0;
    int mismatches = 0;
    int distance = -1;
    int i;

    bitset<BITSIZE> insbit(ins);

    //go through whole dictionary
    for(i=0;i<Dictionary.size();i++) {
        PreferOption* temp_option = new PreferOption("110", i); // option, id
        bitset<BITSIZE> dictionarybit(Dictionary[i]->instruction);
        bitset<BITSIZE> result(insbit ^ dictionarybit);
        //count all the set bits in result after XOR opertion
        mismatches = result.count();
        temp_option->result = result.to_string();

        if(mismatches > 1) {
            isConsecutive = is_Consecutive(result, mismatches);
            distance = get_two_setbit_distance(result);
        }

        //this is in priotity order
        if(mismatches == 0) {
            temp_option->option = "101";
        } else if(mismatches == 1) {
            temp_option->option = "010";
        } else if(mismatches == 2 && isConsecutive == 1) {
            temp_option->option = "011";
        } else if(mismatches > 1 && distance <= 2) {
            temp_option->option = "001";
        } else if(mismatches == 2 && distance > 2) {
            temp_option->option = "100";
        } else {
            temp_option->option = "110";
        }
        currOptions.push_back(temp_option);
    }

    prefer_option = get_profitable_option(currOptions);
    CompressedCode[index]->option = prefer_option->option;

    if(prefer_option->option == "110")
        //Original binaries
        CompressedCode[index]->format = CompressedCode[index]->instruction;
    else
        CompressedCode[index]->format = get_format(prefer_option->option, prefer_option->result, prefer_option->index);
}

//A RLE group => at most 5 repeating sequence
void start_RLE(string ins, int start, int end) {
    int i;
    int fst_ins_in_group = 0;
    int last_ins_in_group = 0;
    int count = end - start + 1;
    int total_group = ceil((float)count/5.0);


    //if there is at most 5 consecutive repeating seq
    if (total_group <= 1) {
        fst_ins_in_group = start;
        last_ins_in_group = end;
        compress_instruction(CompressedCode[fst_ins_in_group]->instruction, fst_ins_in_group);
        CompressedCode[last_ins_in_group]->option = "000"; //RLE option code => 000
        bitset<2> repeat(count-2); // 00=1, 01=2, 10=3, 11=4
        CompressedCode[last_ins_in_group]->format = repeat.to_string();
        //printf("[TEST] %s %s\n", CompressedCode[last_ins_in_group]->option.c_str(), CompressedCode[last_ins_in_group]->format.c_str());
        return;
    }

    //more than 5 consecutive repeating seq
    i = 0;
    while(i<total_group) {  //for each group
        fst_ins_in_group = start + (i * 5);
        
        if(total_group - i == 1) {
            last_ins_in_group = end;
        } else {
            last_ins_in_group = fst_ins_in_group + 4; //at most 4 repeation
        }

        compress_instruction(CompressedCode[fst_ins_in_group]->instruction, fst_ins_in_group);
        
        //if fst = last ins in the group, we are already done
        if(fst_ins_in_group == last_ins_in_group)
            break;
        
        CompressedCode[last_ins_in_group]->option = "000";
        bitset<2> repeat(count-2);
        CompressedCode[last_ins_in_group]->format = repeat.to_string();
        i++;
    }

    return;
}

//find the end of the repeat ins for RLE
int find_end_of_repeat(string ins, int start) {
    int end = 0;

    while(CompressedCode[start]->instruction == ins) {
        end = start;
        start++;

        if(start == (CompressedCode.size()))
            break;
        
    }

    return end;
}

//establish our dictianry according some rules
void establish_dictionary(int size, string filename) {
    
    string line;
    string most_freq_ins;
    int i = 0;
    ifstream infile(filename.c_str());

    for(i=0;getline(infile,line);i++) {
        CompressedCode.push_back(new CompressedNode(line));
        
        //found repeated instruction, add the frequency
        if(Instruction.count(line) != 0)
            Instruction[line]->frequency++;
        else
            Instruction[line] = new InstructionNode(1, i);
    }
    
 
    //incase original code size is smaller than dict size
    if(size >= Instruction.size())
        size = Instruction.size();
    
    i = 0;
    while(i<size) {
        most_freq_ins = get_most_frequent_ins();
        DictinaryNode* entry = new DictinaryNode(most_freq_ins, Instruction[most_freq_ins]->frequency);
        Dictionary.push_back(entry);
        Instruction.erase(most_freq_ins); //remove it
        i++;
    }
}

//get the most frequent ins to form dictionary
string get_most_frequent_ins(void) {
    int max_freq = 0;
    int max_order = 0;
    string target;

    for(map<string, InstructionNode*>::iterator it = Instruction.begin(); it != Instruction.end(); ++it) {
        //find the max first
        if(it->second->frequency > max_freq) {
                   max_freq = it->second->frequency;
                   max_order = it->second->order;
                   target = it->first;
        } else if(it->second->frequency == max_freq) {
            //if we have the same frequecy, find the first one that appears
            if (it->second->order < max_order) {
                max_order = it->second->order;
                target = it->first;
            }
        }
    }

    return target;
}

//get dictionary from compressed.txt
void get_dict_from_compressed_code(string filename) {
    ifstream infile(filename.c_str());
    string line;
    int cur_line = 0;
    bool found = false;
    
    while(getline(infile, line)) {
        if(found){
            DictinaryNode* entry = new DictinaryNode(line, 0);
            Dictionary.push_back(entry);
        }
        
        if(line == "xxxx" && found == false){
            found = true;
        }
        cur_line++;
    }
}

//gets the first set bit in a bit string starting at the MSB
int get_leftmost_1_bit(string s) {
    int i;
    for(i = 0;i<s.length();i++) {
        if(s[i] == '1') {
            return i;
        }
    }
    return 0;
}

//get compressed format for each option
string get_format(string opt, string result, int dict_idx) {
    bitset<3> dictionary_id(dict_idx);
    string format = "";

    if(opt == "001") {
        format = bitmask_based_comp(result);
    } else if(opt == "010" || opt == "011") {
        bitset<5> startBits(get_leftmost_1_bit(result));
        format = startBits.to_string();
    } else if(opt == "100") {
        format = mismatch_anywhere_comp(result);
    }

    return format += dictionary_id.to_string(); //add dictiaonary id at the end of format

}


//get better option
PreferOption* get_profitable_option(vector<PreferOption*> options) {
    int i, j;

    for(i=0;i<6;i++)
        for(j=0;j<options.size();j++)
            //highest priority that matches for this ins
            if(option_priority[i] == options[j]->option)
                return options[j];
    return 0;
}


/* different option of compression */

//"001" option, get bitmask based compression format
string bitmask_based_comp(string ins) {
    //printf("[ALEX] ins = %s\n", ins.c_str());
    int start = get_leftmost_1_bit(ins), i;
    bitset<5> startBits(start);
    string format = "";
    string bitmask = "";
    //printf("[ALEX] start = %s\n", start.c_str());

    for(i=start;i<(start+4);i++)
        bitmask += ins[i];
    
    //printf("[ALEX] bimask = %s\n", bitmask.c_str());
    format = startBits.to_string() + bitmask;
     
    return format;
}

//"100" option, find two mismatch locations
string mismatch_anywhere_comp(string ins) {
    int first = 0;
    int second = 0;
    int i;
    bool found_first = false;
    
    for(i = 0;i<ins.length();i++) {

        if (ins[i] == '1') {
            if(!found_first) {
                first = i;
                found_first = true;
            } else {
                second = i;
                break;
            }
        }
    }
    
  
    bitset<5> first_bit(first);
    bitset<5> second_bit(second);

    return first_bit.to_string() + second_bit.to_string();
}

/* For decompression */

//find the compressed code of optoin and format for every instruction, we will use the information to
//decompress it back to original code
void getDecompFormat(string filename) {
    string all_content = "";
    string line;
    string option;
    string format;
    ifstream infile(filename.c_str());
    int i = 0;
    
    while(getline(infile, line)) {
        if(line == "xxxx")
            break;
        
        all_content += line;
    }
    
    while(i<all_content.size()) {
        option = all_content.substr(i, 3); //optoin is 3 bits long
        i += 3;
        
        //we don't need the pad bits
        if( option == "111")
            break;
        
        format = all_content.substr(i, format_bitCount[option]);
        DecompressedNode* ins = new DecompressedNode(option, format);
        DecompressedCode.push_back(ins);
        
        //get next compressed ins
        i += (format_bitCount[option]-1);
        i++;
    }
}

//check is the mismatches are consecutive
int is_Consecutive(bitset<BITSIZE> bits, int mismatches) {
    bitset<BITSIZE> temp(bits);
    int i;
    
    for(i=0;i<mismatches;i++) {
        if((bits & temp) == 0)
            return 0;
        temp = temp << 1;
    }
    return 1;
}

//get the distance of two set bits
int get_two_setbit_distance(bitset<BITSIZE> bits) {
    int first = -1, second = 0, i = 0;
    
    for(i=0;i<BITSIZE;i++) {
        if(bits[i] == 1 && first == -1)
            first = i;
        else if(bits[i] == 1)
            second = i;
    }
    
    return second - first -1;
}


int transer_bit_to_int(string bits) {
    bitset<5> tmp(bits);
    return tmp.to_ulong();
}

void start_decompression() {
    int i;
    string previous_ins;

    for(i=0;i<DecompressedCode.size();i++) {
        if(i > 0) {
            previous_ins = OriginalCode[OriginalCode.size()-1];
        }
        decompress_instructions(DecompressedCode[i]->option, DecompressedCode[i]->format, previous_ins);
    }
}

void decompress_instructions(string option, string format, string previous_ins) {
    int i;
    int start;
    int dict_idx;
    int first, second;
    bitset<BITSIZE> result(0);
    bitset<BITSIZE> original(0);

    if (option == "110") {
        OriginalCode.push_back(format);
    } else if (option == "000") { //for RLE
        bitset<2> formatBits(format);
        int count = formatBits.to_ulong();

        for(i=0;i<=count;i++)
            OriginalCode.push_back(previous_ins);
        
    } else if(option == "001") {
        start = transer_bit_to_int(format.substr(0, 5));
        dict_idx = transer_bit_to_int(format.substr(9, 3));
        bitset<BITSIZE> bitmask(format.substr(5, 4));
        bitmask = bitmask << (31 - start - 3);
        bitset<BITSIZE> dictionary(Dictionary[dict_idx]->instruction);
        result = result ^ bitmask;
        original = result ^ dictionary;
        OriginalCode.push_back(original.to_string());
    } else if(option == "010") { //1 bit mismatch
        start = transer_bit_to_int(format.substr(0, 5));
        dict_idx = transer_bit_to_int(format.substr(5, 3));
        bitset<BITSIZE> dictionary(Dictionary[dict_idx]->instruction);
        result[31 - start].flip();
        original = result ^ dictionary;
        OriginalCode.push_back(original.to_string());
    } else if(option == "011") { //2 consecutive bit mismatch
        start = transer_bit_to_int(format.substr(0, 5));
        dict_idx = transer_bit_to_int(format.substr(5, 3));
        bitset<BITSIZE> dictionary(Dictionary[dict_idx]->instruction);
        result[31 - start].flip();
        result[30 - start].flip();
        original = result ^ dictionary;
        OriginalCode.push_back(original.to_string());
    } else if(option == "100") {
        first = transer_bit_to_int(format.substr(0, 5));
        second = transer_bit_to_int(format.substr(5, 5));
        dict_idx = transer_bit_to_int(format.substr(10, 3));
        result[31 - first].flip();
        result[31 - second].flip();
        bitset<BITSIZE> dictionary(Dictionary[dict_idx]->instruction);
        original = result ^ dictionary;
        OriginalCode.push_back(original.to_string());
    } else {
        dict_idx = transer_bit_to_int(format.substr(0, 3));
        OriginalCode.push_back(Dictionary[dict_idx]->instruction);
    }
}
