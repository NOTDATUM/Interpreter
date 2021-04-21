/**********************************************************/
/*    filename:bbi_pars.cpp �\�����                      */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

#define NO_FIX_ADRS 0                                 /* �܂��Ԓn����̃}�[�N */
Token token;                                          /* ���ݏ������̃g�[�N�� */
SymTbl tmpTb;                                               /* �ꎞ�i�[�L���\ */
int blkNest;                                                /* �u���b�N�̐[�� */
int localAdrs;                                        /* �Ǐ��ϐ��A�h���X�Ǘ� */
int mainTblNbr;                             /* main�֐�������΂��̋L���\�ʒu */
int loopNest;                                                 /* ���[�v�l�X�g */
bool fncDecl_F;                                       /* �֐���`�������Ȃ�^ */
bool explicit_F;                                        /* �^�Ȃ�ϐ��錾���� */
char codebuf[LIN_SIZ+1], *codebuf_p;                  /* �����R�[�h������Ɨp */
extern vector<char*> intercode;                     /* �ϊ��ςݓ����R�[�h�i�[ */

void init() /* �����l�ݒ� */
{
  initChTyp();                                                    /* ������\ */
  mainTblNbr = -1;
  blkNest = loopNest = 0;
  fncDecl_F = explicit_F = false;
  codebuf_p = codebuf;
}

void convert_to_internalCode(char *fname) /* �R�[�h�ϊ� */
{
  init();                                               /* ������\�ȂǏ����� */

  // �֐���`���̂ݐ�ɓo�^
  fileOpen(fname);
  while (token=nextLine_tkn(), token.kind != EofProg) {
    if (token.kind == Func) {
      token = nextTkn(); set_name(); enter(tmpTb, fncId);
    }
  }

  // �����R�[�h�ւ̕ϊ�
  push_intercode();                                /* 0�s�ڂ͕s�v�Ȃ̂Ŗ��߂� */
  fileOpen(fname);
  token = nextLine_tkn();
  while (token.kind != EofProg) {
    convert();                                            /* �����R�[�h�ɕϊ� */
  }

  // main�֐�������΂��̌Ăяo���R�[�h��ݒ�
  set_startPc(1);                                        /* 1�s�ڂ�����s�J�n */
  if (mainTblNbr != -1) {
    set_startPc(intercode.size());                        /* main������s�J�n */
    setCode(Fcall, mainTblNbr); setCode('('); setCode(')');
    push_intercode();
  }
}

// �擪�����ɏo������R�[�h�������B�c�蕔����convert_rest()�ŏ���
void convert()
{
  switch (token.kind) {
  case Option: optionSet(); break;                          /* �I�v�V�����ݒ� */
  case Var:    varDecl();   break;                                /* �ϐ��錾 */
  case Func:   fncDecl();   break;                                /* �֐���` */
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
       if (loopNest <= 0) err_exit("�s����break�ł��B");
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case Return:
       if (!fncDecl_F) err_exit("�s����return�ł��B");
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case Exit:
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case Print: case Println:
       setCode(token.kind); token = nextTkn(); convert_rest();
       break;
  case End:
       err_exit("�s���� end �ł��B");       /* end ���P�ƂŎg���邱�Ƃ͂Ȃ� */
       break;
  default: convert_rest(); break;
  }
}

void convert_block_set() /* �u���b�N�����Ǘ� */
{
  int patch_line;
  patch_line = setCode(token.kind, NO_FIX_ADRS); token = nextTkn();
  convert_rest();
  convert_block();                                            /* �u���b�N���� */
  backPatch(patch_line, get_lineNo());        /* NO_FIX_ADRS���C��(end�s�ԍ�) */
}

void convert_block() /* �u���b�N�̏��� */
{
  TknKind k;
  ++blkNest;                                      /* �u���b�N�I�[�܂ŕ������ */
  while(k=token.kind, k!=Elif && k!=Else && k!=End && k!=EofProg) {
    convert();
  }
  --blkNest;
}

void convert_rest() /* ���̎c��̏��� */
{
  int tblNbr;

  for (;;) {
    if (token.kind == EofLine) break;
    switch (token.kind) {      //�������̃L�[���[�h���r���Ɍ���邱�Ƃ͂Ȃ� */
    case If: case Elif: case Else: case For: case While: case Break:
    case Func:  case Return:  case Exit:  case Print:  case Println:
    case Option: case Var: case End:
         err_exit("�s���ȋL�q�ł�: ", token.text);
         break;
    case Ident:                                              /* �֐��ďo,�ϐ� */
         set_name();
         if ((tblNbr=searchName(tmpTb.name, 'F')) != -1) {    /* �֐��o�^���� */
           if (tmpTb.name == "main") err_exit("main�֐��̌ďo�͂ł��܂���B");
           setCode(Fcall, tblNbr); continue;
         }
         if ((tblNbr=searchName(tmpTb.name, 'V')) == -1) {    /* �ϐ��o�^�Ȃ� */
           if (explicit_F) err_exit("�ϐ��錾���K�v�ł�: ", tmpTb.name);
           tblNbr = enter(tmpTb, varId);                      /* �����ϐ��o�^ */
         }
         if (is_localName(tmpTb.name, varId)) setCode(Lvar, tblNbr);
         else                                 setCode(Gvar, tblNbr);
         continue;
     case IntNum: case DblNum:                         /* ������double�^�Ŋi�[ */
         setCode(token.kind, set_LITERAL(token.dblVal));
         break;
     case String:
         setCode(token.kind, set_LITERAL(token.text));
         break;
    default:                                                   /* + - <= �Ȃ� */
         setCode(token.kind);
         break;
    }
    token = nextTkn();
  }
  push_intercode();
  token = nextLine_tkn();
}

void optionSet() /* �I�v�V�����ݒ� */
{
  setCode(Option);            /* ���̍s�͔���s�Ȃ̂ŃR�[�h�ϊ��� Option ���� */
  setCode_rest();                                       /* �c��͌��̂܂܊i�[ */
  token = nextTkn();                                  /* ���ϐ��錾���������� */
  if (token.kind==String && token.text=="var") explicit_F = true;
  else err_exit("option�w�肪�s���ł��B");
  token = nextTkn();
  setCode_EofLine();
}

void varDecl() /* var���g���ϐ��錾 */
{
  setCode(Var);                  /* ���̍s�͔���s�Ȃ̂ŃR�[�h�ϊ��� Var ���� */
  setCode_rest();                                       /* �c��͌��̂܂܊i�[ */
  for (;;) {
    token = nextTkn();
    var_namechk(token);                                           /* ���O���� */
    set_name(); set_aryLen();                             /* �z��Ȃ璷���ݒ� */
    enter(tmpTb, varId);                          /* �ϐ��o�^(�A�h���X���ݒ�) */
    if (token.kind != ',') break;                                 /* �錾�I�� */
  }
  setCode_EofLine();
}

void var_namechk(const Token& tk) /* ���O�m�F */
{
  if (tk.kind != Ident) err_exit(err_msg(tk.text, "���ʎq"));
  if (is_localScope() && tk.text[0] == '$')
    err_exit("�֐�����var�錾�ł� $ �t�����O���w��ł��܂���: ", tk.text);
  if (searchName(tk.text, 'V') != -1)
    err_exit("���ʎq���d�����Ă��܂�: ", tk.text);
}

void set_name() /* ���O�ݒ� */
{
  if (token.kind != Ident) err_exit("���ʎq���K�v�ł�: ", token.text);
  tmpTb.clear(); tmpTb.name = token.text;                         /* ���O�ݒ� */
  token = nextTkn();
}

void set_aryLen() /* �z��T�C�Y�ݒ� */
{
  tmpTb.aryLen = 0;
  if (token.kind != '[') return;                                /* �z��łȂ� */

  token = nextTkn();
  if (token.kind != IntNum)
    err_exit("�z�񒷂͐��̐����萔�Ŏw�肵�Ă�������: ", token.text);
  tmpTb.aryLen = (int)token.dblVal + 1;   /* var a[5]�͓Y��0�`5���L���Ȃ̂�+1 */
  token = chk_nextTkn(nextTkn(), ']');
  if (token.kind == '[') err_exit("�������z��͐錾�ł��܂���B");
}

void fncDecl() /* �֐���` */
{
  extern vector<SymTbl> Gtable;                                 /* ���L���\ */
  int tblNbr, patch_line, fncTblNbr;

  if(blkNest > 0) err_exit("�֐���`�̈ʒu���s���ł��B");
  fncDecl_F = true;                                     /* �֐������J�n�t���O */
  localAdrs = 0;                              /* �Ǐ��̈抄�t���J�E���^������ */
  set_startLtable();                                    /* �Ǐ��L���\�J�n�ʒu */
  patch_line = setCode(Func, NO_FIX_ADRS);          /* ���Ƃ�end�s�ԍ�������*/
  token = nextTkn();

  fncTblNbr = searchName(token.text, 'F');      /* �֐����͍ŏ��ɓo�^���Ă��� */
  Gtable[fncTblNbr].dtTyp = DBL_T;                      /* �֐��^��double�Œ� */

  // ���������
  token = nextTkn();
  token = chk_nextTkn(token, '(');                               /* '('�̂͂� */
  setCode('(');
  if (token.kind != ')') {                                        /* �������� */
    for (;; token=nextTkn()) {
      set_name();
      tblNbr = enter(tmpTb, paraId);                              /* �����o�^ */
      setCode(Lvar, tblNbr);                          /* ������Lvar�Ƃ��ď��� */
      ++Gtable[fncTblNbr].args;                               /* ��������+1 */
      if (token.kind != ',') break;                               /* �錾�I�� */
      setCode(',');
    }
  }
  token = chk_nextTkn(token, ')');                               /* ')'�̂͂� */
  setCode(')'); setCode_EofLine();
  convert_block();                                            /* �֐��{�̏��� */

  backPatch(patch_line, get_lineNo());                   /* NO_FIX_ADRS���C�� */
  setCode_End();
  Gtable[fncTblNbr].frame = localAdrs;                      /* �t���[���T�C�Y */

  if (Gtable[fncTblNbr].name == "main") {                     /* main�֐����� */
    mainTblNbr = fncTblNbr;
    if (Gtable[mainTblNbr].args != 0)
      err_exit("main�֐��ł͉��������w��ł��܂���B");
  }
  fncDecl_F = false;                                          /* �֐������I�� */
}

void backPatch(int line, int n) /* line�s��n��ݒ� */
{
  *SHORT_P(intercode[line] + 1) = (short)n;
}

void setCode(int cd) /* �R�[�h�i�[ */
{
  *codebuf_p++ = (char)cd;
}

int setCode(int cd, int nbr) /* �R�[�h��short�l�i�[ */
{
  *codebuf_p++ = (char)cd;
  *SHORT_P(codebuf_p) = (short)nbr; codebuf_p += SHORT_SIZ;
  return get_lineNo();                           /* backPatch�p�Ɋi�[�s��Ԃ� */
}

void setCode_rest() /* �c��̃e�L�X�g�����̂܂܊i�[ */
{
  extern char *token_p;
  strcpy(codebuf_p, token_p);
  codebuf_p += strlen(token_p) + 1;
}

void setCode_End() /* end�̊i�[���� */
{
  if (token.kind != End) err_exit(err_msg(token.text, "end"));
  setCode(End); token = nextTkn(); setCode_EofLine();
}

void setCode_EofLine() /* �ŏI�i�[���� */
{
  if (token.kind != EofLine) err_exit("�s���ȋL�q�ł�: ", token.text);
  push_intercode();
  token = nextLine_tkn();                                     /* ���̍s�ɐi�� */
}

void push_intercode() /* �ϊ����������R�[�h���i�[ */
{
  int len;
  char *p;

  *codebuf_p++ = '\0';
  if ((len = codebuf_p-codebuf) >= LIN_SIZ)
    err_exit("�ϊ���̓����R�[�h���������܂��B����Z�����Ă��������B");

  try {
    p = new char[len];                                          /* �������m�� */
    memcpy(p, codebuf, len);
    intercode.push_back(p);
  }
  catch (bad_alloc) { err_exit("�������m�ۂł��܂���"); }
  codebuf_p = codebuf;                /* ���̏����̂��߂Ɋi�[��擪�Ɉʒu�t�� */
}

bool is_localScope() /* �֐����������Ȃ�^ */
{
  return fncDecl_F;
}

