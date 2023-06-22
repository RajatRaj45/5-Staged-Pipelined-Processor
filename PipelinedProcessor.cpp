#include<bits/stdc++.h>
using namespace std;

int MEM[1024];
int R[32];

//1 index represents the EX/MEM registers 
//2 index represents the MEM/WB registers 

int R_ALUOutputReg;
int R_MEMoutdata;
int R_writeBackData;
int R_memWriteData1;
int R_regDst1;
int R_regDst2;

bool zeroFlagGenerated;
bool R_memDo1;
bool R_writeBackDo1;
bool R_regWrite1;
bool R_memRead1;   
bool R_memToReg1;
bool R_memWrite1;
bool R_end1;
bool R_writeBackDo2;
bool R_regWrite2;
bool R_end2;


//IF/ID registers
string R_getOpCode;
string R_Instruction;

//ID/EX registers
int R_RegDst; 
int R_BranchAddress; 
int R_JumpAddress; 
int R_ALUInput1; 
int R_ALUInput2; 
int R_dataForStore; 
int R_ALUControl;

int JA; 
int BA;

bool R_Branch;
bool R_MemRead;
bool R_MemtoReg;
bool R_MemWrite;
bool R_Jump;
bool R_RegWrite;
bool R_ALUDo;
bool R_MemDo;
bool R_WritebackDo;
bool R_End;

bool JumpFlag;
bool BranchPossible;
bool ZeroFlag;
bool EndFlag;

map<string,string> opCode;//opCode for different instructions
map<string,string> funct;//funct type for different R-type instructions
map<string,int> indicesOfLabels;//contains the line index number in the assembly code at which the labels first appeared, needed for branch/jump

int Clock;
bool finito;

/*  Instr.   opCode
    label :  000000
    rtype :  000001
    lw    :  000010
    st    :  000011
    jump  :  000100
    branch:  000101

    R-type:  funct
    add   :  000001
    addi  :  000010
    sub   :  000011
    subi  :  000100
    mul   :  000101  */     


class Assembler{
  public:
    vector<string> vec;
    Assembler(){
        opCode.insert({"rtype","000001"});
        opCode.insert({"lw","000010"});
        opCode.insert({"st","000011"});
        opCode.insert({"j","000100"});
        opCode.insert({"beq","000101"});
        
        funct.insert({"add","000001"});
        funct.insert({"addi","000010"});
        funct.insert({"sub","000011"});
        funct.insert({"subi","000100"});
        funct.insert({"mul","000101"});
    }

    string decToBinary(string reg){
        //takes input as: $3,$4,%10 etc. and converts it to output as a 5 bit string: "00011", "00100", "01010"
        if(reg[0]=='$' || reg[0]=='%')reg.erase(reg.begin());//erase the first symbol of the input register string
        int number=stoi(reg);//convert the string "3" to integer 3
        bitset<5> b(number);//get the binary representation of the integer 
        string temp=b.to_string();//convert the binary representation into a string
        return temp;
    }

    vector<string> elements(string inst){
        vector<string> el;//vector to store the different elements of a single line instruction input
        //e.x. addi $1,$2,10 ===> {"addi", "$1", "$2", "10"}
        string temp="";//temporary string to store the elements being added into the vector
        int ind=-1;
        
        for(int i=0;i<inst.size();i++){
            if(inst[i]==' '){//finding the first element that's present until the first whitespace
                ind=i+1;
                break;
            }
            temp=temp+inst[i];
        }
        
        if(ind==-1){//no whitespace in instruction implying that it's just a label
            inst.erase(inst.begin()+inst.size()-1);//erasing the colon from the label i.e. "loop:" --> "loop"
            el.push_back(inst);
            return el;
        }
        
        el.push_back(temp);
        temp="";//reset temporary string

        bool found=0;
        
        for(int i=ind;i<inst.size();i++){
            if(inst[i]==','){//finding next element upto the first comma
                ind=i+1;
                found=1;
                break;
            }
            temp=temp+inst[i];
        }
        
        if(!found){//no comma found, implying that the instruction is a jump instruction
            el.push_back(inst.substr(ind,inst.size()-ind));//pushing back the label of jump to the vector
            return el;
        }
        
        found=0;
        el.push_back(temp);
        temp="";
        
        for(int i=ind;i<inst.size();i++){
            if(inst[i]==','){//finding next element from the first comma upto the second comma
                found=1;
                ind=i+1;
                break;
            }
            temp=temp+inst[i];
        }
        
        if(!found){//second comma not found, so just appending the last element and returning
            el.push_back(temp);
            temp="";
            return el;
        }

        el.push_back(temp);//pushing back the element between the two commas to the vector
        temp="";
        
        el.push_back(inst.substr(ind,inst.size()-ind));//pushing back the element after the second comma to the vector
        return el;
    }

    string instructionToMachine(string inst){
        //converts the elements i.e. {"addi", "$2", "$3", "10"} to a 32-bit machineCode string
        vector<string> el=elements(inst);//extract the individual elements of the instruction
        string temp;
        
        if(el.size()>1){//instruction is not just a label
            
            if(el[0]=="add"||el[0]=="addi"||el[0]=="sub"||el[0]=="subi"||el[0]=="mul"){
                //R-type instruction
                temp=temp+opCode["rtype"];//bits 31:26 -- opCode for R-type instruction
                temp=temp+decToBinary(el[1]);//bits 25:21 -- first register index binary value
                temp=temp+decToBinary(el[2]);//bits 20:16 -- second register index binary value
                string se=el[3];//last value of R-type instruction maybe immediate value or another register
                if(se[0]=='$')se.erase(se.begin());//erase the '$' sign from the string if operand is a register
                int number=stoi(se);
                bitset<10> b(number);
                //Note that we've ignored shamt in our instructions, we've instead taken the 
                //corresponding 5 bits of shamt to increase the range of the immediate value
                temp=temp+b.to_string();
                temp=temp+funct[el[0]];//bits 5:0 -- funct 
            }
            
            else if(el[0]=="j"){//jump instruction
                temp=temp+opCode["j"];//bits 31:26 -- opCode for Jump instruction
                for(int i=0;i<21;i++)temp=temp+'0';//bits 25:5 -- waste '0' bits 
                int index=indicesOfLabels[el[1]];//getting the labelIndex of the label
                bitset<5> b(index);//converting the labelIndex to a 5-bit binary string
                temp=temp+b.to_string();//bits 4:0 -- labelIndex to jump to
            }
            
            else if(el[0]=="beq"){//branch instruction
                temp=temp+opCode["beq"];//bits 31:26 -- opCode for Branch instruction
                temp=temp+decToBinary(el[1]);//bits 25:21 -- first register index binary value
                temp=temp+decToBinary(el[2]);//bits 20:16 -- second register index binary value
                for(int i=0;i<11;i++)temp=temp+'0';//11 waste immediate bits for label
                int index=indicesOfLabels[el[3]];//getting the labelIndex of the label
                bitset<5> b(index);//converting the labelIndex to a 5-bit binary string
                temp=temp+b.to_string();//bits 4:0 -- labelIndex to branch to
            }
            
            else if(el[0]=="lw"||el[0]=="st"){//load/store instruction
                temp=temp+opCode[el[0]];//bits 31:26 -- opCode for Load/Store instruction
                temp=temp+decToBinary(el[1]);//bits 25:21 -- first register index binary value
                string mem=el[2];//memory element of instruction e.x. mem="%2"
                mem.erase(mem.begin());//"%2" --> "2"
                int x=stoi(mem);//converting string to corresponding integer value
                bitset<21> b(x);//creating a 21 bit binary representation of the integer
                temp=temp+b.to_string();//bits 20:0 -- memory index which is to be accessed
            }

        }
        
        else{//instruction is just a label
            if(el[0]=="*end"){
                for(int i=0;i<32;i++)temp=temp+'1';//we've set all bits to '1' to signify the end of the program
                return temp;
            }
            for(int i=0;i<27;i++)temp=temp+'0';//first 27 bits are '0' for a label instruction
            int index=indicesOfLabels[el[0]];//convert the line index of the first appearance of label to integer
            bitset<5> b(index);//converting the index of the label to binary
            temp=temp+b.to_string();//appending the binary string to the 27 '0' bits and returning the 32-bit instruction
        }

        return temp;
    }

    vector<int> whatIsRead(vector<string> el){
        vector<int> read;
        for(int i=0;i<el.size();i++){
            if(el[i][0]=='$'){
                el[i].erase(el[i].begin());
            }
        }
        string op;
        if(el[0]=="addi"||el[0]=="add"||el[0]=="subi"||el[0]=="sub"||el[0]=="mul")op="000001";
        if(el[0]=="lw")op="000010";
        if(el[0]=="st")op="000011";
        if(el[0]=="beq")op="000101";
        if(op=="000001"){
            read.push_back(stoi(el[2]));
            if(el[0]!="addi"&&el[0]!="subi"){
                read.push_back(stoi(el[3]));
            }
        }else if(op=="000101"){
            read.push_back(stoi(el[1]));
            read.push_back(stoi(el[2]));
        }else if(op=="000011"){
            read.push_back(stoi(el[1]));
        }
        return read;
    }

    int whereToWrite(vector<string> el){
        for(int i=0;i<el.size();i++){
            if(el[i][0]=='$'){
                el[i].erase(el[i].begin());
            }
        }
        string op;
        if(el[0]=="addi"||el[0]=="add"||el[0]=="subi"||el[0]=="sub"||el[0]=="mul")return stoi(el[1]);
        if(el[0]=="lw")return stoi(el[1]);
        return -1;
    }

    vector<string> generateMachineCode(){
        //takes assembly code input and returns the corresponding vector 
        //containing the machine code counterparts of the assembly code
        vector<string> instructions;
        string end="*end:";
        string s; 
        
        while(1){
            getline(cin,s);
            instructions.push_back(s);
            if(s==end)break;
        }

        //dealing with hazards by inserting waste instructions 
        for(int i=0;i<instructions.size();i++){
            vector<string> temp=elements(instructions[i]);
            int val=whereToWrite(temp);
            if(temp[0]!="*end" && temp.size()==1){
                string vec="addi $31,$0,0";
                instructions.insert(instructions.begin()+i+1,vec);
                instructions.insert(instructions.begin()+i+1,vec);
                instructions.insert(instructions.begin()+i+1,vec);
                continue;
            }
            vector<int> reading;
            if(i<instructions.size()-1){
                bool cond=0;
                reading=whatIsRead(elements(instructions[i+1]));
                for(auto x: reading){
                    if(x==val){
                        string vec="addi $31,$0,0";
                        instructions.insert(instructions.begin()+i+1,vec);
                        instructions.insert(instructions.begin()+i+1,vec);
                        instructions.insert(instructions.begin()+i+1,vec);
                        cond=1;
                        break;
                    }
                }
                if(cond)continue;
            }
            if(i<instructions.size()-2){
                bool cond=0;
                reading=whatIsRead(elements(instructions[i+2]));
                for(auto x: reading){
                    if(x==val){
                        string vec="addi $31,$0,0";
                        instructions.insert(instructions.begin()+i+1,vec);
                        instructions.insert(instructions.begin()+i+1,vec);
                        cond=1;
                        break;
                    }
                }
                if(cond)continue;
            }
            if(i<instructions.size()-3){
                bool cond=0;
                reading=whatIsRead(elements(instructions[i+3]));
                for(auto x: reading){
                    if(x==val){
                        string vec="addi $31,$0,0";
                        instructions.insert(instructions.begin()+i+1,vec);
                        cond=1;
                        break;
                    }
                }
                if(cond)continue;
            }
        }

        vec=instructions;
        //filling the appropriate line index for labels for jump/branch
        for(int i=0;i<instructions.size();i++){
            if(elements(instructions[i])[0]!="*end" && elements(instructions[i]).size()==1){//if instruction is just a label and it's not the *end label
                indicesOfLabels.insert({elements(instructions[i])[0],i});
            }
        }

        //converting the assembly code into machine code
        vector<string> machineCode;
        for(int i=0;i<instructions.size();i++){
            string temp=instructionToMachine(instructions[i]);
            machineCode.push_back(temp);
        }

        return machineCode;
    }

};

class Fetch{
	private:
		int PC=0;
		string Instruction;
        vector<string> MachineCode;
	public:
        Fetch(){}
        void setMachineCode(vector<string> x){
            MachineCode=x;
        }
		void setInstruction(vector<string> x){
            if(PC<=x.size()-1){
                if(BranchPossible==0){//in case of a branch, we don't fetch a new instruction until the zero flag is generated
                    Instruction=MachineCode[PC];
                    PC++;
                }
            } 
		}
		int getPC(){
			return PC;
		}	 

		void setPC(int k){
			PC=k;
		}

		string getOpcode(){
			return Instruction.substr(0,6);
		}

        string getIn(int m, int n){
            return Instruction.substr(m, n);
        }

        void giveToRegister(){
            //transferring the necessary information to the IF/ID global registers 
            if(EndFlag==0){
                if(BranchPossible==0){
                    if(JumpFlag==1){
                        //processing required for jump instruction
                        setPC(JA);
                        Instruction=MachineCode[PC];
                        PC++;
                        JumpFlag=0;
                    }
                    //storing the opCode and Instruction in the IF/ID registers
                    R_getOpCode=Instruction.substr(0,6);
                    R_Instruction=Instruction;
                }
                
                if(BranchPossible==1 && zeroFlagGenerated==1){
                    if(ZeroFlag==1){
                        //processing for a taken branch 
                        setPC(BA);
                        Instruction=MachineCode[PC];
                        PC++;
                        ZeroFlag=0;
                        BA=0;
                    }
                    R_getOpCode=Instruction.substr(0,6);
                    R_Instruction=Instruction;
                    zeroFlagGenerated=0;
                    BranchPossible=0;
                }
                else if(BranchPossible){
                    //zero flag not generated yet
                    R_getOpCode=Instruction.substr(0,6);
                    R_Instruction=Instruction;
                }
                BranchPossible=0;
                ZeroFlag=0;
            }
        }
};


class ControlUnit{
    private:
        string getOpCode, Instruction;
        bool DoSomething;

    public:
        ControlUnit(){
            DoSomething=0;
        }

        void takeFromRegister(){
            //takes the necessary data from the IF/ID registers
            if(!R_getOpCode.empty()){
                DoSomething=1;
                getOpCode=R_getOpCode;
                Instruction=R_Instruction;
                R_Instruction.clear();
                R_getOpCode.clear();
                //once taken, clearing the data of the IF/ID global registers
                if(Instruction.substr(0,6)=="000100"){//jump instruction
                    JumpFlag=1;
                    JA=stoi(Instruction.substr(6,26),0,2);
                } 
                if(Instruction.substr(0,6)=="000101"){//branch instruction
                    BranchPossible=1;
                    BA=stoi(Instruction.substr(16,16),0,2);
                }
                if(Instruction.substr(0,6)=="111111") EndFlag=1; //*end:
            }
        }

	    int RegDst(){
			if(getOpCode=="000010" || getOpCode=="000001") return stoi(Instruction.substr(6,5), 0, 2);//returns index of register to be written
            else return -1;
		}
		
		bool Branch(){
			if(getOpCode=="000101") return 1;
			else return 0;
		}
   
        // int BranchAddress(){
        //     return stoi(Instruction.substr(16,16),0,2);
        // } 
		
		bool MemRead(){
		   	if(getOpCode=="000010") return 1;
			else return 0;
		}
		
		bool MemtoReg(){
			if(getOpCode=="000010") return 1;
			else return 0;
		}
		
		bool MemWrite(){
			if(getOpCode=="000011") return 1;
			else return 0;
		}

        bool Jump(){
            if(getOpCode=="000100")return 1;
            else return 0;
        }

        // int JumpAddress(){
        //     return stoi(Instruction.substr(6,26),0,2);
        // }

        int ALUInput1(){
			if(getOpCode=="000010" || getOpCode=="000011") return 0;//load/store
            else return R[stoi(Instruction.substr(11, 5), 0, 2)];//second register of instruction orderwise
        }

		int ALUInput2(){
			if(getOpCode=="000001"){//R-type
                //Instruction.substr(26,6) is funct
                if(Instruction.substr(26, 6)=="000010" || Instruction.substr(26, 6)=="000100") return stoi(Instruction.substr(16, 10), 0, 2);//immediate R-type (modified)
                else return R[stoi(Instruction.substr(16, 10), 0, 2)];//register value for R-type, orderwise 3rd register
            }
            else if(getOpCode=="000101") return R[stoi(Instruction.substr(6, 5), 0, 2)];//branch, first register orderwise
            else return stoi(Instruction.substr(11, 21), 0, 2);//effective memory address for load/store
		}
		
		bool RegWrite(){
			if(getOpCode=="000010" || getOpCode=="000001") return 1;
			else return 0;
		}

        int dataForStore(){//which data to store for st
            return R[stoi(Instruction.substr(6, 5), 0, 2)];
        }
		
		int ALUControl(){
            //tells the ALU which operation to perform
            /*0: add
              1: sub
              2: mul*/
            if(getOpCode=="000101") return 1;//branch
            else if(getOpCode=="000010" || getOpCode=="000011") return 0;//load/store
            else{
                if(Instruction.substr(26, 6)=="000001" || Instruction.substr(26, 6)=="000010") return 0;//funct is for add/addi
                else if(Instruction.substr(26, 6)=="000011" || Instruction.substr(26, 6)=="000100") return 1;//funct is for sub/subi
                else return 2;//func is mul
            }
        }

		bool ALUDo(){
			if(getOpCode=="000100" || getOpCode=="000000" || getOpCode=="111111") return 0;//jump or label or end
			else return 1;
		}
		
		bool MemDo(){
			if(getOpCode=="000010" || getOpCode=="000011") return 1;//for load/store, memory unit will be active
			else return 0;
		}

		bool WritebackDo(){
			if(getOpCode=="000001" || getOpCode=="000010") return 1;//for R-type/load, writeBack unit will be active
			else return 0;
		}

        bool End(){
            if(getOpCode=="111111") return 1;
            else 0;
        }

        // bool getDoSomething(){
        //     return DoSomething;
        // }

        void giveToRegister(){
            if(DoSomething==1){
                //giving the ID/EX registers the required data
                R_RegDst=RegDst();
                R_Branch=Branch();
                // R_BranchAddress=BranchAddress();
                // R_BranchAddress=BA;
                R_MemRead=MemRead();
                R_MemtoReg=MemtoReg();
                R_MemWrite=MemWrite();
                R_Jump=Jump();
                // R_JumpAddress=JumpAddress();
                R_ALUInput1=ALUInput1();
                R_ALUInput2=ALUInput2();
                R_RegWrite=RegWrite();
                R_dataForStore=dataForStore();
                R_ALUControl=ALUControl();
                R_ALUDo=ALUDo();
                R_MemDo=MemDo();
                R_WritebackDo=WritebackDo();
                R_End=End();
                DoSomething=0;
            }
            
        }
};

class ALU{
  private:
    int ALUInput1;
    int ALUInput2;
    bool ALUdo;
    bool ALUBranch;
    bool ALUMemRead;
    bool ALUMemToReg; 
    bool ALUMemWrite;
    bool ALURegWrite;
    bool ALUMemDo;
    bool ALUWriteBackDo;
    int passDataForSt;
    int regAddress;
    int ALUControl;
    bool end;
  public:
    void takeFromRegister(){
        //take the necessary data from the ID/EX registers
        ALUdo = R_ALUDo;
        ALUInput1 = R_ALUInput1;
        ALUInput2 = R_ALUInput2;
        ALUBranch = R_Branch;
        ALUMemRead = R_MemRead;
        ALUWriteBackDo = R_WritebackDo;
        ALUMemDo = R_MemDo;
        ALURegWrite = R_RegWrite;
        ALUMemWrite = R_MemWrite;
        ALUMemToReg = R_MemtoReg;
        ALUMemRead = R_MemRead;
        passDataForSt = R_dataForStore;
        regAddress = R_RegDst;
        ALUControl=R_ALUControl;
        end=R_End;
        if(ALUdo){
            if(ALUInput1-ALUInput2==0 && ALUBranch){
                ZeroFlag = true;
            }
            if(ALUBranch){
                zeroFlagGenerated = true;
            }
            R_ALUDo = false;
        }
}

    void giveToRegister(){
        if(ALUControl==0 && ALUdo){ 
            R_ALUOutputReg =  ALUInput1+ALUInput2;//funct add
        }

        else if(ALUControl==1 && ALUdo){
            R_ALUOutputReg = ALUInput1-ALUInput2;//funct subtract
        }

        else if(ALUControl==2 && ALUdo){
            R_ALUOutputReg = ALUInput1*ALUInput2;//funct mul
        }
        
        R_memRead1 = ALUMemRead; 
        R_writeBackDo1 = ALUWriteBackDo;
        R_memDo1 = ALUMemDo;
        R_regWrite1 = ALURegWrite;
        R_memWrite1 = ALUMemWrite;
        R_memToReg1 = ALUMemToReg;
        R_memWriteData1 = passDataForSt;
        R_regDst1 = regAddress;
        R_end1 = end;
    }
};

class Memory{
  private:
    bool memRead;
    bool memWrite;
    bool memDo;
    bool regWrite; 
    bool writeBackDo;
    int ALUOutput;
    int dataToBeStored;
    int readData;
    int regDst;
    bool end;
  public:
    void takeFromRegister(){ 
        //takes the necessary data from EX/MEM registers
        memWrite = R_memWrite1;
        memDo = R_memDo1;
        ALUOutput = R_ALUOutputReg;
        dataToBeStored = R_memWriteData1;
        memRead = R_memRead1;
        writeBackDo = R_writeBackDo1;
        regWrite = R_regWrite1;
        regDst = R_regDst1;
        end=R_end1;
        if(memWrite==1 && memDo==1) { 
            MEM[ALUOutput] = dataToBeStored;
            R_MemDo=false;  
        }
    }

    void giveToRegister(){
        if(memRead==1 && memDo==1){
            readData = MEM[ALUOutput];
        }
        else if(memRead==0 && memDo==0){//by-pass
            readData = ALUOutput;
        }
        R_writeBackData = readData;//data to write
        R_writeBackDo2 = writeBackDo;//wb on/off state
        R_regDst2 = regDst;//where to write
        R_end2=end;
    }
};

class Writeback{
  private:
    int dataToBeWritten;
    int regDst;
    bool writeDo;
    bool end;
  public:
    void takeFromRegister(){
        //taking the necessary data from MEM/WB registers
        writeDo = R_writeBackDo2;
        regDst = R_regDst2;
        if(writeDo){
            dataToBeWritten = R_writeBackData;
            R_WritebackDo = false;
        }
        end=R_end2;
    }

    void writeBackToRegister(){
        if(writeDo==1){
            R[regDst] = dataToBeWritten;
        }
        finito=end;
    }
};

bool stall(Fetch* f){
    if(f->getIn(0,6)=="000101") return 1;
    return 0;
}

void print(){
    cout<<"\nValue of registers: ";
    for(int i=0;i<10;i++){
        cout<<R[i]<<" ";
    }
    cout<<"\n";
    cout<<"\nValue in memory: ";
    for(int i=0;i<10;i++){
        cout<<MEM[i]<<" ";
    }
    cout<<"\n\n";
    for(int i=0;i<69;i++)cout<<"-";
    cout<<"\n";
}

int main(){
    vector<string> v;
    Assembler* assembler = new Assembler();
    Fetch* f=new Fetch();
    v=assembler->generateMachineCode();
    f->setMachineCode(v);
    ControlUnit* c=new ControlUnit();
    ALU* alu=new ALU();
    Memory* mem=new Memory();
    Writeback* wb=new Writeback();

    int cnt=0, meaningfulInstructions=0;
    bool skip=0;
    while(!finito){
        if(!skip){
            f->setInstruction(v);
            if(f->getIn(0,32) != "00000111111000000000000000000010") meaningfulInstructions++;
        }

        c->takeFromRegister();
        alu->takeFromRegister();
        mem->takeFromRegister();
        wb->takeFromRegister();
        if(skip){
            skip=0;
            cnt++;
        }

        f->giveToRegister();
        c->giveToRegister();
        alu->giveToRegister();
        mem->giveToRegister();
        wb->writeBackToRegister();

        if(cnt<1)skip=stall(f);
        else cnt=0;
        
        Clock++;
        print();
    }
    cout<<"Clocks: "<<Clock<<"\n";
    cout<<"CPI : "<<setprecision(10)<<float(float(Clock)/meaningfulInstructions)<<"\n";
    return 0;
}