/*-------------------------------------------*/
/*    逆ポーランド記法の評価 polish_p.cpp    */
/*-------------------------------------------*/
#include <iostream>
#include <cstdlib>      // for exit()
#include <cctype>       // for is...()
using namespace std;

void polish(char *s);
int execute();
int getvalue(int ch);
int order(int ch);
void push(int n);
int pop();

#define STK_SIZ 20
int stack[STK_SIZ+1];                                    /* スタック */
int stkct;                                       /* スタックポインタ */
char plsh_out[80];                             /* 逆ポーランド出力先 */

int main()
{
    char siki[80];
    int ans;

    cout << "入力: "; cin.getline(siki, 80);          /* 1行読み込み */
    polish(siki);
    if (plsh_out[0] == '\0') exit(1);              /* 有効な式がない */
    ans = execute();
    cout << "変換: " << plsh_out << endl;
    cout << "結果: " << ans << endl;

    return 0;
}

void polish(char *s)
{
    int wk;
    char *out = plsh_out;

    stkct = 0;
    for (;;) {
        while (isspace(*s)) { ++s; }           /* 空白スキップ       */
        if (*s == '\0') {                      /* 末尾になった       */
            while (stkct > 0) {                /* スタック残りを出力 */
                if ((*out++ = pop()) == '(') {
                    puts("'('が不正\n"); exit(1);
                }
            }
            break;
        }
        if (islower(*s) || isdigit(*s)) {       /* 変数か数字なら出力*/
            *out++ = *s++; continue;
        }
        switch (*s) {
        case '(':                              /* ( なら積む         */
            push(*s);
            break;
        case ')':                              /* ) なら             */
            while ((wk = pop()) != '(') {      /* ( に会うまで出力   */
                *out++ = wk;
                if (stkct == 0) { puts("'('が不足\n"); exit(1); }
            }
            break;
        default:                               /* 演算子             */
            if (order(*s) == -1) {             /* 利用不適文字       */
                cout << "不正な文字: " << *s << endl; exit(1);
            }            /* 自分以上強い要素がスタックトップにある間 */
            while (stkct>0 && (order(stack[stkct]) >= order(*s))) {
                *out++ = pop();                /* スタック内容を出力 */
            }
            push(*s);                          /* 現要素を積む       */
        }
        s++;
    }
    *out = '\0';
}

int execute()                                            /* 式の計算 */
{
    int d1, d2;
    char *s;

    stkct = 0;
    for (s=plsh_out; *s; s++) {
        if (islower(*s))                       /* 変数なら           */
            push(getvalue(*s));                /* 値をスタックに積む */
        else if (isdigit(*s))                  /* 数字なら           */
            push(*s - '0');                    /* 値をスタックに積む */
        else {                                 /* 演算子なら         */
            d2 = pop(); d1 = pop();            /* 2つの値を取り出し  */
            switch (*s) {
            case '+': push(d1+d2); break;      /* 計算結果を積む     */
            case '-': push(d1-d2); break;
            case '*': push(d1*d2); break;
            case '/': if (d2 == 0) { puts("ゼロ除算"); exit(1); }
                      push(d1/d2); break;
            }
        }
    }
    if (stkct != 1) { cout << "error\n"; exit(1); }
    return pop();                                      /* 結果を返す */
}

int getvalue(int ch)                                  /* 1～26を返す */
{
    if (islower(ch)) return ch-'a' + 1;
    return 0;
}

int order(int ch)                                        /* 優先順位 */
{
    switch (ch) {
    case '*': case '/': return 3;
    case '+': case '-': return 2;
    case '(':           return 1;
    }
    return -1;
}

void push(int n)                                     /* スタック積込 */
{
    if (stkct+1 > STK_SIZ) { puts("stack overflow"); exit(1); }
    stack[++stkct] = n;
}

int pop()                                            /* スタック取出 */
{
    if (stkct < 1) { puts("stack underflow"); exit(1); }
    return stack[stkct--];
}
