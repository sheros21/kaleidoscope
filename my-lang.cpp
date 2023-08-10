#include <string>
#include <iostream>

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

int main(){
    while(true){
        int tok = gettok();
        std::cout << "got token: " << tok << std::endl;
    }

}

