#include <string>
#include <iostream>
#include <vector>
#include <memory> // make unique
#include <map>  


// Forward declare the ExprAST class
/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double Val) : Val(Val) {}
};


// definition for possible language token types
enum Token {
    tok_eof = -1, 
    tok_def = -2, 
    tok_extern = -3, 
    tok_identifier = -4,
    tok_number = -5
};
// any other symbol would just return the ascii code of the character that is entered

// unideal practice of global variables, necessary for this
static std::string IdentifierStr; // any strings or doubles will be saved here
static double NumVal;

static int gettok(){
    // while there is a white space following, then the program hasn't been fully read in
    // continue reading the tokens
    static int LastChar =  ' ';

    while (isspace(LastChar)){
        LastChar = getchar();
    }

    // the  segement that gets the identifier of the program
    // recognize standard identiifier: [a-z,A-Z][] first character is a letter, following ones are any character or number
    if (isalpha(LastChar)){
        IdentifierStr = LastChar; // coerces/casts the last char from int to string
        // C++ interprets the number as a string, Javascript takes the string representation of the num
        while(isalnum((LastChar = getchar()))){
            IdentifierStr += LastChar;
        }

        if(IdentifierStr == "def"){
            return tok_def;
        }

        if(IdentifierStr == "extern"){
            return tok_extern;
        }
        return tok_identifier;
    }

    // if there a digit, keep collecting the characters until the token is formed
    // fault here, if you get a digit and then a dot an then another digit and then another dot
    // or if you get a dot before getting a digit
    if(isdigit(LastChar) || LastChar == '.'){
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    // "#" represents comments
    // ignore all chars until the end of the line
    if(LastChar == '#'){
        do{
            LastChar = getchar();
        }
        while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        // we recursively call ourself because we are trying to find the end of the lien
        if(LastChar != EOF){
            return gettok();
        }
    }

    if(LastChar == EOF){
        return tok_eof;
    }

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar; // [][] not sure the point of this?
}

// int main(){
//     while(true){
//         int tok = gettok();
//         std::cout << "got token: " << tok << std::endl;
//     }

// }


/*
 * Parser Phase
 */

// all AST node classes will inherit from base parent AST node class

class VariableExprAST : public ExprAST {
    std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name(Name){}
};

class BinaryExprAST : public ExprAST {
    char Op; // operators: + - * / < >
    std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(
            char Op, 
            std::unique_ptr<ExprAST> LHS,
            std::unique_ptr<ExprAST> RHS
            // move operation is transfering ownership of LHS from outside this class to inside this class
        ) : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST> > Args;

public:
  CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST> > Args)
    : Callee(Callee), Args(std::move(Args)) {}
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
    : Name(Name), Args(std::move(Args)) {}

  const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

// recursive decsent parser
// syntax or rdp: symbol points to the 
static int CurTok; // save the current token to make it avaliable to the parser

static int getNextToken(){
    return CurTok = gettok();
}

std::unique_ptr<ExprAST> LogError(const char *Str){
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}

// log parsering string specific error
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str){
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseNumberExpr(){
    auto Result = std::make_unique<NumberExprAST>(NumVal); //create new unique ptr 
    getNextToken(); // expects the token to have already been parsed and seen taht it is a number
    return std::move(Result);
    // move the ownership of the ownership out of the smart poiter out of the function
    // so that ownership falls on who calls the ParseNumberExpr function
}

static std::unique_ptr<ExprAST> ParseParenExpr(){
    getNextToken(); // eat '('
    // 'eat' refers to changing the value of the current token to the next
    // functional parsing takes a different approach to it
    auto V = ParseExpression();
    if (!V) {
        return nullptr;
    }

    if(CurTok == ')'){
        getNextToken(); // eat )
        return V;
    }
    else{
        return LogError("expected ')'");
        return nullptr;
    }
}

static std::unique_ptr<ExprAST> ParseIdentifierorCallExpr(){
    std::string IdName = IdentifierStr;

    getNextToken(); // eat identifier

    // if there is a '(' after an identifier then there is a function call
    // [][] TODO difference between function argument and parameter?
    if(CurTok == '('){ 
        getNextToken(); // eat ()
        std::vector<std::unique_ptr<ExprAST> > Args;
        while(true){
            auto Arg = ParseExpression();
            if(Arg) {
                Args.push_back(Arg);
            } else{
                return nullptr;
            }

            if(CurTok == ')'){
                getNextToken(); // eat )
                break;
            } else if (CurTok == ','){
                getNextToken(); // eat ,
                continue;
            }
            else{
                return LogError("Expected ')' or ',' in argument list");
            }
        }
        return std::make_unique<CallExprAST>(IdName, std::move(Args));
    }
    else{
        return std::make_unique<VariableExprAST>(IdName);
    }
}

// primary
// identifer expression, number epxression, a expression enclosed in parenthesis
// binary expression (a single operator)

static std:: unique_ptr<ExprAST> ParserPrimary(){
    switch(CurTok){
        case tok_identifier:
            return ParseIdentifierorCallExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        default:
            return LogError("unknown token when expecting an expresion");
    }
}

// binary expression parsing
// ex. a + b * c can look at the expression as: 
// operator precedence: a + ( b * c )
// or non precedence: ( a + b ) * c

// this parser uses shunting yard algorithm for operater precedence 
// shunting yard: convert infix expression to postfix using stacks for operator

// static std::map<char, int> BinopPrecedence;
// get the precedence of the current binary operator token
// static int GetTokPrecedence(){
//     if(!isascii(CurTok)){
//         return -1;
//     }

//     // make sure that it's a valid declared binary operator 
//     int TokPrec = BinopPrecedence[CurTok];
//     if (TokPrec <= 0){
//         return -1;
//     }
//     return TokPrec;
//     // define precedence through switch statement (can use map)

//     switch(CurTok){
//         case '<':
//         case '>':
//             return 10;
//         case '+':
//         case '-':  
//             return 20;
//         case '*':
//         case '/':
//             return 40;
//         default:
//             return 1;
//     }
// }


static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;  
  BinopPrecedence['/'] = 40;  
}


static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);


static std::unique_ptr<ExprAST> ParseExpression(){
    auto LHS = ParserPrimary();
    if(LHS){
        return ParseBinOpRHS(0, std::move(LHS));
    }

    return nullptr;
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(
    int ExprPrec,
    std::unique_ptr<ExprAST> LHS
    ) {
    // if this is a binary opreator, get it's precedence first
    while(true){
        int TokPrec = GetTokPrecedence();
        if (TokPrec < ExprPrec){
            return LHS;
        }  
        // if it doesn't return, then we know that the next token is the targeted token
        else {
            int BinOp = CurTok; 
            getNextToken(); // eat binop
            auto RHS = ParserPrimary();
            // look ahead again to the right in case the next operater has even higher precedence
            // ex. a + b * c
            // if a and b are already parsed, the operator after b needs to be examined before executing expression
            if(RHS) {
                int NextPrec = GetTokPrecedence();
                if(TokPrec < NextPrec) {
                    RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
                    if(!RHS){
                        return nullptr;
                    }
                }
                // [][] TODO here?
                LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS),std::move(RHS));
                // if the precendence on the the right is greater, then current left and right hand side are grouped together
                // and they become the new left hand side
            } else{
               return nullptr;
            }
        }
    }
}


// parsing the prototype of the function (function signature)
//      ex. foobar(n, m)
//      identifier(param, param...)
static std::unique_ptr<PrototypeAST> ParsePrototype(){
    if(CurTok != tok_identifier) {
        return LogErrorP("Expected function name in prototype");
    }
    std::string FnName = IdentifierStr;
    getNextToken(); // eat the identifier
    // next token should be openparen, after the function name
    if(CurTok != '('){
        return LogErrorP("Expected '(' in prototype");
    }

    std::vector<std::string> ArgNames;
    while(getNextToken() == tok_identifier){
        ArgNames.push_back(IdentifierStr);
    }   
    if(CurTok != ')'){
        return LogErrorP("Expected ')' in prototype");
    }

    getNextToken();

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

// parse the definition of the function
static std::unique_ptr<FunctionAST> ParseDefinition(){
    getNextToken(); // eat def
    auto Proto = ParsePrototype();
    if(!Proto){
        return nullptr;
    }

    auto E = ParseExpression();
    if(E){
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    } else{
        return nullptr;
    }
}

// parse an extern (function that is definined externally)
// parse a top level expression
static std::unique_ptr<PrototypeAST> ParseExtern(){
    getNextToken(); // eat extern
    return ParsePrototype();
}

// parse top level expressions (evaluated on the fly)
// using anon nullary (zero argument) functions 
static std::unique_ptr<FunctionAST> ParseTopLevelExpr(){
    auto E = ParseExpression();
    if(E) {
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}




// top level parsing
static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed an extern\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

// the driver, invoke all parsing pieces with top level dispatch loop

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}