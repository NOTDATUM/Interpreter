#include <bits/stdc++.h>
using namespace std;
int pri[1000] = {0,};
set<char> op = {'+', '-', '*', '/', '(', ')', '!', '_', '%', '^'};
vector<char> vec;
map<char, int> ma;
map<char, int> mb;
int MAXINT = 2147483647;
int exp(int x, int y) {
	int result = 1;
	for(int i = 0; i<y; i++) {
		result *= x;
	}
	return result;
}
bool isNum(char c) {
	return 48<=(int)c && (int)c<=57 ? true : false;
}
int ToNum(char c) {
	return (int)c-48;
}
void Trans(string st);
int Calculate();
string Turn(string st);
int main() {
	ma['+'] = 2;
	ma['-'] = 2;
	ma['*'] = 3;
	ma['/'] = 3;
	ma['('] = 1;
	ma['%'] = 4;
	ma['^'] = 5;
	mb['+'] = -1;
	mb['-'] = -2;
	mb['*'] = -3;
	mb['/'] = -4;
	mb['%'] = -5;
	mb['^'] = -6;
	while(1) {
		string st;
		getline(cin, st);
		/*remove(st.begin(), st.end(), ' ');
		st.erase(0, st.length());*/
		st.erase(remove(st.begin(), st.end(), ' '), st.end());
		st = Turn(st);//cout<<st;
		Trans(st);
		/*for(int i = 0; i<1000; i++) {
			printf("%d ", pri[i]);
		}*/
		printf("%d\n", Calculate());
	}
	
}
string Turn(string st) {
	for(int i = 0; i<st.length(); i++) {
		//printf("!!");
		if(op.find(st[i])==op.end() || st[i]==')') continue;
		if(i!=0) i++;
		//printf("\\%d\n", i);
		while(op.find(st[i])!=op.end()) {
			char lasterased = st[i];
			//cout<<"|"<<st[i]<<"|";
			if(lasterased=='-') {
				st.erase(i, 1);
				st.insert(i, "_");
			}
			else if(lasterased=='+') {
				st.erase(i, 1);
				st.insert(i, "!");
			}
			i++;
		}
	}
	return st;
}
void Trans(string st) {
	int index = 0, cnt = 0, fl = 0;
	while(index<st.length()) {
		if(op.find(st[index])==op.end()) {
			if(!isNum(st[index])) {
				index++;
				continue;                                    
			}
			int result = 0;
			while(isNum(st[index])) {
				result = result*10+ToNum(st[index]); //10
				index++;
			}
			if(fl==-1) {
				pri[cnt] = 0;
				pri[cnt+1] = result;
				pri[cnt+2] = -2;
				cnt += 3;
			}
			else {
				pri[cnt] = result;
				cnt++;
			}
			fl = 0;
		}
		else {
			//printf("%d\n", index);
			if(st[index]=='!') {
				index++;
				continue;
			}
			else if(st[index]=='_') {
				if(fl==0) fl = -1;
				else fl *= -1;
			}
			else if(vec.size()==0) {
				vec.push_back(st[index]);
			}
			else if(st[index]==')') {
				char k = vec.back();
				vec.pop_back();
				while(k!='(') {
					pri[cnt] = mb[k];
					cnt++;
					k = vec.back();
					vec.pop_back();
				}
				
			}
			else if(st[index]=='(') {
				vec.push_back(st[index]);
			}
			else {
				//printf("|%d| %d\n", ma[vec.back()], fl);
				//printf("|%d|\n", ma[vec.back()]);
				while(ma[vec.back()]>=ma[st[index]]) {
					pri[cnt] = mb[vec.back()];
					vec.pop_back();
					cnt++;
				}
				vec.push_back(st[index]);
			}
			index++;
		}
	}
	while(!vec.empty()) {
		pri[cnt] = mb[vec.back()];
		cnt++;
		vec.pop_back();
	}
	pri[cnt] = -100;
	return;
}

int Calculate() {
	vector<int> cal;
	int i = 0;
	while(pri[i]!=-100) {
		if(pri[i]<0) {
			int x = cal.back();
			cal.pop_back();
			int y = cal.back();
			cal.pop_back();
			switch(pri[i]) {
				case -1: cal.push_back(y+x); break;
				case -2: cal.push_back(y-x); break;
				case -3: cal.push_back(y*x); break;
				case -4: cal.push_back(y/x); break;
				case -5: cal.push_back(y%x); break;
				case -6: cal.push_back(exp(y, x)); break;
				default: break;
			}
		}
		else {
			cal.push_back(pri[i]);
		}
		i++;
	}
	return cal.back();
}
