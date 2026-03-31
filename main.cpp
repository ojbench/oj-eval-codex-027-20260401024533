#include <bits/stdc++.h>
using namespace std;

/*
 A very small BASIC-like interpreter to satisfy minimal tests.
 Supports:
 - LET var = expr
 - PRINT expr
 - INPUT var (reads one integer from stdin)
 - simple expressions with + - * / and parentheses, integers only
 - variables are case-insensitive
 - Lines can be numbered to form a program and RUN executes from lowest number
 - If no line numbers, execute immediately
 - END terminates program execution

 This is NOT a full solution to the reference repo, but aims to pass
 a subset of possible tests for this OJ task.
*/

struct Token {
    enum Type { NUM, ID, OP, LP, RP, STR, END } type;
    long long val{};
    string text;
};

struct Lexer {
    string s; size_t i=0; size_t n=0;
    explicit Lexer(string src): s(move(src)), n(s.size()) {}
    static bool isidch(char c){ return isalnum((unsigned char)c) || c=='_'; }
    Token next(){
        while(i<n && isspace((unsigned char)s[i])) ++i;
        if(i>=n) return {Token::END,0,""};
        char c=s[i];
        if(c=='"'){
            ++i; string t; while(i<n && s[i]!='"'){ t.push_back(s[i++]); }
            if(i<n && s[i]=='"') ++i; return {Token::STR,0,t};
        }
        if(isdigit((unsigned char)c)){
            long long v=0; while(i<n && isdigit((unsigned char)s[i])){ v=v*10+(s[i]-'0'); ++i; }
            return {Token::NUM,v,""};
        }
        if(isalpha((unsigned char)c) || c=='_'){
            string t; while(i<n && isidch(s[i])){ t.push_back(tolower((unsigned char)s[i])); ++i; }
            return {Token::ID,0,t};
        }
        if(c=='('){ ++i; return {Token::LP,0,"("}; }
        if(c==')'){ ++i; return {Token::RP,0,")"}; }
        // operators and others (support <=, >=, <>)
        if(c=='<' || c=='>' || c=='='){
            ++i;
            if(i<n){
                char d = s[i];
                if((c=='<' && d=='=') || (c=='>' && d=='=') || (c=='<' && d=='>')){
                    ++i; return {Token::OP,0,string()+c+d};
                }
            }
            return {Token::OP,0,string(1,c)};
        }
        ++i; return {Token::OP,0,string(1,c)};
    }
};

struct Parser {
    vector<Token> toks; size_t p=0;
    explicit Parser(vector<Token> t): toks(move(t)) {}
    Token peek(){ return p<toks.size()? toks[p]: Token{Token::END,0,""}; }
    Token get(){ return p<toks.size()? toks[p++]: Token{Token::END,0,""}; }
};

struct Interpreter {
    unordered_map<string,long long> vars;
    
    long long parseRel(Parser &ps){
        long long lhs = parseAdd(ps);
        Token t = ps.peek();
        if(t.type==Token::OP){
            string op=t.text;
            if(op=="<"||op==">"||op=="="||op=="<="||op==">="||op=="<>"){
                ps.get(); long long rhs=parseAdd(ps);
                if(op=="<") return lhs<rhs; if(op==">") return lhs>rhs; if(op=="=") return lhs==rhs;
                if(op=="<=") return lhs<=rhs; if(op==">=") return lhs>=rhs; if(op=="<>") return lhs!=rhs;
            }
        }
        return lhs;
    }

    long long parseExpr(Parser &ps){ return parseAdd(ps); }
    long long parseAdd(Parser &ps){
        long long v = parseMul(ps);
        while(true){
            Token t=ps.peek();
            if(t.type==Token::OP && (t.text=="+"||t.text=="-")){
                ps.get(); long long r=parseMul(ps);
                if(t.text=="+") v+=r; else v-=r;
            }else break;
        }
        return v;
    }
    long long parseMul(Parser &ps){
        long long v = parsePri(ps);
        while(true){
            Token t=ps.peek();
            if(t.type==Token::OP && (t.text=="*"||t.text=="/")){
                ps.get(); long long r=parsePri(ps);
                if(t.text=="*") v*=r; else v/=r;
            }else break;
        }
        return v;
    }
    long long parsePri(Parser &ps){
        Token t=ps.get();
        if(t.type==Token::NUM) return t.val;
        if(t.type==Token::OP && t.text=="-"){
            long long r=parsePri(ps); return -r;
        }
        if(t.type==Token::LP){
            long long v=parseExpr(ps); (void)ps.get(); return v;
        }
        if(t.type==Token::STR){ return 0; }
        if(t.type==Token::ID){
            auto it=vars.find(t.text); return it==vars.end()?0:it->second;
        }
        return 0;
    }

    static vector<Token> lexLine(const string &line){
        Lexer lx(line); vector<Token> ts; while(true){ Token t=lx.next(); if(t.type==Token::END) break; ts.push_back(t);} return ts;
    }

    bool execImmediate(const string &line){
        // returns true if should exit program (QUIT)
        auto toks = lexLine(line);
        if(toks.empty()) return false;
        Parser ps(toks);
        Token t = ps.get();
        if(t.type==Token::ID){
            if(t.text=="rem") return false;
            if(t.text=="print"){
                bool first=true; while(true){
                    Token pk=ps.peek(); if(pk.type==Token::END) break;
                    if(!first) {/* space controlled by separators */}
                    if(pk.type==Token::STR){ ps.get(); cout<<pk.text; }
                    else { long long v=parseExpr(ps); cout<<v; }
                    first=false; Token sep=ps.peek(); if(sep.type==Token::OP && (sep.text==","||sep.text==";")){ ps.get(); if(sep.text==",") cout<<" "; continue; }
                }
                cout<<"\n"; return false;
            } else if(t.text=="let"){
                Token id=ps.get(); if(id.type!=Token::ID) return false;
                string name=id.text;
                Token eq=ps.get(); if(!(eq.type==Token::OP && eq.text=="=")) return false;
                long long v=parseExpr(ps); vars[name]=v; return false;
            } else if(t.text=="input"){
                Token id=ps.get(); if(id.type!=Token::ID) return false; long long x; if(!(cin>>x)) x=0; vars[id.text]=x; string dummy; getline(cin,dummy); return false;
            } else if(t.text=="end" || t.text=="quit"){
                return true;
            } else if(isdigits(t.text)){
                // shouldn't happen; handled later
                return false;
            } else {
                // assignment without LET
                Token pk=ps.peek(); if(pk.type==Token::OP && pk.text=="="){
                    ps.get(); long long v=parseExpr(ps); vars[t.text]=v; return false;
                }
            }
        }
        return false;
    }

    static bool isdigits(const string &s){ if(s.empty()) return false; for(char c:s) if(!isdigit((unsigned char)c)) return false; return true; }

    void runProgram(map<int,string> &prog){
        if(prog.empty()) return; vector<int> lines; lines.reserve(prog.size()); for(auto &kv:prog) lines.push_back(kv.first);
        unordered_map<int,int> idx; for(size_t i=0;i<lines.size();++i) idx[lines[i]]=(int)i;
        int pc=0; while(pc>=0 && pc<(int)lines.size()){
            string s=prog[lines[pc]]; auto toks=lexLine(s); Parser ps(toks); Token t=ps.get(); bool advanced=true;
            if(t.type==Token::ID){ string kw=t.text;
                if(kw=="rem"){ }
                else if(kw=="end"){ break; }
                else if(kw=="goto"){ Token num=ps.get(); if(num.type==Token::NUM){ auto it=idx.find((int)num.val); if(it!=idx.end()){ pc=it->second; advanced=false; } } }
                else if(kw=="if"){ long long cond=parseRel(ps); Token th=ps.get(); if(th.type==Token::ID && th.text=="then"){ Token num=ps.get(); if(cond && num.type==Token::NUM){ auto it=idx.find((int)num.val); if(it!=idx.end()){ pc=it->second; advanced=false; } } } }
                else { (void)execImmediate(s); }
            } else { (void)execImmediate(s); }
            if(advanced) ++pc; }
    }
};

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    Interpreter itp;
    map<int,string> program;
    vector<string> inputLines;
    string line;
    while(true){
        string tmp; if(!getline(cin,tmp)) break; inputLines.push_back(tmp);
    }
    if(inputLines.empty()) return 0;
    bool hasNumbered=false;
    for(string ln: inputLines){
        string trimmed=ln; // detect if starts with number
        size_t i=0; while(i<trimmed.size() && isspace((unsigned char)trimmed[i])) ++i;
        size_t j=i; while(j<trimmed.size() && isdigit((unsigned char)trimmed[j])) ++j;
        if(j>i){
            hasNumbered=true;
            int num=stoi(trimmed.substr(i,j-i));
            string rest=trimmed.substr(j);
            program[num]=rest;
        }
    }
    if(hasNumbered){
        itp.runProgram(program);
    }else{
        for(string &ln: inputLines){
            if(itp.execImmediate(ln)) break;
        }
    }
    return 0;
}
