/*-----------------------------------*/
/*    電卓プログラム minicalc.cpp    */
/*-----------------------------------*/
#include <iostream>
#include <cstdlib>      // for exit()
#include <cctype>       // for is...()
using namespace std;

enum TknKind {                             /* トークンの種類 */
    Print, Lparen, Rparen, Plus, Minus, Multi, Divi,
    Assign, VarName, IntNum, EofTkn, Others
};

struct Token {
    TknKind kind;                          /* トークンの種類   */
    int  intVal;                           /* 定数値や変数番号 */
    Token ()           { kind = Others; intVal = 0; }
    Token (TknKind k, int d=0) { kind = k; intVal = d; }
};

void input();
void statement();
void expression();
void term();
void factor();
Token nextTkn();
int nextCh();
void operate(TknKind op);
void push(int n);
int pop();
void chkTkn(TknKind kd);

#define STK_SIZ 20                         /* スタックサイズ */
int stack[STK_SIZ+1];                      /* スタック       */
int stkct;                                 /* スタック管理   */
Token token;                               /* トークン格納   */
char buf[80], *bufp;                       /* 入力用         */
int ch;                                    /* 取得文字を格納 */
int var[26];                               /* 変数a-z        */
int errF;                                  /* エラー発生     */

int main()
{
    while (1) {
        input();                           /* 入力   */
        token = nextTkn();                 /* 最初のトークン */
        if (token.kind == EofTkn) exit(1); /* 終了   */
        statement();                       /* 文実行 */
        if (errF) cout << "  --err--\n";
    }
    return 0;
}

void input()
{
    errF = 0; stkct = 0;                   /* 初期設定           */
    cin.getline(buf, 80);                  /* 80文字以内の入力   */
    bufp = buf;                            /* 先頭文字に位置づけ */
    ch = nextCh();                         /* 最初の文字取得     */
}

void statement()                           /* 文 */
{
    int vNbr;

    switch (token.kind) {
    case VarName:                          /* 代入文     */
        vNbr = token.intVal;               /* 代入先保存 */
        token = nextTkn();
        chkTkn(Assign); if (errF) break;   /* '=' のはず */
        token = nextTkn();
        expression();                      /* 右辺計算   */
        var[vNbr] = pop();                 /* 代入実行   */
        break;
    case Print:                            /* print文:?  */
        token = nextTkn();
        expression();
        chkTkn(EofTkn); if (errF) break;
        cout << "  " << pop() << endl;
        return;
    default:
        errF = 1;
    }
    chkTkn(EofTkn);                        /* 行末チェック */
}

void expression()                          /* 式 */
{
    TknKind op;

    term();
    while (token.kind==Plus || token.kind==Minus) {
        op = token.kind;
        token = nextTkn(); term(); operate(op);
    }
}

void term()                                /* 項 */
{
    TknKind op;

    factor();
    while (token.kind==Multi || token.kind==Divi) {
        op = token.kind;
        token = nextTkn(); factor(); operate(op);
    }
}

void factor()                              /* 因子     */
{
    switch (token.kind) {
    case VarName:                          /* 変数     */
        push(var[token.intVal]);
        break;
    case IntNum:                           /* 整数定数 */
        push(token.intVal);
        break;
    case Lparen:                           /* ( 式 )   */
        token = nextTkn();
        expression();
        chkTkn(Rparen);                    /* ) のはず */
        break;
    default:
        errF = 1;
    }
    token = nextTkn();
}

Token nextTkn()                            /* 次トークン */
{
    TknKind kd = Others;
    int  num;

    while (isspace(ch))                    /* 空白読み捨て */
        ch = nextCh();
    if (isdigit(ch)) {                     /* 数字 */
        for (num=0; isdigit(ch); ch = nextCh())
           num = num*10 + (ch-'0');
        return Token(IntNum, num);
    }
    else if (islower(ch)) {                /* 変数 */
        num = ch - 'a';                    /* 変数番号0-25 */
        ch = nextCh();
        return Token(VarName, num);
    }
    else {
        switch (ch) {
        case '(':  kd = Lparen; break;
        case ')':  kd = Rparen; break;
        case '+':  kd = Plus;   break;
        case '-':  kd = Minus;  break;
        case '*':  kd = Multi;  break;
        case '/':  kd = Divi;   break;
        case '=':  kd = Assign; break;
        case '?':  kd = Print;  break;
        case '\0': kd = EofTkn; break;       // 【: これを入れた】
        }
        ch = nextCh();
        return Token(kd);
    }
}

int nextCh()                               /* 次の1文字 */
{
    if (*bufp == '\0') return '\0'; else return *bufp++;
}

void operate(TknKind op)                   /* 演算実行 */
{
    int d2 = pop(), d1 = pop();

    if (op==Divi && d2==0) { cout << "  division by 0\n"; errF = 1; }
    if (errF) return;
    switch (op) {
    case Plus:  push(d1+d2); break;
    case Minus: push(d1-d2); break;
    case Multi: push(d1*d2); break;
    case Divi:  push(d1/d2); break;
    }
}

void push(int n)                           /* スタック積込 */
{
    if (errF) return;
    if (stkct+1 > STK_SIZ) { cout << "stack overflow\n"; exit(1); }
    stack[++stkct] = n;
}

int pop()                                  /* スタック取出 */
{
    if (errF) return 1;                    /* エラー時は単に1を返す */
    if (stkct < 1) { cout << "stack underflow\n"; exit(1); }
    return stack[stkct--];
}

void chkTkn(TknKind kd)                    /* トークン種別確認 */
{
    if (token.kind != kd) errF = 1;        /* 不一致 */
}
