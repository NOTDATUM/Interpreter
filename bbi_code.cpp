/**************************************************************/
/*    filename:bbi_code.cpp メモリ管理と構文チェックと実行    */
/**************************************************************/
#include "bbi.h"
#include "bbi_prot.h"

CodeSet code;                                                 /* コードセット */
int startPc;                                                    /* 実行開始行 */
int Pc = -1;                                /* プログラムカウンタ -1:非実行中 */
int baseReg;                                                /* ベースレジスタ */
int spReg;                                                /* スタックポインタ */
int maxLine;                                              /* プログラム末尾行 */
vector<char*> intercode;                            /* 変換済み内部コード格納 */
char *code_ptr;                                   /* 内部コード解析用ポインタ */
double returnValue;                                               /* 関数戻値 */
bool break_Flg, return_Flg, exit_Flg;                         /* 制御用フラグ */
Mymemory Dmem;                                                      /* 主記憶 */
vector<string> strLITERAL;                              /* 文字列リテラル格納 */
vector<double> nbrLITERAL;                                /* 数値リテラル格納 */
bool syntaxChk_mode = false;                          /* 構文チェックのとき真 */
extern vector<SymTbl> Gtable;                                   /* 大域記号表 */

class Mystack {                                   /* stack<double> のラッパー */
private:
  stack<double> st;
public:
  void push(double n) { st.push(n); }                                 /* 積込 */
  int size() { return (int)st.size(); }                             /* サイズ */
  bool empty() { return st.empty(); }                             /* 空き判定 */
  double pop() {                        /* 読出&削除(元のpopと異なるので注意) */
    if (st.empty()) err_exit("stack underflow");
    double d = st.top();                                          /* トップ値 */
    st.pop(); return d;                                         /* ひとつ削除 */
  }
};
Mystack stk;                                            /* オペランドスタック */

void syntaxChk() /* 構文チェック */
{
  syntaxChk_mode = true;
  for (Pc=1; Pc<(int)intercode.size(); Pc++) {
    code = firstCode(Pc);
    switch (code.kind) {
    case Func: case Option: case Var:                         /* チェック済み */
      break;
    case Else: case End: case Exit:
      code = nextCode(); chk_EofLine();
      break;
    case If: case Elif: case While:
      code = nextCode(); (void)get_expression(0, EofLine);            /* 式値 */
      break;
    case For:
      code = nextCode();
      (void)get_memAdrs(code);                            /* 制御変数アドレス */
      (void)get_expression('=', 0);                                 /* 初期値 */
      (void)get_expression(To, 0);                                  /* 最終値 */
      if (code.kind == Step) (void)get_expression(Step,0);          /* 刻み値 */
      chk_EofLine();
      break;
    case Fcall:                                         /* 代入のない関数呼出 */
      fncCall_syntax(code.symNbr);
      chk_EofLine();
      (void)stk.pop();                                          /* 戻り値不要 */
      break;
    case Print: case Println:
      sysFncExec_syntax(code.kind);
      break;
    case Gvar: case Lvar:                                           /* 代入文 */
      (void)get_memAdrs(code);                                /* 左辺アドレス */
      (void)get_expression('=', EofLine);                         /* 右辺式値 */
      break;
    case Return:
      code = nextCode();                                              /* 戻値 */
      if (code.kind!='?' && code.kind!=EofLine) (void)get_expression();
      if (code.kind == '?') (void)get_expression('?', 0);
      chk_EofLine();
      break;
    case Break:
      code = nextCode();
      if (code.kind == '?') (void)get_expression('?', 0);
      chk_EofLine();
      break;
    case EofLine:
      break;
    default:
      err_exit("不正な記述です: ", kind_to_s(code.kind));
    }
  }
  syntaxChk_mode = false;
}

void set_startPc(int n) /* 開始行設定 */
{
  startPc = n;
}

void execute() /* 実行 */
{
  baseReg = 0;                                        /* ベースレジスタ初期値 */
  spReg = Dmem.size();                              /* スタックポインタ初期値 */
  Dmem.resize(spReg+1000);                              /* 主記憶領域初期確保 */
  break_Flg = return_Flg = exit_Flg = false;

  Pc = startPc;
  maxLine = intercode.size() - 1;
  while (Pc<=maxLine && !exit_Flg) {
    statement();
  }
  Pc = -1;                                                      /* 非実行状態 */
}

void statement() /* 文 */
{
  CodeSet save;
  int top_line, end_line, varAdrs;
  double wkVal, endDt, stepDt;

  if (Pc>maxLine || exit_Flg) return;                       /* プログラム終了 */
  code = save = firstCode(Pc);
  top_line = Pc; end_line = code.jmpAdrs;             /* 制御範囲の先頭と終端 */
  if (code.kind == If ) end_line = endline_of_If(Pc);     /* if文のときの終端 */

  switch (code.kind) {
  case If:
    // if
    if (get_expression(If, 0)) {                          /* ─┐Trueなら     */
      ++Pc; block(); Pc = end_line + 1;                   /*   │実行して     */
      return;                                             /* ─┘終了         */
    }
    Pc = save.jmpAdrs;                                    /*     次へ         */
    // elif
    while (lookCode(Pc) == Elif) {
      save = firstCode(Pc); code = nextCode();
      if (get_expression()) {                             /* ─┐Trueなら     */
        ++Pc; block(); Pc = end_line + 1;                 /*   │実行して     */
        return;                                           /* ─┘終了         */
      }
      Pc = save.jmpAdrs;                                  /*     次へ         */
    }
    // else
    if (lookCode(Pc) == Else) {                           /* ─┐elseを       */
      ++Pc; block(); Pc = end_line + 1;                   /*   │実行して     */
      return;                                             /* ─┘終了         */
    }
    // end
    ++Pc;
    break;
  case While:
    for (;;) {                                           /* ←┐              */
      if (!get_expression(While, 0)) break;              /* ─┼┐false終了   */
      ++Pc; block();                                     /*   ││[実行]      */
      if (break_Flg || return_Flg || exit_Flg) {         /*   ││            */
        break_Flg = false; break;                        /* ─┼┤中断        */
      }                                                  /*   ││            */
      Pc = top_line; code = firstCode(Pc);               /* ─┘│先頭へ      */
    }                                                    /*     │            */
    Pc = end_line + 1;                                   /* ←─┘            */
    break;
  case For:
    save = nextCode();
    varAdrs = get_memAdrs(save);                    /* 制御変数アドレスを取得 */

    expression('=', 0);                                             /* 初期値 */
    set_dtTyp(save, DBL_T);                                         /* 型確定 */
    Dmem.set(varAdrs, stk.pop());                             /* 初期値を設定 */

    endDt = get_expression(To, 0);                            /* 最終値を保存 */
                                                              /* 刻み値を保存 */
    if (code.kind == Step) stepDt = get_expression(Step, 0); else stepDt = 1.0;
    for (;; Pc=top_line) {                               /* ←┐              */
      if (stepDt >= 0) {                                 /*   │  増分ループ  */
        if (Dmem.get(varAdrs) > endDt) break;            /* ─┼┐偽なら終了  */
      } else {                                           /*   ││減分ループ  */
        if (Dmem.get(varAdrs) < endDt) break;            /* ─┼┤偽なら終了  */
      }                                                  /*   ││            */
      ++Pc; block();                                     /*   ││[実行]      */
      if (break_Flg || return_Flg || exit_Flg) {         /*   ││            */
        break_Flg = false; break;                        /* ─┼┤中断        */
      }                                                  /*   ││            */
      Dmem.add(varAdrs, stepDt);                         /* ─┘│ 値更新     */
    }                                                    /*     │            */
    Pc = end_line + 1;                                   /* ←─┘            */
    break;
  case Fcall:                                           /* 代入のない関数呼出 */
    fncCall(code.symNbr);
    (void)stk.pop();                                            /* 戻り値不要 */
    ++Pc;
    break;
  case Func:                                            /* 関数定義はスキップ */
    Pc = end_line + 1;
    break;
  case Print: case Println:
    sysFncExec(code.kind);
    ++Pc;
    break;
  case Gvar: case Lvar:                                             /* 代入文 */
    varAdrs = get_memAdrs(code);
    expression('=', 0);
    set_dtTyp(save, DBL_T);                                 /* 代入時に型確定 */
    Dmem.set(varAdrs, stk.pop());
    ++Pc;
    break;
  case Return:
    wkVal = returnValue;
    code = nextCode();
    if (code.kind!='?' && code.kind!=EofLine)   /* 「式」があれば戻り値を計算 */
      wkVal = get_expression();
    post_if_set(return_Flg);                                /* ? があれば処理 */
    if (return_Flg) returnValue = wkVal;
    if (!return_Flg) ++Pc;
    break;
  case Break:
    code = nextCode(); post_if_set(break_Flg);              /* ? があれば処理 */
    if (!break_Flg) ++Pc;
    break;
  case Exit:
    code = nextCode(); exit_Flg = true;
    break;
  case Option: case Var: case EofLine:                        /* 実行時は無視 */
    ++Pc;
    break;
  default:
    err_exit("不正な記述です: ", kind_to_s(code.kind));
  }
}

void block() /* ブロック終端までの文を実行 */
{
  TknKind k;
  while (!break_Flg && !return_Flg && !exit_Flg) {  /* break,return,exitで終了*/
    k = lookCode(Pc);                                       /* 次の先頭コード */
    if (k==Elif || k==Else || k==End) break;              /* ブロック正常終了 */
    statement();
  }
}

// 関数宣言で次のデフォルト引数を指定
// double get_expression(int kind1=0, int kind2=0)
double get_expression(int kind1, int kind2) /* 結果を返すexpression */
{
  expression(kind1, kind2); return stk.pop();
}

void expression(int kind1, int kind2) /* 確認付きexpression */
{
  if (kind1 != 0) code = chk_nextCode(code, kind1);
  expression();
  if (kind2 != 0) code = chk_nextCode(code, kind2);
}

void expression() /* 式 */
{
  term(1);
}

void term(int n) /* nは優先順位 */
{
  TknKind op;
  if (n == 7) { factor(); return; }
  term(n+1);
  while (n == opOrder(code.kind)) {                 /* 強さが同じ演算子が続く */
    op = code.kind;
    code = nextCode(); term(n+1);
    if (syntaxChk_mode) { stk.pop(); stk.pop(); stk.push(1.0); } /* 構文chk時 */
    else binaryExpr(op);
  }
}

void factor() /* 因子 */
{
  TknKind kd = code.kind;

  if (syntaxChk_mode) {                                          /* 構文chk時 */
    switch (kd) {
    case Not: case Minus: case Plus:
         code = nextCode(); factor(); stk.pop(); stk.push(1.0);
         break;
    case Lparen:
         expression('(', ')');
         break;
    case IntNum: case DblNum:
         stk.push(1.0); code = nextCode();
         break;
    case Gvar: case Lvar:
         (void)get_memAdrs(code); stk.push(1.0);
         break;
    case Toint: case Input:
         sysFncExec_syntax(kd);
         break;
    case Fcall:
         fncCall_syntax(code.symNbr);
         break;
    case EofLine:
         err_exit("式が不正です。");
    default:
         err_exit("式誤り:", kind_to_s(code));            /* a + = などで発生 */
    }
    return;
  }

  switch (kd) {                                                     /* 実行時 */
  case Not: case Minus: case Plus:
       code = nextCode(); factor();                         /* 次の値を取得し */
       if (kd == Not) stk.push(!stk.pop());                      /* !処理する */
       if (kd == Minus) stk.push(-stk.pop());                    /* -処理する */
       break;                                            /* 単項+は何もしない */
  case Lparen:
       expression('(', ')');
       break;
  case IntNum: case DblNum:
       stk.push(code.dblVal); code = nextCode();
       break;
  case Gvar: case Lvar:
       chk_dtTyp(code);                                 /* 値設定済みの変数か */
       stk.push(Dmem.get(get_memAdrs(code)));
       break;
  case Toint: case Input:
       sysFncExec(kd);
       break;
  case Fcall:
       fncCall(code.symNbr);
       break;
  }
}

int opOrder(TknKind kd) /* 二項演算子の優先順位 */
{
    switch (kd) {
    case Multi: case Divi: case Mod:
    case IntDivi:                    return 6; /* *  /  % \  */
    case Plus:  case Minus:          return 5; /* +  -       */
    case Less:  case LessEq:
    case Great: case GreatEq:        return 4; /* <  <= > >= */
    case Equal: case NotEq:          return 3; /* == !=      */
    case And:                        return 2; /* &&         */
    case Or:                         return 1; /* ||         */
    default:                         return 0; /* 該当なし   */
    }
}

void binaryExpr(TknKind op) /* 二項演算 */
{
  double d = 0, d2 = stk.pop(), d1 = stk.pop();

  if ((op==Divi || op==Mod || op==IntDivi) && d2==0)
    err_exit("ゼロ除算です。");

  switch (op) {
  case Plus:    d = d1 + d2;  break;
  case Minus:   d = d1 - d2;  break;
  case Multi:   d = d1 * d2;  break;
  case Divi:    d = d1 / d2;  break;
  case Mod:     d = (int)d1 % (int)d2; break;
  case IntDivi: d = (int)d1 / (int)d2; break;
  case Less:    d = d1 <  d2; break;
  case LessEq:  d = d1 <= d2; break;
  case Great:   d = d1 >  d2; break;
  case GreatEq: d = d1 >= d2; break;
  case Equal:   d = d1 == d2; break;
  case NotEq:   d = d1 != d2; break;
  case And:     d = d1 && d2; break;
  case Or:      d = d1 || d2; break;
  }
  stk.push(d);
}

void post_if_set(bool& flg) /* ? 式 */
{
  if (code.kind == EofLine) { flg = true; return; }       /* ?無しならflgを真 */
  if (get_expression('?', 0)) flg = true;                     /* 条件式で処理 */
}

void fncCall_syntax(int fncNbr) /* 関数呼出チェック */
{
  int argCt = 0;

  code = nextCode(); code = chk_nextCode(code, '(');
  if (code.kind != ')') {                                       /* 引数がある */
    for (;; code=nextCode()) {
      (void)get_expression(); ++argCt;                /* 引数式処理と引数個数 */
      if (code.kind != ',') break;                        /* , なら引数が続く */
    }
  }
  code = chk_nextCode(code, ')');                                 /* ) のはず */
  if (argCt != Gtable[fncNbr].args)                       /* 引数個数チェック */
    err_exit(Gtable[fncNbr].name, " 関数の引数個数が誤っています。");
  stk.push(1.0);                                                /* 無難な戻値 */
}

void fncCall(int fncNbr) /* 関数呼出 */
{
  int  n, argCt = 0;
  vector<double> vc;

  // 実引数積み込み
  nextCode(); code = nextCode();                         /* 関数名 ( スキップ */
  if (code.kind != ')') {                                       /* 引数がある */
    for (;; code=nextCode()) {
      expression(); ++argCt;                          /* 引数式処理と引数個数 */
      if (code.kind != ',') break;                        /* , なら引数が続く */
    }
  }
  code = nextCode();                                            /* ) スキップ */

  // 引数積み込み順序変更
  for (n=0; n<argCt; n++) vc.push_back(stk.pop());  /* 後ろから引数積込に修正 */
  for (n=0; n<argCt; n++) { stk.push(vc[n]); }

  fncExec(fncNbr);                                                /* 関数実行 */
}

void fncExec(int fncNbr) /* 関数実行 */
{
  // 関数入口処理1
  int save_Pc = Pc;                                     /* 現在の実行行を保存 */
  int save_baseReg = baseReg;                          /* 現在のbaseRegを保存 */
  int save_spReg = spReg;                                /* 現在のspRegを保存 */
  char *save_code_ptr = code_ptr;         /* 現在の実行行解析用ポインタを保存 */
  CodeSet save_code = code;                               /* 現在のcodeを保存 */

  // 関数入口処理2
  Pc = Gtable[fncNbr].adrs;                                   /* 新しいPc設定 */
  baseReg = spReg;                                /* 新しいベースレジスタ設定 */
  spReg += Gtable[fncNbr].frame;                              /* フレーム確保 */
  Dmem.auto_resize(spReg);                            /* 主記憶の有効領域確保 */
  returnValue = 1.0;                                          /* 戻り値既定値 */
  code = firstCode(Pc);                                     /* 先頭コード取得 */

  // 引数格納処理
  nextCode(); code = nextCode();                           /* Func ( スキップ */
  if (code.kind != ')') {                                         /* 引数あり */
    for (;; code=nextCode()) {
      set_dtTyp(code, DBL_T);                               /* 代入時に型確定 */
      Dmem.set(get_memAdrs(code), stk.pop());                 /* 実引数値格納 */
      if (code.kind != ',') break;                                /* 引数終了 */
    }
  }
  code = nextCode();                                            /* ) スキップ */

  // 関数本体処理
  ++Pc; block(); return_Flg = false;                          /* 関数本体処理 */

  // 関数出口処理
  stk.push(returnValue);                                        /* 戻り値設定 */
  Pc       = save_Pc;                                   /* 呼出前の環境を復活 */
  baseReg  = save_baseReg;
  spReg    = save_spReg;
  code_ptr = save_code_ptr;
  code     = save_code;
}

void sysFncExec_syntax(TknKind kd) /* 組込関数チェック */
{
  switch (kd) {
  case Toint:
       code = nextCode(); (void)get_expression('(', ')');
       stk.push(1.0);
       break;
  case Input:
       code = nextCode();
       code = chk_nextCode(code, '('); code = chk_nextCode(code, ')');
       stk.push(1.0);                                             /* 無難な値 */
       break;
  case Print: case Println:
       do {
         code = nextCode();
         if (code.kind == String) code = nextCode();        /* 文字列出力確認 */
         else (void)get_expression();                           /* 値出力確認 */
       } while (code.kind == ',');                  /* , ならパラメータが続く */
       chk_EofLine();
       break;
  }
}

void sysFncExec(TknKind kd) /* 組込関数実行 */
{
  double d;
  string s;

  switch (kd) {
  case Toint:
       code = nextCode();
       stk.push((int)get_expression('(', ')'));               /* 端数切り捨て */
       break;
  case Input:
       nextCode(); nextCode(); code = nextCode();       /* input ( ) スキップ */
       getline(cin, s);                                        /* 1行読み込み */
       stk.push(atof(s.c_str()));                       /* 数字に変換して格納 */
       break;
  case Print: case Println:
       do {
         code = nextCode();
         if (code.kind == String) {                             /* 文字列出力 */
           cout << code.text; code = nextCode();
         } else {
           d = get_expression();                  /* 関数内でexitの可能性あり */
           if (!exit_Flg) cout << d;                              /* 数値出力 */
         }
       } while (code.kind == ',');                  /* , ならパラメータが続く */
       if (kd == Println) cout << endl;                    /* printlnなら改行 */
       break;
  }
}

// 単純変数または配列要素のアドレスを返す
int get_memAdrs(const CodeSet& cd)
{
  int adr=0, index, len;
  double d;

  adr = get_topAdrs(cd);
  len = tableP(cd)->aryLen;
  code = nextCode();
  if (len == 0) return adr;                                     /* 非配列変数 */

  d = get_expression('[', ']');
  if ((int)d != d) err_exit("添字は端数のない数値で指定してください。");
  if (syntaxChk_mode) return adr;                           /* 構文チェック時 */

  index = (int)d;
  if (index <0 || len <= index)
    err_exit(index, " は添字範囲外です（添字範囲:0-", len-1, "）");
  return adr + index;                                             /* 添字加算 */
}

// 変数の先頭(配列のときもその先頭)アドレスを返す
int get_topAdrs(const CodeSet& cd)
{
  switch (cd.kind) {
  case Gvar: return tableP(cd)->adrs;
  case Lvar: return tableP(cd)->adrs + baseReg;
  default: err_exit("変数名が必要です: ", kind_to_s(cd));
  }
  return 0; // ここにはこない
}

int endline_of_If(int line) /* if文の対応end位置 */
{
  CodeSet cd;
  char *save = code_ptr;

  cd = firstCode(line);
  for (;;) {
    line = cd.jmpAdrs;
    cd = firstCode(line);
    if (cd.kind==Elif || cd.kind==Else) continue;
    if (cd.kind == End) break;
  }
  code_ptr = save;
  return line;
}

void chk_EofLine() /* コード確認 */
{
  if (code.kind != EofLine) err_exit("不正な記述です: ", kind_to_s(code));
}

TknKind lookCode(int line) /* line行の先頭コード */
{
  return (TknKind)(unsigned char)intercode[line][0];
}

CodeSet chk_nextCode(const CodeSet& cd, int kind2) /* 確認付コード取得 */
{
  if (cd.kind != kind2) {
    if (kind2   == EofLine) err_exit("不正な記述です: ", kind_to_s(cd));
    if (cd.kind == EofLine) err_exit(kind_to_s(kind2), " が必要です。");
    err_exit(kind_to_s(kind2) + " が " + kind_to_s(cd) + " の前に必要です。");
  }
  return nextCode();
}

CodeSet firstCode(int line) /* 先頭コード取得 */
{
  code_ptr = intercode[line];                 /* 解析用ポインタを行先頭に設定 */
  return nextCode();
}

CodeSet nextCode() /* コード取得 */
{
  TknKind kd;
  short int jmpAdrs, tblNbr;

  if (*code_ptr == '\0') return CodeSet(EofLine);
  kd = (TknKind)*UCHAR_P(code_ptr++);
  switch (kd) {
  case Func:
  case While: case For: case If: case Elif: case Else:
       jmpAdrs = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, -1, jmpAdrs);                           /* 飛先番地 */
  case String:
       tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, strLITERAL[tblNbr].c_str());  /* 文字列リテラル位置 */
  case IntNum: case DblNum:
       tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, nbrLITERAL[tblNbr]);              /* 数値リテラル値 */
  case Fcall: case Gvar: case Lvar:
       tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, tblNbr, -1);
  default:                                            /* 付属情報のないコード */
       return CodeSet(kd);
  }
}

void chk_dtTyp(const CodeSet& cd) /* 型あり確認 */
{
  if (tableP(cd)->dtTyp == NON_T)
    err_exit("初期化されていない変数が使用されました: ", kind_to_s(cd));
}

void set_dtTyp(const CodeSet& cd, char typ) /* 型設定 */
{
  int memAdrs = get_topAdrs(cd);
  vector<SymTbl>::iterator p = tableP(cd);

  if (p->dtTyp != NON_T) return;                    /* すでに型が決定している */
  p->dtTyp = typ;
  if (p->aryLen != 0) {                           /* 配列なら内容をゼロ初期化 */
    for (int n=0; n < p->aryLen; n++) { Dmem.set(memAdrs+n, 0); }
  }
}

int set_LITERAL(double d) /* 数値リテラル */
{
  for (int n=0; n<(int)nbrLITERAL.size(); n++) {
    if (nbrLITERAL[n] == d) return n;                   /* 同じ添字位置を返す */
  }
  nbrLITERAL.push_back(d);                                /* 数値リテラル格納 */
  return nbrLITERAL.size() - 1;                 /* 格納数値リテラルの添字位置 */
}

int set_LITERAL(const string& s) /* 文字列リテラル */
{
  for (int n=0; n<(int)strLITERAL.size(); n++) {
    if (strLITERAL[n] == s) return n;                   /* 同じ添字位置を返す */
  }
  strLITERAL.push_back(s);                              /* 文字列リテラル格納 */
  return strLITERAL.size() - 1;               /* 格納文字列リテラルの添字位置 */
}

