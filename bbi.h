/**********************************************************/
/*    filename:bbi.h 共通ヘッダ                           */
/**********************************************************/
#include <iostream>
#include <fstream>    // ファイル処理用
#include <sstream>    // 文字列ストリーム
#include <string>
#include <vector>
#include <stack>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>

using namespace std;

/* -------------------- define */
#define SHORT_SIZ  sizeof(short int)                  /* short int 型のサイズ */
#define SHORT_P(p) (short int *)(p)              /* short int型ポインタに変換 */
#define UCHAR_P(p) (unsigned char *)(p)      /* unsigned char型ポインタに変換 */
#define LIN_SIZ 255                                  /* ソースの1行最大サイズ */

/* -------------------- enum struct etc */
enum TknKind {                                                /* トークン要素 */
  Lparen='(', Rparen=')', Lbracket='[', Rbracket=']', Plus='+', Minus='-',
  Multi='*',  Divi='/',   Mod='%',      Not='!',      Ifsub='?',  Assign='=',
  IntDivi='\\', Comma=',',     DblQ='"',
  Func=150, Var,   If,     Elif,   Else,   For, To, Step,  While,
  End,      Break, Return, Option, Print,  Println, Input, Toint,
  Exit,     Equal, NotEq,  Less,   LessEq, Great,   GreatEq, And, Or,
  END_KeyList,
  Ident,      IntNum, DblNum, String,   Letter, Doll, Digit,
  Gvar, Lvar, Fcall,  EofProg, EofLine, Others
};

struct Token {                /* トークンの管理     */
  TknKind kind;               /* トークンの種類     */
  string  text;               /* トークン文字列     */
  double  dblVal;             /* 数値定数のときの値 */
  Token() {  kind=Others; text=""; dblVal=0.0; }
  Token (TknKind k)           { kind=k; text=""; dblVal=0.0; }
  Token (TknKind k, double d) { kind=k; text=""; dblVal=d; }
  Token (TknKind k, const string& s) { kind=k; text=s; dblVal=0.0; }
  Token (TknKind k, const string& s, double d) { kind=k; text=s; dblVal=d; }
};

enum SymKind { noId, varId, fncId, paraId };  /* 記号表登録名の種類 */
enum DtType  { NON_T, DBL_T };                /* 型名 */

struct SymTbl {               /* 記号表構成           */
  string  name;               /* 変数や関数の名前     */
  SymKind nmKind;             /* 種類                 */
  char    dtTyp;              /* 変数型(NON_T,DBL_T)  */
  int     aryLen;             /* 配列長。0:単純変数   */
  short   args;               /* 関数の引数個数       */
  int     adrs;               /* 変数,関数の番地      */
  int     frame;              /* 関数用フレームサイズ */
  SymTbl() { clear(); }
  void clear() {
    name=""; nmKind=noId; dtTyp=NON_T;
    aryLen=0; args=0; adrs=0; frame=0;
  }
};

struct CodeSet {             /* コードの管理               */
  TknKind kind;              /* 種類                       */
  const char *text;          /* 文字列リテラルのときの位置 */
  double dblVal;             /* 数値定数のときの値         */
  int    symNbr;             /* 記号表の添字位置           */
  int    jmpAdrs;            /* 飛先番地                   */
  CodeSet() { clear(); }
  CodeSet(TknKind k)           { clear(); kind=k; }
  CodeSet(TknKind k, double d) { clear(); kind=k; dblVal=d; }
  CodeSet(TknKind k, const char *s) { clear(); kind=k; text=s; }
  CodeSet(TknKind k, int sym, int jmp) {
    clear(); kind=k; symNbr=sym; jmpAdrs=jmp;
  }
  void clear() { kind=Others; text=""; dblVal=0.0; jmpAdrs=0; symNbr=-1; }
};

struct Tobj {        /* 型情報付きobj     */
  char type;         /* 格納型 'd':double 's':string  '-':未定 */
  double d;
  string s;
  Tobj()                 { type = '-'; d = 0.0; s = ""; }
  Tobj(double dt)        { type = 'd'; d = dt;  s = ""; }
  Tobj(const string& st) { type = 's'; d = 0.0; s = st; }
  Tobj(const char *st)   { type = 's'; d = 0.0; s = st; }
};

class Mymemory {
private:
  vector<double> mem;
public:
  void auto_resize(int n) {                 /* 再確保回数抑制のため多めに確保 */
    if (n >= (int)mem.size()) { n = (n/256 + 1) * 256; mem.resize(n); }
  }
  void set(int adrs, double dt) { mem[adrs] =  dt; }            /* メモリ書込 */
  void add(int adrs, double dt) { mem[adrs] += dt; }            /* メモリ加算 */
  double get(int adrs)     { return mem[adrs]; }                /* メモリ読出 */
  int size()               { return (int)mem.size(); }          /* 格納サイズ */
  void resize(unsigned int n) { mem.resize(n); }                /* サイズ確保 */
};

