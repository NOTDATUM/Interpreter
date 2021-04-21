/**********************************************************/
/*    filename:bbi_tbl.cpp 記号表処理                     */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

/* ----------------------------テーブル */
vector<SymTbl> Gtable;            /* 大域記号表 */
vector<SymTbl> Ltable;            /* 局所記号表 */
int startLtable;            /* 局所用の開始位置 */

int enter(SymTbl& tb, SymKind kind) /* 記号表登録 */
{
  int n, mem_size;
  bool isLocal = is_localName(tb.name, kind);
  extern int localAdrs;                               /* 局所変数アドレス管理 */
  extern Mymemory Dmem;                                             /* 主記憶 */

  // 確認
  mem_size = tb.aryLen;
  if (mem_size == 0) mem_size = 1;                          /* 単純変数の場合 */
  if (kind!=varId && tb.name[0]=='$')                          /* $使用の確認 */
    err_exit("変数名以外で $ を使うことはできません: ", tb.name);
  tb.nmKind = kind;
  n = -1;                                                         /* 重複確認 */
  if (kind == fncId)  n = searchName(tb.name, 'G');
  if (kind == paraId) n = searchName(tb.name, 'L');
  if (n != -1) err_exit("名前が重複しています: ", tb.name);

  // アドレス設定
  if (kind == fncId) tb.adrs = get_lineNo();                    /* 関数開始行 */
  else {
    if (isLocal) { tb.adrs = localAdrs; localAdrs += mem_size; }      /* 局所 */
    else {
      tb.adrs = Dmem.size();                                          /* 大域 */
      Dmem.resize(Dmem.size() + mem_size);                    /* 大域領域確保 */
    }
  }

  // 登録
  if (isLocal) { n = Ltable.size(); Ltable.push_back(tb); }           /* 局所 */
  else         { n = Gtable.size(); Gtable.push_back(tb); }           /* 大域 */
  return n;                                                       /* 登録位置 */
}

void set_startLtable() /* 局所記号表の開始位置 */
{
  startLtable = Ltable.size();
}

bool is_localName(const string& name, SymKind kind) /* 局所名なら真 */
{
  if (kind == paraId) return true;
  if (kind == varId) {
    if (is_localScope() && name[0]!='$') return true; else return false;
  }
  return false;                                                      /* fncId */
}

int searchName(const string& s, int mode) /* 名前検索 */
{
  int n;
  switch (mode) {
  case 'G':  /* 大域記号表検索 */
       for (n=0; n<(int)Gtable.size(); n++) {
         if (Gtable[n].name == s) return n;
       }
       break;
  case 'L':  /* 局所記号表検索 */
       for (n=startLtable; n<(int)Ltable.size(); n++) {
         if (Ltable[n].name == s) return n;
       }
       break;
  case 'F':  /* 関数名検索 */
       n = searchName(s, 'G');
       if (n != -1 && Gtable[n].nmKind==fncId) return n;
       break;
  case 'V':  /* 変数名検索 */
       if (searchName(s, 'F') != -1) err_exit("関数名と重複しています: ", s);
       if (s[0] == '$')     return searchName(s, 'G');
       if (is_localScope()) return searchName(s, 'L');      /* 局所領域処理中 */
       else                 return searchName(s, 'G');      /* 大域領域処理中 */
  }
  return -1; // 見つからない
}

vector<SymTbl>::iterator tableP(const CodeSet& cd) /* 反復子取得 */
{
  if (cd.kind == Lvar) return Ltable.begin() + cd.symNbr;             /* Lvar */
  return Gtable.begin() + cd.symNbr;                            /* Gvar Fcall */
}

