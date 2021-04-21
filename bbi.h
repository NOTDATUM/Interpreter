/**********************************************************/
/*    filename:bbi.h ���ʃw�b�_                           */
/**********************************************************/
#include <iostream>
#include <fstream>    // �t�@�C�������p
#include <sstream>    // ������X�g���[��
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
#define SHORT_SIZ  sizeof(short int)                  /* short int �^�̃T�C�Y */
#define SHORT_P(p) (short int *)(p)              /* short int�^�|�C���^�ɕϊ� */
#define UCHAR_P(p) (unsigned char *)(p)      /* unsigned char�^�|�C���^�ɕϊ� */
#define LIN_SIZ 255                                  /* �\�[�X��1�s�ő�T�C�Y */

/* -------------------- enum struct etc */
enum TknKind {                                                /* �g�[�N���v�f */
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

struct Token {                /* �g�[�N���̊Ǘ�     */
  TknKind kind;               /* �g�[�N���̎��     */
  string  text;               /* �g�[�N��������     */
  double  dblVal;             /* ���l�萔�̂Ƃ��̒l */
  Token() {  kind=Others; text=""; dblVal=0.0; }
  Token (TknKind k)           { kind=k; text=""; dblVal=0.0; }
  Token (TknKind k, double d) { kind=k; text=""; dblVal=d; }
  Token (TknKind k, const string& s) { kind=k; text=s; dblVal=0.0; }
  Token (TknKind k, const string& s, double d) { kind=k; text=s; dblVal=d; }
};

enum SymKind { noId, varId, fncId, paraId };  /* �L���\�o�^���̎�� */
enum DtType  { NON_T, DBL_T };                /* �^�� */

struct SymTbl {               /* �L���\�\��           */
  string  name;               /* �ϐ���֐��̖��O     */
  SymKind nmKind;             /* ���                 */
  char    dtTyp;              /* �ϐ��^(NON_T,DBL_T)  */
  int     aryLen;             /* �z�񒷁B0:�P���ϐ�   */
  short   args;               /* �֐��̈�����       */
  int     adrs;               /* �ϐ�,�֐��̔Ԓn      */
  int     frame;              /* �֐��p�t���[���T�C�Y */
  SymTbl() { clear(); }
  void clear() {
    name=""; nmKind=noId; dtTyp=NON_T;
    aryLen=0; args=0; adrs=0; frame=0;
  }
};

struct CodeSet {             /* �R�[�h�̊Ǘ�               */
  TknKind kind;              /* ���                       */
  const char *text;          /* �����񃊃e�����̂Ƃ��̈ʒu */
  double dblVal;             /* ���l�萔�̂Ƃ��̒l         */
  int    symNbr;             /* �L���\�̓Y���ʒu           */
  int    jmpAdrs;            /* ���Ԓn                   */
  CodeSet() { clear(); }
  CodeSet(TknKind k)           { clear(); kind=k; }
  CodeSet(TknKind k, double d) { clear(); kind=k; dblVal=d; }
  CodeSet(TknKind k, const char *s) { clear(); kind=k; text=s; }
  CodeSet(TknKind k, int sym, int jmp) {
    clear(); kind=k; symNbr=sym; jmpAdrs=jmp;
  }
  void clear() { kind=Others; text=""; dblVal=0.0; jmpAdrs=0; symNbr=-1; }
};

struct Tobj {        /* �^���t��obj     */
  char type;         /* �i�[�^ 'd':double 's':string  '-':���� */
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
  void auto_resize(int n) {                 /* �Ċm�ۉ񐔗}���̂��ߑ��߂Ɋm�� */
    if (n >= (int)mem.size()) { n = (n/256 + 1) * 256; mem.resize(n); }
  }
  void set(int adrs, double dt) { mem[adrs] =  dt; }            /* ���������� */
  void add(int adrs, double dt) { mem[adrs] += dt; }            /* ���������Z */
  double get(int adrs)     { return mem[adrs]; }                /* �������Ǐo */
  int size()               { return (int)mem.size(); }          /* �i�[�T�C�Y */
  void resize(unsigned int n) { mem.resize(n); }                /* �T�C�Y�m�� */
};

