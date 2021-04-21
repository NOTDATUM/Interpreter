/**********************************************************/
/*    filename:bbi_pars.cpp 構文解析                      */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

#define NO_FIX_ADRS 0                                 /* まだ番地未定のマーク */
Token token;                                          /* 現在処理中のトークン */
SymTbl tmpTb;                                               /* 一時格納記号表 */
int blkNest;                                                /* ブロックの深さ */
int localAdrs;                                        /* 局所変数アドレス管理 */
int mainTblNbr;                             /* main関数があればその記号表位置 */
int loopNest;                                                 /* ループネスト */
bool fncDecl_F;                                       /* 関数定義処理中なら真 */
bool explicit_F;                                        /* 真なら変数宣言強制 */
char codebuf[LIN_SIZ+1], *codebuf_p;                  /* 内部コード生成作業用 */
extern vector<char*> intercode;                     /* 変換済み内部コード格納 */

void init() /* 初期値設定 */
{
  initChTyp();                                                    /* 文字種表 */
  mainTblNbr = -1;
  blkNest = loopNest = 0;
  fncDecl_F = explicit_F = false;
  codebuf_p = codebuf;
}

void convert_to_internalCode(char *fname) /* コード変換 */
{
  init();                                               /* 文字種表など初期化 */

  // 関数定義名のみ先に登録
  fileOpen(fname);
  while (token=nextLine_tkn(), token.kind != EofProg) {
    if (token.kind == Func) {
      token = nextTkn(); set_name(); enter(tmpTb, fncId);
    }
  }

  // 内部コードへの変換
  push_intercode();                                /* 0行目は不要なので埋める */
  fileOpen(fname);
  token = nextLine_tkn();
  while (token.kind != EofProg) {
    convert();                                            /* 内部コードに変換 */
  }

  // main関数があればその呼び出しコードを設定
  set_startPc(1);                                        /* 1行目から実行開始 */
  if (mainTblNbr != -1) {
    set_startPc(intercode.size());                        /* mainから実行開始 */
    setCode(Fcall, mainTblNbr); setCode('('); setCode(')');
    push_intercode();
  }
}

// 先頭だけに出現するコードを処理。残り部分はconvert_rest()で処理
void convert()
{
  switch (token.kind) {
  case Option: optionSet(); break;                          /* オプション設定 */
  case Var:    varDecl();   break;                                /* 変数宣言 */
  case Func:   fncDecl();   break;                                /* 関数定義 */
  case While: case For:
       ++loopNest;
       convert_block_set(); setCode_End();
       --loopNest;
       break;
  case If:
       convert_block_set();                                // if
       while (token.kind == Elif) { convert_block_set(); } // elif
       if (token.kind == Else)    { convert_block_set(); } // else
       setCode_End();                                      // end
       break;
  case Break:
       if (loopNest <= 0) err_exit("不正なbreakです。");
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case Return:
       if (!fncDecl_F) err_exit("不正なreturnです。");
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case Exit:
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case Print: case Println:
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case End:
       err_exit("不正な end です。");       /* end が単独で使われることはない */
       break;
  default: convert_rest(); break;
  }
}

void convert_block_set() /* ブロック処理管理 */
{
  int patch_line;
  patch_line = setCode(token.kind, NO_FIX_ADRS); token = nextTkn();
  convert_rest();
  convert_block();                                            /* ブロック処理 */
  backPatch(patch_line, get_lineNo());        /* NO_FIX_ADRSを修正(end行番号) */
}

void convert_block() /* ブロックの処理 */
{
  TknKind k;
  ++blkNest;                                      /* ブロック終端まで文を解析 */
  while(k=token.kind, k!=Elif && k!=Else && k!=End && k!=EofProg) {
    convert();
  }
  --blkNest;
}

void convert_rest() /* 文の残りの処理 */
{
  int tblNbr;

  for (;;) {
    if (token.kind == EofLine) break;
    switch (token.kind) {      //↓これらのキーワードが途中に現れることはない */
    case If: case Elif: case Else: case For: case While: case Break:
    case Func:  case Return:  case Exit:  case Print:  case Println:
    case Option: case Var: case End:
         err_exit("不正な記述です: ", token.text);
         break;
    case Ident:                                              /* 関数呼出,変数 */
         set_name();
         if ((tblNbr=searchName(tmpTb.name, 'F')) != -1) {    /* 関数登録あり */
           if (tmpTb.name == "main") err_exit("main関数の呼出はできません。");
           setCode(Fcall, tblNbr); continue;
         }
         if ((tblNbr=searchName(tmpTb.name, 'V')) == -1) {    /* 変数登録なし */
           if (explicit_F) err_exit("変数宣言が必要です: ", tmpTb.name);
           tblNbr = enter(tmpTb, varId);                      /* 自動変数登録 */
         }
         if (is_localName(tmpTb.name, varId)) setCode(Lvar, tblNbr);
         else                                 setCode(Gvar, tblNbr);
         continue;
     case IntNum: case DblNum:                         /* 整数もdouble型で格納 */
         setCode(token.kind, set_LITERAL(token.dblVal));
         break;
     case String:
         setCode(token.kind, set_LITERAL(token.text));
         break;
    default:                                                   /* + - <= など */
         setCode(token.kind);
         break;
    }
    token = nextTkn();
  }
  push_intercode();
  token = nextLine_tkn();
}

void optionSet() /* オプション設定 */
{
  setCode(Option);            /* この行は非実行なのでコード変換は Option だけ */
  setCode_rest();                                       /* 残りは元のまま格納 */
  token = nextTkn();                                  /* ↓変数宣言を強制する */
  if (token.kind==String && token.text=="var") explicit_F = true;
  else err_exit("option指定が不正です。");
  token = nextTkn();
  setCode_EofLine();
}

void varDecl() /* varを使う変数宣言 */
{
  setCode(Var);                  /* この行は非実行なのでコード変換は Var だけ */
  setCode_rest();                                       /* 残りは元のまま格納 */
  for (;;) {
    token = nextTkn();
    var_namechk(token);                                           /* 名前検査 */
    set_name(); set_aryLen();                             /* 配列なら長さ設定 */
    enter(tmpTb, varId);                          /* 変数登録(アドレスも設定) */
    if (token.kind != ',') break;                                 /* 宣言終了 */
  }
  setCode_EofLine();
}

void var_namechk(const Token& tk) /* 名前確認 */
{
  if (tk.kind != Ident) err_exit(err_msg(tk.text, "識別子"));
  if (is_localScope() && tk.text[0] == '$')
    err_exit("関数内のvar宣言では $ 付き名前を指定できません: ", tk.text);
  if (searchName(tk.text, 'V') != -1)
    err_exit("識別子が重複しています: ", tk.text);
}

void set_name() /* 名前設定 */
{
  if (token.kind != Ident) err_exit("識別子が必要です: ", token.text);
  tmpTb.clear(); tmpTb.name = token.text;                         /* 名前設定 */
  token = nextTkn();
}

void set_aryLen() /* 配列サイズ設定 */
{
  tmpTb.aryLen = 0;
  if (token.kind != '[') return;                                /* 配列でない */

  token = nextTkn();
  if (token.kind != IntNum)
    err_exit("配列長は正の整数定数で指定してください: ", token.text);
  tmpTb.aryLen = (int)token.dblVal + 1;   /* var a[5]は添字0～5が有効なので+1 */
  token = chk_nextTkn(nextTkn(), ']');
  if (token.kind == '[') err_exit("多次元配列は宣言できません。");
}

void fncDecl() /* 関数定義 */
{
  extern vector<SymTbl> Gtable;                                 /* 大域記号表 */
  int tblNbr, patch_line, fncTblNbr;

  if(blkNest > 0) err_exit("関数定義の位置が不正です。");
  fncDecl_F = true;                                     /* 関数処理開始フラグ */
  localAdrs = 0;                              /* 局所領域割付けカウンタ初期化 */
  set_startLtable();                                    /* 局所記号表開始位置 */
  patch_line = setCode(Func, NO_FIX_ADRS);          /* あとでend行番号を入れる*/
  token = nextTkn();

  fncTblNbr = searchName(token.text, 'F');      /* 関数名は最初に登録している */
  Gtable[fncTblNbr].dtTyp = DBL_T;                      /* 関数型はdouble固定 */

  // 仮引数解析
  token = nextTkn();
  token = chk_nextTkn(token, '(');                               /* '('のはず */
  setCode('(');
  if (token.kind != ')') {                                        /* 引数あり */
    for (;; token=nextTkn()) {
      set_name();
      tblNbr = enter(tmpTb, paraId);                              /* 引数登録 */
      setCode(Lvar, tblNbr);                          /* 引数はLvarとして処理 */
      ++Gtable[fncTblNbr].args;                               /* 引数個数を+1 */
      if (token.kind != ',') break;                               /* 宣言終了 */
      setCode(',');
    }
  }
  token = chk_nextTkn(token, ')');                               /* ')'のはず */
  setCode(')'); setCode_EofLine();
  convert_block();                                            /* 関数本体処理 */

  backPatch(patch_line, get_lineNo());                   /* NO_FIX_ADRSを修正 */
  setCode_End();
  Gtable[fncTblNbr].frame = localAdrs;                      /* フレームサイズ */

  if (Gtable[fncTblNbr].name == "main") {                     /* main関数処理 */
    mainTblNbr = fncTblNbr;
    if (Gtable[mainTblNbr].args != 0)
      err_exit("main関数では仮引数を指定できません。");
  }
  fncDecl_F = false;                                          /* 関数処理終了 */
}

void backPatch(int line, int n) /* line行にnを設定 */
{
  *SHORT_P(intercode[line] + 1) = (short)n;
}

void setCode(int cd) /* コード格納 */
{
  *codebuf_p++ = (char)cd;
}

int setCode(int cd, int nbr) /* コードとshort値格納 */
{
  *codebuf_p++ = (char)cd;
  *SHORT_P(codebuf_p) = (short)nbr; codebuf_p += SHORT_SIZ;
  return get_lineNo();                           /* backPatch用に格納行を返す */
}

void setCode_rest() /* 残りのテキストをそのまま格納 */
{
  extern char *token_p;
  strcpy(codebuf_p, token_p);
  codebuf_p += strlen(token_p) + 1;
}

void setCode_End() /* endの格納処理 */
{
  if (token.kind != End) err_exit(err_msg(token.text, "end"));
  setCode(End); token = nextTkn(); setCode_EofLine();
}

void setCode_EofLine() /* 最終格納処理 */
{
  if (token.kind != EofLine) err_exit("不正な記述です: ", token.text);
  push_intercode();
  token = nextLine_tkn();                                     /* 次の行に進む */
}

void push_intercode() /* 変換した内部コードを格納 */
{
  int len;
  char *p;

  *codebuf_p++ = '\0';
  if ((len = codebuf_p-codebuf) >= LIN_SIZ)
    err_exit("変換後の内部コードが長すぎます。式を短くしてください。");

  try {
    p = new char[len];                                          /* メモリ確保 */
    memcpy(p, codebuf, len);
    intercode.push_back(p);
  }
  catch (bad_alloc) { err_exit("メモリ確保できません"); }
  codebuf_p = codebuf;                /* 次の処理のために格納先先頭に位置付け */
}

bool is_localScope() /* 関数内処理中なら真 */
{
  return fncDecl_F;
}

