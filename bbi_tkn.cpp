/**********************************************************/
/*    filename:bbi_tkn.cpp �g�[�N������                   */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

struct KeyWord {                                    /* ����Ǝ�ʂ̑Ή����Ǘ� */
  const char *keyName;                                         /* if��for�Ȃ� */
  TknKind keyKind;                        /* �Ή�����l�BTknKind��bbi.h�Œ�` */
};

KeyWord KeyWdTbl[] = {                          /* �\����L���Ǝ�ʂ̑Ή��\ */
  {"func"   , Func  }, {"var"    , Var    },
  {"if"     , If    }, {"elif"   , Elif   },
  {"else"   , Else  }, {"for"    , For    },
  {"to"     , To    }, {"step"   , Step   },
  {"while"  , While }, {"end"    , End    },
  {"break"  , Break }, {"return" , Return },
  {"print"  , Print }, {"println", Println},
  {"option" , Option}, {"input"  , Input  },
  {"toint"  , Toint }, {"exit"   , Exit   },
  {"("  , Lparen    }, {")"  , Rparen   },
  {"["  , Lbracket  }, {"]"  , Rbracket },
  {"+"  , Plus      }, {"-"  , Minus    },
  {"*"  , Multi     }, {"/"  , Divi     },
  {"==" , Equal     }, {"!=" , NotEq    },
  {"<"  , Less      }, {"<=" , LessEq   },
  {">"  , Great     }, {">=" , GreatEq  },
  {"&&" , And       }, {"||" , Or       },
  {"!"  , Not       }, {"%"  , Mod      },
  {"?"  , Ifsub     }, {"="  , Assign   },
  {"\\" , IntDivi   }, {","  , Comma    },
  {"\"" , DblQ      },
  {"@dummy", END_KeyList},
};

int srcLineno;                                             /* �\�[�X�̍s�ԍ� */
TknKind ctyp[256];                                         /* ������\�̔z�� */
char *token_p;                                        /* 1�����擾�p�����ʒu */
bool endOfFile_F;                                      /* �t�@�C���I���t���O */
char buf[LIN_SIZ+5];                                       /* �\�[�X�Ǎ��ꏊ */
ifstream fin;                                              /* ���̓X�g���[�� */
#define MAX_LINE 2000                                  /* �ő�v���O�����s�� */

void initChTyp() /* ������\�̐ݒ� */
{                  /* ��:�S�v�f���g���Ă͂��Ȃ����g���Ή��œ���Ă��� */
  int i;
  for (i=0; i<256; i++)    { ctyp[i] = Others; }
  for (i='0'; i<='9'; i++) { ctyp[i] = Digit;  }
  for (i='A'; i<='Z'; i++) { ctyp[i] = Letter; }
  for (i='a'; i<='z'; i++) { ctyp[i] = Letter; }
  ctyp['_']  = Letter;    ctyp['$']  = Doll;
  ctyp['(']  = Lparen;    ctyp[')']  = Rparen;
  ctyp['[']  = Lbracket;  ctyp[']']  = Rbracket;
  ctyp['<']  = Less;      ctyp['>']  = Great;
  ctyp['+']  = Plus;      ctyp['-']  = Minus;
  ctyp['*']  = Multi;     ctyp['/']  = Divi;
  ctyp['!']  = Not;       ctyp['%']  = Mod;
  ctyp['?']  = Ifsub;     ctyp['=']  = Assign;
  ctyp['\\'] = IntDivi;   ctyp[',']  = Comma;
  ctyp['\"'] = DblQ;
}

void fileOpen(char *fname) /* �t�@�C�����J�� */
{
  srcLineno = 0;
  endOfFile_F = false;
  fin.open(fname);
  if (!fin) { cout << fname << "���I�[�v���ł��܂���\n"; exit(1); }
}

void nextLine() /* ����1�s���擾���� */
{
  string s;

  if (endOfFile_F) return;
  fin.getline(buf, LIN_SIZ+5);                                 /* 1�s�ǂݍ��� */
  if (fin.eof()) {                                            /* �t�@�C���I�� */
    fin.clear(); fin.close();                    /* clear�͍ăI�[�v���ɔ����� */
    endOfFile_F = true; return;
  }

  if (strlen(buf) > LIN_SIZ)
    err_exit("�v���O������1�s ", LIN_SIZ, " �����ȓ��ŋL�q���Ă��������B");
  if (++srcLineno > MAX_LINE)
    err_exit("�v���O������ ", MAX_LINE, " �𒴂��܂����B");
  token_p = buf;                 /* �g�[�N����͗p�|�C���^��buf�擪�Ɉʒu�t�� */
}

Token nextLine_tkn() /* ���̍s��ǂݎ��̃g�[�N����Ԃ� */
{
  nextLine();
  return nextTkn();
}

#define CH (*token_p)
#define C2 (*(token_p+1))
#define NEXT_CH()  ++token_p
Token nextTkn() /* ���̃g�[�N�� */
{
  TknKind kd;
  string txt = "";

  if (endOfFile_F) return Token(EofProg);                     /* �t�@�C���I�� */
  while (isspace(CH)) NEXT_CH();                              /* �󔒓ǂݎ̂� */
  if (CH == '\0')  return Token(EofLine);                             /* �s�� */

  switch (ctyp[CH]) {
  case Doll: case Letter:
    txt += CH; NEXT_CH();
    while (ctyp[CH]==Letter || ctyp[CH]==Digit) { txt += CH; NEXT_CH(); }
    break;
  case Digit:                                                     /* ���l�萔 */
    kd = IntNum;
    while (ctyp[CH] == Digit)   { txt += CH; NEXT_CH(); }
    if (CH == '.') { kd = DblNum; txt += CH; NEXT_CH(); }
    while (ctyp[CH] == Digit)   { txt += CH; NEXT_CH(); }
    return Token(kd, txt, atof(txt.c_str()));       /* IntNum��double�^�Ŋi�[ */
  case DblQ:                                                    /* ������萔 */
    NEXT_CH();
    while (CH!='\0' && CH!='"') { txt += CH; NEXT_CH(); }
    if (CH == '"') NEXT_CH(); else err_exit("�����񃊃e���������Ă��Ȃ��B");
    return Token(String, txt);
  default:
    if (CH=='/' && C2=='/') return Token(EofLine);                /* �R�����g */
    if (is_ope2(CH, C2)) { txt += CH; txt += C2; NEXT_CH(); NEXT_CH(); }
    else                 { txt += CH; NEXT_CH(); }
  }
  kd = get_kind(txt);                                             /* ��ʐݒ� */

  if (kd == Others) err_exit("�s���ȃg�[�N���ł�: ", txt);
  return Token(kd, txt);
}

bool is_ope2(char c1, char c2) /* 2�������Z�q�Ȃ�^ */
{
  char s[] = "    ";
  if (c1=='\0' || c2=='\0') return false;
  s[1] = c1; s[2] = c2;
  return strstr(" ++ -- <= >= == != && || ", s) != NULL;
}

TknKind get_kind(const string& s) /* �g�[�N����ʐݒ� */
{
  for (int i=0; KeyWdTbl[i].keyKind != END_KeyList; i++) {
    if (s == KeyWdTbl[i].keyName) return KeyWdTbl[i].keyKind;
  }
  if (ctyp[s[0]]==Letter || ctyp[s[0]]==Doll) return Ident;
  if (ctyp[s[0]] == Digit)  return DblNum;
  return Others;   // �Ȃ�
}

Token chk_nextTkn(const Token& tk, int kind2) /* �m�F�t�g�[�N���擾 */
{
  if (tk.kind != kind2) err_exit(err_msg(tk.text, kind_to_s(kind2)));
  return nextTkn();
}

void set_token_p(char *p) /* �g�[�N�������|�C���^�ݒ� */
{
  token_p = p;
}

string kind_to_s(int kd) /* ��ʁ������� */
{
  for (int i=0; ; i++) {
    if (KeyWdTbl[i].keyKind == END_KeyList) break;
    if (KeyWdTbl[i].keyKind == kd) return KeyWdTbl[i].keyName;
  }
  return "";
}

string kind_to_s(const CodeSet& cd) /* ��ʁ������� */
{
  switch (cd.kind) {
  case Lvar: case Gvar: case Fcall: return tableP(cd)->name;
  case IntNum: case DblNum: return dbl_to_s(cd.dblVal);
  case String: return string("\"") + cd.text + "\"";
  case EofLine: return "";
  }
  return kind_to_s(cd.kind);
}

int get_lineNo() /* �Ǎ�or���s���̍s�ԍ� */
{
  extern int Pc;
  return (Pc == -1) ? srcLineno : Pc;                      /* ��͒� : ���s�� */
}

