#include <bits/stdc++.h>
using namespace std;
ifstream fin;
vector<int> endfind, parenendfind;
enum TknKind { // 종류
	Lparen = 0, Rparen, Plus, Minus, Multiple, Divide = 5,
	Percent, Exp, Letter, Comma, If = 10,
	Else, Elif, For, While, Number = 15, 
	Quot, Semi, Var, Equal, Lmparen = 20,
	Rmparen, String, Typeint, Typefloat, Typestring=25,
	Typebool, Others
}; 
struct KeyW { 
	const char *keyName;
	TknKind keyKind;
};
struct Token {
	TknKind tk;
	string txt = "";
	int intVal = 0;
	int endnum = 0;
};
Token buffer[1000];
TknKind a[256];
KeyW Tble[9] = {{"if", If}, {"for", For}, {"while", While}, {"elif", Elif}, {"else", Else},
{"int", Typeint}, {"float", Typefloat}, {"bool", Typebool}, {"string", Typestring}}; // 인식방법(토큰)
void initTkn() { // 
	for(int i = 0; i<256; i++) a[i] = Others;
	for(int i = 'a'; i<='z'; i++) a[i] = Letter;
	for(int i = 'A'; i<='Z'; i++) a[i] = Letter;
	a['_'] = Letter;
	for(int i = '0'; i<='9'; i++) a[i] = Number;
	a['"'] = Quot; a['('] = Lparen; a[')'] = Rparen; a['+'] = Plus; a['-'] = Minus;
	a['*'] = Multiple; a['/'] = Divide; a['%'] = Percent; a['^'] = Exp;
	a[','] = Comma; a[';'] = Semi; a['='] = Equal;  a['{'] = Lmparen; a['}'] = Rmparen;
}
int main(int argc, char *argv[]) {
	fin.open("Source.txt"); if(!fin) exit(1);
	int cnt = 0;
	initTkn();
	char c = fin.get();
	while(true) {
		if(c==EOF) break;
		if(c==' ' || c=='\n') {
			c = fin.get();
			continue;
		}
		if(a[c]==Letter) {
			string intxt = "";
			while(a[c]==Letter) {
				intxt += c;
				c = fin.get();
			}
			if(a[c]==Number) return 0;
			TknKind t = Var;
			for(int i = 0; i<9; i++) {
				if(intxt==(string)Tble[i].keyName) {
					t = Tble[i].keyKind;
					break;
				}
			}
			Token cur;
			cur.tk = t;
			cur.txt = intxt;
			buffer[cnt] = cur;
			cnt++;
		}
		else if(a[c]==Number) {
			string intxt = "";
			int val = 0;
			while(a[c]==Number) {
				intxt += c;
				val = val*10+c-48;
				c = fin.get();
			}
			if(a[c]==Letter) return 0;
			TknKind t = Number;
			Token cur;
			cur.tk = t;
			cur.txt = intxt;
			cur.intVal = val;
			buffer[cnt] = cur;
			cnt++;
		}
		else {
			if(c=='(') parenendfind.push_back(cnt);
			if(c==')') {
				int last = parenendfind.back();
				parenendfind.pop_back();
				buffer[last].endnum = cnt;
			}
			if(c=='{') endfind.push_back(cnt);
			if(c=='}') {
				int last = endfind.back();
				endfind.pop_back();
				buffer[last].endnum = cnt;
			}
			if(c=='"') {
				Token cur;
				cur.tk = Quot;
				cur.txt = c;
				buffer[cnt] = cur;
				buffer[cnt+2] = cur;
				cnt++;
				string strtxt = "";
				c = fin.get();
				while(c!='"') {
					strtxt+=c;
					c = fin.get();
				}
				cur.tk = String;
				cur.txt = strtxt;
				buffer[cnt] = cur; cnt+=2;
				c = fin.get();
				continue;
			}
			Token cur;
			cur.tk = a[c];
			cur.txt = c;
			buffer[cnt] = cur;
			cnt++;
			c = fin.get();
		}
	}
	for(int i = 0; i<1000; i++) {
		if(buffer[i].txt=="") break;
		cout<<buffer[i].tk<<" "<<buffer[i].intVal<<" "<<buffer[i].txt<<"\n";
	}
}
