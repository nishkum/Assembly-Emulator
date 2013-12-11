#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <cstring>
#include <sstream>
using namespace std;
vector<string> commands;												//the vector holding the commands
map<string , int> labels; 												//the map that maps the labels to the corresponding lines
string filename;														//the filename
int registers[16];														//the register file
int pc;																	//program counter
int memory[1024];														//memory implemented as a 1024 int array
bool Eflag , GTflag;													//the E and GT flag
vector<string> singlecommand;											//temporary array that will hold individual commands of a line


vector<string> extractToken(string s , string delim){					//extracts the tokens of the string s splitted by delim
	char arr[s.size()];
	strcpy(arr , s.c_str());
	const char* de = delim.c_str();
	char* te = strtok(arr, de);
	vector<string> toret;
	while(te!=NULL){
		toret.push_back(string((const char*) te));
		te = strtok(NULL , de );
	}
	return toret;
} 


void loadFile(){														//loads the file contents , commands and labels
	ifstream asmfile;
	asmfile.open(filename.c_str());
	if (!asmfile.is_open()){
		cerr<<"Error Reading the mentioned file .";
		exit(1);
	}
	string temp;
	while(!asmfile.eof()){
		getline(asmfile , temp);
		if (temp.length() == 0) continue;
		string temp1;
		size_t found = temp.find(':');
		if (found == string::npos){
			//no label found
			temp1 = temp;
		}
		else{
			//label found
			vector<string> vec = extractToken(temp.substr(0, found) , " \t\n\r");
			string lab = vec[0];		
			labels[lab] = commands.size();
			temp1 = temp.substr(found+1);
		}

		if (temp1.length()==0) continue;
		size_t foundblank = temp1.find_first_not_of(" \t\n\r");
		if (foundblank == string::npos){
			continue;
		}
		commands.push_back(temp1);
	}
	asmfile.close();
}




//a function that depending on the modifier converts the immediate to its required 32 bit format
int extractImmediate(string ins , string imm){
	//temp is the string that contains the last character
	char temp = ins[ins.size()-1];
	
	int op; 				//the final 32 bit operand to be returned
	unsigned short oper;
	stringstream sst ;

	if (imm.find("0x") != string::npos){
		//hex number
		sst<<hex<<imm;
		sst>>oper;
	}
	else{
		sst<<imm;
		sst>>oper;
	}
	
	if (temp == 'u') {op = (short)oper; op = (op & 0x0000FFFF);}
	else if (temp == 'h'){//modifier = 2
		op = (short)oper; op = op<<16;	
	}
	else op = (short)oper;;
	return op;
}


//a function that basically tells which register is referred in the string ; 
//reg must be a register
int tellWhichRegister(string reg){
	if (reg.compare("sp") == 0) return 14;
	else if (reg.compare("ra") == 0) return 15;
	else if (reg[0] == 'r') return atoi(reg.substr(1).c_str());
	//the following case comes means that the reg string is not a register
	else return -1;
}


//process the hex string to remove the whitespaces and the tabs
string processHex(string hex){
	stringstream toReturn;
	vector<string> ve = extractToken(hex, " \t\r\n");
	for(vector<string>::iterator it = ve.begin() ; it != ve.end(); it++){
		toReturn<<*it;
	}
	return toReturn.str();
}


//executes all arithmetic instructions that are in 3 address format
void execArithmetic(vector<string>& comm){
	int rd,rs1,rs2,op1,op2;
	rd = tellWhichRegister(comm[1]);

	rs1 = tellWhichRegister(comm[2]);
	op1 = registers[rs1];

	int temp1 = tellWhichRegister(comm[3]);
	if (temp1 != -1){
		//means that the third operand is a register
		rs2 = temp1;
		op2 = registers[rs2];
	}
	else{
		//the third operand is an immediate
		op2 = extractImmediate(comm[0] , comm[3]);
	}

	//all operands are ready
	if (comm[0].substr(0,2).compare("or") == 0) registers[rd] = op1 | op2;
	else if (comm[0].substr(0,3).compare("add") == 0)  registers[rd] = op1 + op2;
	else if (comm[0].substr(0,3).compare("sub") == 0)  registers[rd] = op1 - op2;
	else if (comm[0].substr(0,3).compare("mul") == 0)  registers[rd] = op1 * op2;
	else if (comm[0].substr(0,3).compare("div") == 0)  {
		if (op2 == 0){
			cerr<<"Error division by zero ."<<endl;
			exit(1);
		}
		registers[rd] = op1 / op2;
	}
	else if (comm[0].substr(0,3).compare("mod") == 0)  registers[rd] = op1 % op2;
	else if (comm[0].substr(0,3).compare("and") == 0)  registers[rd] = op1 & op2;
	else if (comm[0].substr(0,3).compare("lsl") == 0)  registers[rd] = op1 << op2;//assuming op2 is not negative
	else if (comm[0].substr(0,3).compare("lsr") == 0) {
		registers[rd] = ((unsigned)op1>>op2) ;
	} 
	else if (comm[0].substr(0,3).compare("asr") == 0) {
		registers[rd] = (op1>>op2) ;
	}
}



//executes the 2 address arithmetic instructions - not , mov , cmp
void twoAddArith(vector<string>& comm){
	//read which is the first regiuster
	int op2;
	int r1 = tellWhichRegister(comm[1]);
	int r2 = tellWhichRegister(comm[2]);
	if (r2 != -1 ) {
		//the second operand is a register
		op2 = registers[r2];
	}
	else{
		//r2 == -1
		//the second operand is an immediate
		op2 = extractImmediate(comm[0] , comm[2]);
	}

	//check the first three letters of the instruction and proceed accordingly
	if (comm[0].substr(0,3).compare("not") == 0)  registers[r1] = ~op2;
	else if (comm[0].substr(0,3).compare("cmp") == 0){
		if (registers[r1] == op2) {Eflag = true;GTflag=false;}
		else if (registers[r1] > op2) {GTflag = true;Eflag=false;}
		else {Eflag = false;GTflag =false;}
	}
	else if (comm[0].substr(0,3).compare("mov") == 0){
		registers[r1] = op2;
	}
}


//executes load store instructions
void loadStoreIns(string temp){
	//if a load/store instruction is present it must have a '[' or ']' in its statement
	size_t foundhex = temp.find("0x");
	//for storing commands
	vector<string> comm;
	if (foundhex == string::npos){
		//hex not found
		comm = extractToken(temp , " ,[]\t\n\r");
		if (comm[2][0] == 'r' || comm[2][0] == 's') {
			comm.push_back(comm[2]);
			comm[2] = "0";
		}
	}
	else{
		//hex found
		size_t foundopenbracket = temp.find('[');
		size_t foundclosebracket = temp.find(']');
		//IT cannot be the case that '[' is not found .. it must be found
		string t1 = temp.substr(0, foundhex);
		string t2 = temp.substr(foundopenbracket+1 , foundclosebracket - foundopenbracket - 1);
		
		//since t1 is the part before hex number .. so it will have ld/st instruction string and the first register name
		//t2 will only have the second register name
		//now read the first part before the hex number 
		
		comm = extractToken(t1 , " ,\t\r\n");
		comm.push_back( processHex ( temp.substr(foundhex , foundopenbracket - foundhex )));
		//now read the part between square brackets ..
		vector<string> qw = extractToken( t2 , " \t\n\r");
		comm.push_back(qw[0]);
	}

	//done reading all inputs 
	//now comm[0] contains the instruction .. comm[1] the first register , comm[2]  the immediate and comm[3] the third register
	
	int r1 ,r2 ,imm;
	r1 = tellWhichRegister(comm[1]);
	r2 = tellWhichRegister(comm[3]);
	imm = extractImmediate(comm[0] , comm[2]);

	if (comm[0].compare("ld") == 0){
		//load instruction
		registers[r1] = memory[(registers[r2] + imm)/4];
	}
	else if (comm[0].compare("st") == 0){
		//store instruction
		memory[(registers[r2] + imm)/4] = registers[r1];	
	}
}

//the loop which executes each statement 
void execute(){	
	string temp;
	temp = commands[pc];
	int progSize = commands.size();
	
	int count =0;
	while(pc != progSize){

		temp = commands[pc];
		singlecommand.clear();
		//extra caution - check if the string command is empty
		size_t foundb = temp.find_first_not_of(" \t\n\r");
		if (foundb == string::npos) {pc++;continue;}

		size_t loadst = temp.find('[');
		if (loadst != string::npos){
			//found a load store instruction

			loadStoreIns(temp);
			pc++;
			continue;
		}

		//else not found a load store instruction : continue normally
		//check if a hex number is present

		size_t foundhex = temp.find("0x");
		
		string toprocess ;
		string toaddhex;
		bool hexano ;
		if (foundhex == string::npos){
			//not found any hex number
			toprocess = temp;
			hexano = false;
		}
		else{
			//found hex number 

			toprocess = temp.substr( 0 , foundhex);
			string hex = temp.substr(foundhex);
			//process this hex number 
			toaddhex = processHex(hex) ;
			hexano = true;
		}

		singlecommand = extractToken(toprocess , " ,\t\r\n");
		if (hexano){
			singlecommand.push_back(toaddhex);
		}
	
		//done adding all tokens .. now process
		//if 3 address format arithmetic instruction call execArithmetic
		if (singlecommand[0].compare("add")==0 || singlecommand[0].compare("addu")==0 ||singlecommand[0].compare("addh")==0
			||singlecommand[0].compare("sub")==0 ||singlecommand[0].compare("subu")==0 || singlecommand[0].compare("subh")==0
			||  singlecommand[0].compare("mul")==0 || singlecommand[0].compare("mulu")==0 || singlecommand[0].compare("mulh")==0
			|| singlecommand[0].compare("div")==0 ||singlecommand[0].compare("divu")==0 || singlecommand[0].compare("divh")==0
			|| singlecommand[0].compare("mod")==0 || singlecommand[0].compare("modu")==0 || singlecommand[0].compare("modh")==0
			|| singlecommand[0].compare("and")==0 || singlecommand[0].compare("andu")==0  || singlecommand[0].compare("andh")==0
			||singlecommand[0].compare("or")==0 || singlecommand[0].compare("oru")==0 || singlecommand[0].compare("orh")==0
			|| singlecommand[0].compare("lsl")==0 || singlecommand[0].compare("lslu")==0 || singlecommand[0].compare("lslh")==0
			|| singlecommand[0].compare("lsr")==0 || singlecommand[0].compare("lsru")==0 ||singlecommand[0].compare("lsrh")==0
			|| singlecommand[0].compare("asr")==0 || singlecommand[0].compare("asru")==0 ||singlecommand[0].compare("asrh")==0)
		{
			//if these arithmetic commands then call a  general function
			execArithmetic(singlecommand);
		}

		else if (singlecommand[0].compare("nop") == 0) {pc++;continue;}
		//if (singlecommand[0].compare("cmp") ) 

		else if (singlecommand[0].compare(".print") == 0) {
			int x = tellWhichRegister(singlecommand[1]);
			cout<<registers[x]<<"\n";
		}

		else if (singlecommand[0].substr(0,3).compare("not") == 0 || singlecommand[0].substr(0,3).compare("mov") == 0
				|| singlecommand[0].substr(0,3).compare("cmp") == 0)
		{
			twoAddArith(singlecommand);
		}
		else if (singlecommand[0][0] == 'b'){
			//if the label for branching was not found , then there is an error
			map<string , int>::iterator it = labels.find(singlecommand[1]);
			if (it == labels.end()){
				//the label was not found
				cerr<<"Label specified not found at command number :"<<pc <<"."<<endl;
				exit(1);
			}
			//a branching instruction
			if (singlecommand[0].compare("b") == 0){
				//uncinditional branch
				pc = labels[singlecommand[1]];
			}
			else if (singlecommand[0].compare("beq") == 0){
				if (Eflag) pc = labels[singlecommand[1]];
				else pc++;
			}
			else {
				//bgt instruction
				if (GTflag) pc = labels[singlecommand[1]];
				else pc++;
			}
			continue;
		}
		else if (singlecommand[0].compare("ret") == 0){
			pc = registers[15];
			continue;
		}
		else if (singlecommand[0].compare("call") == 0){
			registers[15] = pc+1;
			pc = labels[singlecommand[1]];
			continue;
		}		
		pc+=1;
	}
}


int main(int argc, char** argv){
	filename = argv[1];
	if (filename.compare(string("")) == 0) {					//if the filename is blank then there is an error
		cerr<<"Error filename not specified ."<<endl;
		exit(1);
	}
	loadFile();													//first load the file
	map<string , int> ::iterator it = labels.find(".main");		//find if the ".main" label is there
	if (it == labels.end()){
		pc = 0;													//if .main is not there then use pc = 0 
	}	
	else{
		pc = labels[".main"];									//else use the .main position as starting point
	}
	registers[14] = 4092;										//initialize the stack pointer
	execute();													//initialize the iterative loop
	return 0;
}