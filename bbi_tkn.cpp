/**********************************************************/
/*    filename:bbi_tkn.cpp トークン処理                   */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

struct KeyWord {                                    /* 字句と種別の対応を管理 */
  const char *keyName;                                         /* ifやforなど */
  TknKind keyKind;                        /* 対応する値。TknKindはbbi.hで定義 */
};

KeyWord KeyWdTbl[] = {                          /* 予約語や記号と種別の対応表 */
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

int srcLineno;                                             /* ソースの行番号 */
TknKind ctyp[256];                                         /* 文字種表の配列 */
char *token_p;                                        /* 1文字取得用文字位置 */
bool endOfFile_F;                                      /* ファイル終了フラグ */
char buf[LIN_SIZ+5];                                       /* ソース読込場所 */
ifstream fin;                                              /* 入力ストリーム */
#define MAX_LINE 2000                                  /* 最大プログラム行数 */

void initChTyp() /* 文字種表の設定 */
{                  /* 注:全要素を使ってはいないが拡張対応で入れている */
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

void fileOpen(char *fname) /* ファイルを開く */
{
  srcLineno = 0;
  endOfFile_F = false;
  fin.open(fname);
  if (!fin) { cout << fname << "をオープンできません\n"; exit(1); }
}

void nextLine() /* 次の1行を取得する */
{
  string s;

  if (endOfFile_F) return;
  fin.getline(buf, LIN_SIZ+5);                                 /* 1行読み込み */
  if (fin.eof()) {                                            /* ファイル終了 */
    fin.clear(); fin.close();                    /* clearは再オープンに備える */
    endOfFile_F = true; return;
  }

  if (strlen(buf) > LIN_SIZ)
    err_exit("プログラムは1行 ", LIN_SIZ, " 文字以内で記述してください。");
  if (++srcLineno > MAX_LINE)
    err_exit("プログラムが ", MAX_LINE, " を超えました。");
  token_p = buf;                 /* トークン解析用ポインタをbuf先頭に位置付け */
}

Token nextLine_tkn() /* 次の行を読み次のトークンを返す */
{
  nextLine();
  return nextTkn();
}

#define CH (*token_p)
#define C2 (*(token_p+1))
#define NEXT_CH()  ++token_p
Token nextTkn() /* 次のトークン */
{
  TknKind kd;
  string txt = "";

  if (endOfFile_F) return Token(EofProg);                     /* ファイル終了 */
  while (isspace(CH)) NEXT_CH();                              /* 空白読み捨て */
  if (CH == '\0')  return Token(EofLine);                             /* 行末 */

  switch (ctyp[CH]) {
  case Doll: case Letter:
    txt += CH; NEXT_CH();
    while (ctyp[CH]==Letter || ctyp[CH]==Digit) { txt += CH; NEXT_CH(); }
    break;
  case Digit:                                                     /* 数値定数 */
    kd = IntNum;
    while (ctyp[CH] == Digit)   { txt += CH; NEXT_CH(); }
    if (CH == '.') { kd = DblNum; txt += CH; NEXT_CH(); }
    while (ctyp[CH] == Digit)   { txt += CH; NEXT_CH(); }
    return Token(kd, txt, atof(txt.c_str()));       /* IntNumもdouble型で格納 */
  case DblQ:                                                    /* 文字列定数 */
    NEXT_CH();
    while (CH!='\0' && CH!='"') { txt += CH; NEXT_CH(); }
    if (CH == '"') NEXT_CH(); else err_exit("文字列リテラルが閉じていない。");
    return Token(String, txt);
  default:
    if (CH=='/' && C2=='/') return Token(EofLine);                /* コメント */
    if (is_ope2(CH, C2)) { txt += CH; txt += C2; NEXT_CH(); NEXT_CH(); }
    else                 { txt += CH; NEXT_CH(); }
  }
  kd = get_kind(txt);                                             /* 種別設定 */

  if (kd == Others) err_exit("不正なトークンです: ", txt);
  return Token(kd, txt);
}

bool is_ope2(char c1, char c2) /* 2文字演算子なら真 */
{
  char s[] = "    ";
  if (c1=='\0' || c2=='\0') return false;
  s[1] = c1; s[2] = c2;
  return strstr(" ++ -- <= >= == != && || ", s) != NULL;
}

TknKind get_kind(const string& s) /* トークン種別設定 */
{
  for (int i=0; KeyWdTbl[i].keyKind != END_KeyList; i++) {
    if (s == KeyWdTbl[i].keyName) return KeyWdTbl[i].keyKind;
  }
  if (ctyp[s[0]]==Letter || ctyp[s[0]]==Doll) return Ident;
  if (ctyp[s[0]] == Digit)  return DblNum;
  return Others;   // ない
}

Token chk_nextTkn(const Token& tk, int kind2) /* 確認付トークン取得 */
{
  if (tk.kind != kind2) err_exit(err_msg(tk.text, kind_to_s(kind2)));
  return nextTkn();
}

void set_token_p(char *p) /* トークン処理ポインタ設定 */
{
  token_p = p;
}

string kind_to_s(int kd) /* 種別→文字列 */
{
  for (int i=0; ; i++) {
    if (KeyWdTbl[i].keyKind == END_KeyList) break;
    if (KeyWdTbl[i].keyKind == kd) return KeyWdTbl[i].keyName;
  }
  return "";
}

string kind_to_s(const CodeSet& cd) /* 種別→文字列 */
{
  switch (cd.kind) {
  case Lvar: case Gvar: case Fcall: return tableP(cd)->name;
  case IntNum: case DblNum: return dbl_to_s(cd.dblVal);
  case String: return string("\"") + cd.text + "\"";
  case EofLine: return "";
  }
  return kind_to_s(cd.kind);
}

int get_lineNo() /* 読込or実行中の行番号 */
{
  extern int Pc;
  return (Pc == -1) ? srcLineno : Pc;                      /* 解析中 : 実行中 */
}

