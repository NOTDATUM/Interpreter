/**********************************************************/
/*    filename:bbi_tbl.cpp �L���\����                     */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

/* ----------------------------�e�[�u�� */
vector<SymTbl> Gtable;            /* ���L���\ */
vector<SymTbl> Ltable;            /* �Ǐ��L���\ */
int startLtable;            /* �Ǐ��p�̊J�n�ʒu */

int enter(SymTbl& tb, SymKind kind) /* �L���\�o�^ */
{
  int n, mem_size;
  bool isLocal = is_localName(tb.name, kind);
  extern int localAdrs;                               /* �Ǐ��ϐ��A�h���X�Ǘ� */
  extern Mymemory Dmem;                                             /* ��L�� */

  // �m�F
  mem_size = tb.aryLen;
  if (mem_size == 0) mem_size = 1;                          /* �P���ϐ��̏ꍇ */
  if (kind!=varId && tb.name[0]=='$')                          /* $�g�p�̊m�F */
    err_exit("�ϐ����ȊO�� $ ���g�����Ƃ͂ł��܂���: ", tb.name);
  tb.nmKind = kind;
  n = -1;                                                         /* �d���m�F */
  if (kind == fncId)  n = searchName(tb.name, 'G');
  if (kind == paraId) n = searchName(tb.name, 'L');
  if (n != -1) err_exit("���O���d�����Ă��܂�: ", tb.name);

  // �A�h���X�ݒ�
  if (kind == fncId) tb.adrs = get_lineNo();                    /* �֐��J�n�s */
  else {
    if (isLocal) { tb.adrs = localAdrs; localAdrs += mem_size; }      /* �Ǐ� */
    else {
      tb.adrs = Dmem.size();                                          /* ��� */
      Dmem.resize(Dmem.size() + mem_size);                    /* ���̈�m�� */
    }
  }

  // �o�^
  if (isLocal) { n = Ltable.size(); Ltable.push_back(tb); }           /* �Ǐ� */
  else         { n = Gtable.size(); Gtable.push_back(tb); }           /* ��� */
  return n;                                                       /* �o�^�ʒu */
}

void set_startLtable() /* �Ǐ��L���\�̊J�n�ʒu */
{
  startLtable = Ltable.size();
}

bool is_localName(const string& name, SymKind kind) /* �Ǐ����Ȃ�^ */
{
  if (kind == paraId) return true;
  if (kind == varId) {
    if (is_localScope() && name[0]!='$') return true; else return false;
  }
  return false;                                                      /* fncId */
}

int searchName(const string& s, int mode) /* ���O���� */
{
  int n;
  switch (mode) {
  case 'G':  /* ���L���\���� */
       for (n=0; n<(int)Gtable.size(); n++) {
         if (Gtable[n].name == s) return n;
       }
       break;
  case 'L':  /* �Ǐ��L���\���� */
       for (n=startLtable; n<(int)Ltable.size(); n++) {
         if (Ltable[n].name == s) return n;
       }
       break;
  case 'F':  /* �֐������� */
       n = searchName(s, 'G');
       if (n != -1 && Gtable[n].nmKind==fncId) return n;
       break;
  case 'V':  /* �ϐ������� */
       if (searchName(s, 'F') != -1) err_exit("�֐����Əd�����Ă��܂�: ", s);
       if (s[0] == '$')     return searchName(s, 'G');
       if (is_localScope()) return searchName(s, 'L');      /* �Ǐ��̈揈���� */
       else                 return searchName(s, 'G');      /* ���̈揈���� */
  }
  return -1; // ������Ȃ�
}

vector<SymTbl>::iterator tableP(const CodeSet& cd) /* �����q�擾 */
{
  if (cd.kind == Lvar) return Ltable.begin() + cd.symNbr;             /* Lvar */
  return Gtable.begin() + cd.symNbr;                            /* Gvar Fcall */
}

