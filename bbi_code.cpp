/**************************************************************/
/*    filename:bbi_code.cpp �������Ǘ��ƍ\���`�F�b�N�Ǝ��s    */
/**************************************************************/
#include "bbi.h"
#include "bbi_prot.h"

CodeSet code;                                                 /* �R�[�h�Z�b�g */
int startPc;                                                    /* ���s�J�n�s */
int Pc = -1;                                /* �v���O�����J�E���^ -1:����s�� */
int baseReg;                                                /* �x�[�X���W�X�^ */
int spReg;                                                /* �X�^�b�N�|�C���^ */
int maxLine;                                              /* �v���O���������s */
vector<char*> intercode;                            /* �ϊ��ςݓ����R�[�h�i�[ */
char *code_ptr;                                   /* �����R�[�h��͗p�|�C���^ */
double returnValue;                                               /* �֐��ߒl */
bool break_Flg, return_Flg, exit_Flg;                         /* ����p�t���O */
Mymemory Dmem;                                                      /* ��L�� */
vector<string> strLITERAL;                              /* �����񃊃e�����i�[ */
vector<double> nbrLITERAL;                                /* ���l���e�����i�[ */
bool syntaxChk_mode = false;                          /* �\���`�F�b�N�̂Ƃ��^ */
extern vector<SymTbl> Gtable;                                   /* ���L���\ */

class Mystack {                                   /* stack<double> �̃��b�p�[ */
private:
  stack<double> st;
public:
  void push(double n) { st.push(n); }                                 /* �ύ� */
  int size() { return (int)st.size(); }                             /* �T�C�Y */
  bool empty() { return st.empty(); }                             /* �󂫔��� */
  double pop() {                        /* �Ǐo&�폜(����pop�ƈقȂ�̂Œ���) */
    if (st.empty()) err_exit("stack underflow");
    double d = st.top();                                          /* �g�b�v�l */
    st.pop(); return d;                                         /* �ЂƂ폜 */
  }
};
Mystack stk;                                            /* �I�y�����h�X�^�b�N */

void syntaxChk() /* �\���`�F�b�N */
{
  syntaxChk_mode = true;
  for (Pc=1; Pc<(int)intercode.size(); Pc++) {
    code = firstCode(Pc);
    switch (code.kind) {
    case Func: case Option: case Var:                         /* �`�F�b�N�ς� */
      break;
    case Else: case End: case Exit:
      code = nextCode(); chk_EofLine();
      break;
    case If: case Elif: case While:
      code = nextCode(); (void)get_expression(0, EofLine);            /* ���l */
      break;
    case For:
      code = nextCode();
      (void)get_memAdrs(code);                            /* ����ϐ��A�h���X */
      (void)get_expression('=', 0);                                 /* �����l */
      (void)get_expression(To, 0);                                  /* �ŏI�l */
      if (code.kind == Step) (void)get_expression(Step,0);          /* ���ݒl */
      chk_EofLine();
      break;
    case Fcall:                                         /* ����̂Ȃ��֐��ďo */
      fncCall_syntax(code.symNbr);
      chk_EofLine();
      (void)stk.pop();                                          /* �߂�l�s�v */
      break;
    case Print: case Println:
      sysFncExec_syntax(code.kind);
      break;
    case Gvar: case Lvar:                                           /* ����� */
      (void)get_memAdrs(code);                                /* ���ӃA�h���X */
      (void)get_expression('=', EofLine);                         /* �E�ӎ��l */
      break;
    case Return:
      code = nextCode();                                              /* �ߒl */
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
      err_exit("�s���ȋL�q�ł�: ", kind_to_s(code.kind));
    }
  }
  syntaxChk_mode = false;
}

void set_startPc(int n) /* �J�n�s�ݒ� */
{
  startPc = n;
}

void execute() /* ���s */
{
  baseReg = 0;                                        /* �x�[�X���W�X�^�����l */
  spReg = Dmem.size();                              /* �X�^�b�N�|�C���^�����l */
  Dmem.resize(spReg+1000);                              /* ��L���̈揉���m�� */
  break_Flg = return_Flg = exit_Flg = false;

  Pc = startPc;
  maxLine = intercode.size() - 1;
  while (Pc<=maxLine && !exit_Flg) {
    statement();
  }
  Pc = -1;                                                      /* ����s��� */
}

void statement() /* �� */
{
  CodeSet save;
  int top_line, end_line, varAdrs;
  double wkVal, endDt, stepDt;

  if (Pc>maxLine || exit_Flg) return;                       /* �v���O�����I�� */
  code = save = firstCode(Pc);
  top_line = Pc; end_line = code.jmpAdrs;             /* ����͈͂̐擪�ƏI�[ */
  if (code.kind == If ) end_line = endline_of_If(Pc);     /* if���̂Ƃ��̏I�[ */

  switch (code.kind) {
  case If:
    // if
    if (get_expression(If, 0)) {                          /* ����True�Ȃ�     */
      ++Pc; block(); Pc = end_line + 1;                   /*   �����s����     */
      return;                                             /* �����I��         */
    }
    Pc = save.jmpAdrs;                                    /*     ����         */
    // elif
    while (lookCode(Pc) == Elif) {
      save = firstCode(Pc); code = nextCode();
      if (get_expression()) {                             /* ����True�Ȃ�     */
        ++Pc; block(); Pc = end_line + 1;                 /*   �����s����     */
        return;                                           /* �����I��         */
      }
      Pc = save.jmpAdrs;                                  /*     ����         */
    }
    // else
    if (lookCode(Pc) == Else) {                           /* ����else��       */
      ++Pc; block(); Pc = end_line + 1;                   /*   �����s����     */
      return;                                             /* �����I��         */
    }
    // end
    ++Pc;
    break;
  case While:
    for (;;) {                                           /* ����              */
      if (!get_expression(While, 0)) break;              /* ������false�I��   */
      ++Pc; block();                                     /*   ����[���s]      */
      if (break_Flg || return_Flg || exit_Flg) {         /*   ����            */
        break_Flg = false; break;                        /* ���������f        */
      }                                                  /*   ����            */
      Pc = top_line; code = firstCode(Pc);               /* �������擪��      */
    }                                                    /*     ��            */
    Pc = end_line + 1;                                   /* ������            */
    break;
  case For:
    save = nextCode();
    varAdrs = get_memAdrs(save);                    /* ����ϐ��A�h���X���擾 */

    expression('=', 0);                                             /* �����l */
    set_dtTyp(save, DBL_T);                                         /* �^�m�� */
    Dmem.set(varAdrs, stk.pop());                             /* �����l��ݒ� */

    endDt = get_expression(To, 0);                            /* �ŏI�l��ۑ� */
                                                              /* ���ݒl��ۑ� */
    if (code.kind == Step) stepDt = get_expression(Step, 0); else stepDt = 1.0;
    for (;; Pc=top_line) {                               /* ����              */
      if (stepDt >= 0) {                                 /*   ��  �������[�v  */
        if (Dmem.get(varAdrs) > endDt) break;            /* �������U�Ȃ�I��  */
      } else {                                           /*   �����������[�v  */
        if (Dmem.get(varAdrs) < endDt) break;            /* �������U�Ȃ�I��  */
      }                                                  /*   ����            */
      ++Pc; block();                                     /*   ����[���s]      */
      if (break_Flg || return_Flg || exit_Flg) {         /*   ����            */
        break_Flg = false; break;                        /* ���������f        */
      }                                                  /*   ����            */
      Dmem.add(varAdrs, stepDt);                         /* ������ �l�X�V     */
    }                                                    /*     ��            */
    Pc = end_line + 1;                                   /* ������            */
    break;
  case Fcall:                                           /* ����̂Ȃ��֐��ďo */
    fncCall(code.symNbr);
    (void)stk.pop();                                            /* �߂�l�s�v */
    ++Pc;
    break;
  case Func:                                            /* �֐���`�̓X�L�b�v */
    Pc = end_line + 1;
    break;
  case Print: case Println:
    sysFncExec(code.kind);
    ++Pc;
    break;
  case Gvar: case Lvar:                                             /* ����� */
    varAdrs = get_memAdrs(code);
    expression('=', 0);
    set_dtTyp(save, DBL_T);                                 /* ������Ɍ^�m�� */
    Dmem.set(varAdrs, stk.pop());
    ++Pc;
    break;
  case Return:
    wkVal = returnValue;
    code = nextCode();
    if (code.kind!='?' && code.kind!=EofLine)   /* �u���v������Ζ߂�l���v�Z */
      wkVal = get_expression();
    post_if_set(return_Flg);                                /* ? ������Ώ��� */
    if (return_Flg) returnValue = wkVal;
    if (!return_Flg) ++Pc;
    break;
  case Break:
    code = nextCode(); post_if_set(break_Flg);              /* ? ������Ώ��� */
    if (!break_Flg) ++Pc;
    break;
  case Exit:
    code = nextCode(); exit_Flg = true;
    break;
  case Option: case Var: case EofLine:                        /* ���s���͖��� */
    ++Pc;
    break;
  default:
    err_exit("�s���ȋL�q�ł�: ", kind_to_s(code.kind));
  }
}

void block() /* �u���b�N�I�[�܂ł̕������s */
{
  TknKind k;
  while (!break_Flg && !return_Flg && !exit_Flg) {  /* break,return,exit�ŏI��*/
    k = lookCode(Pc);                                       /* ���̐擪�R�[�h */
    if (k==Elif || k==Else || k==End) break;              /* �u���b�N����I�� */
    statement();
  }
}

// �֐��錾�Ŏ��̃f�t�H���g�������w��
// double get_expression(int kind1=0, int kind2=0)
double get_expression(int kind1, int kind2) /* ���ʂ�Ԃ�expression */
{
  expression(kind1, kind2); return stk.pop();
}

void expression(int kind1, int kind2) /* �m�F�t��expression */
{
  if (kind1 != 0) code = chk_nextCode(code, kind1);
  expression();
  if (kind2 != 0) code = chk_nextCode(code, kind2);
}

void expression() /* �� */
{
  term(1);
}

void term(int n) /* n�͗D�揇�� */
{
  TknKind op;
  if (n == 7) { factor(); return; }
  term(n+1);
  while (n == opOrder(code.kind)) {                 /* �������������Z�q������ */
    op = code.kind;
    code = nextCode(); term(n+1);
    if (syntaxChk_mode) { stk.pop(); stk.pop(); stk.push(1.0); } /* �\��chk�� */
    else binaryExpr(op);
  }
}

void factor() /* ���q */
{
  TknKind kd = code.kind;

  if (syntaxChk_mode) {                                          /* �\��chk�� */
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
         err_exit("�����s���ł��B");
    default:
         err_exit("�����:", kind_to_s(code));            /* a + = �ȂǂŔ��� */
    }
    return;
  }

  switch (kd) {                                                     /* ���s�� */
  case Not: case Minus: case Plus:
       code = nextCode(); factor();                         /* ���̒l���擾�� */
       if (kd == Not) stk.push(!stk.pop());                      /* !�������� */
       if (kd == Minus) stk.push(-stk.pop());                    /* -�������� */
       break;                                            /* �P��+�͉������Ȃ� */
  case Lparen:
       expression('(', ')');
       break;
  case IntNum: case DblNum:
       stk.push(code.dblVal); code = nextCode();
       break;
  case Gvar: case Lvar:
       chk_dtTyp(code);                                 /* �l�ݒ�ς݂̕ϐ��� */
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

int opOrder(TknKind kd) /* �񍀉��Z�q�̗D�揇�� */
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
    default:                         return 0; /* �Y���Ȃ�   */
    }
}

void binaryExpr(TknKind op) /* �񍀉��Z */
{
  double d = 0, d2 = stk.pop(), d1 = stk.pop();

  if ((op==Divi || op==Mod || op==IntDivi) && d2==0)
    err_exit("�[�����Z�ł��B");

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

void post_if_set(bool& flg) /* ? �� */
{
  if (code.kind == EofLine) { flg = true; return; }       /* ?�����Ȃ�flg��^ */
  if (get_expression('?', 0)) flg = true;                     /* �������ŏ��� */
}

void fncCall_syntax(int fncNbr) /* �֐��ďo�`�F�b�N */
{
  int argCt = 0;

  code = nextCode(); code = chk_nextCode(code, '(');
  if (code.kind != ')') {                                       /* ���������� */
    for (;; code=nextCode()) {
      (void)get_expression(); ++argCt;                /* �����������ƈ����� */
      if (code.kind != ',') break;                        /* , �Ȃ���������� */
    }
  }
  code = chk_nextCode(code, ')');                                 /* ) �̂͂� */
  if (argCt != Gtable[fncNbr].args)                       /* �������`�F�b�N */
    err_exit(Gtable[fncNbr].name, " �֐��̈�����������Ă��܂��B");
  stk.push(1.0);                                                /* ����Ȗߒl */
}

void fncCall(int fncNbr) /* �֐��ďo */
{
  int  n, argCt = 0;
  vector<double> vc;

  // �������ςݍ���
  nextCode(); code = nextCode();                         /* �֐��� ( �X�L�b�v */
  if (code.kind != ')') {                                       /* ���������� */
    for (;; code=nextCode()) {
      expression(); ++argCt;                          /* �����������ƈ����� */
      if (code.kind != ',') break;                        /* , �Ȃ���������� */
    }
  }
  code = nextCode();                                            /* ) �X�L�b�v */

  // �����ςݍ��ݏ����ύX
  for (n=0; n<argCt; n++) vc.push_back(stk.pop());  /* ��납������ύ��ɏC�� */
  for (n=0; n<argCt; n++) { stk.push(vc[n]); }

  fncExec(fncNbr);                                                /* �֐����s */
}

void fncExec(int fncNbr) /* �֐����s */
{
  // �֐���������1
  int save_Pc = Pc;                                     /* ���݂̎��s�s��ۑ� */
  int save_baseReg = baseReg;                          /* ���݂�baseReg��ۑ� */
  int save_spReg = spReg;                                /* ���݂�spReg��ۑ� */
  char *save_code_ptr = code_ptr;         /* ���݂̎��s�s��͗p�|�C���^��ۑ� */
  CodeSet save_code = code;                               /* ���݂�code��ۑ� */

  // �֐���������2
  Pc = Gtable[fncNbr].adrs;                                   /* �V����Pc�ݒ� */
  baseReg = spReg;                                /* �V�����x�[�X���W�X�^�ݒ� */
  spReg += Gtable[fncNbr].frame;                              /* �t���[���m�� */
  Dmem.auto_resize(spReg);                            /* ��L���̗L���̈�m�� */
  returnValue = 1.0;                                          /* �߂�l����l */
  code = firstCode(Pc);                                     /* �擪�R�[�h�擾 */

  // �����i�[����
  nextCode(); code = nextCode();                           /* Func ( �X�L�b�v */
  if (code.kind != ')') {                                         /* �������� */
    for (;; code=nextCode()) {
      set_dtTyp(code, DBL_T);                               /* ������Ɍ^�m�� */
      Dmem.set(get_memAdrs(code), stk.pop());                 /* �������l�i�[ */
      if (code.kind != ',') break;                                /* �����I�� */
    }
  }
  code = nextCode();                                            /* ) �X�L�b�v */

  // �֐��{�̏���
  ++Pc; block(); return_Flg = false;                          /* �֐��{�̏��� */

  // �֐��o������
  stk.push(returnValue);                                        /* �߂�l�ݒ� */
  Pc       = save_Pc;                                   /* �ďo�O�̊��𕜊� */
  baseReg  = save_baseReg;
  spReg    = save_spReg;
  code_ptr = save_code_ptr;
  code     = save_code;
}

void sysFncExec_syntax(TknKind kd) /* �g���֐��`�F�b�N */
{
  switch (kd) {
  case Toint:
       code = nextCode(); (void)get_expression('(', ')');
       stk.push(1.0);
       break;
  case Input:
       code = nextCode();
       code = chk_nextCode(code, '('); code = chk_nextCode(code, ')');
       stk.push(1.0);                                             /* ����Ȓl */
       break;
  case Print: case Println:
       do {
         code = nextCode();
         if (code.kind == String) code = nextCode();        /* ������o�͊m�F */
         else (void)get_expression();                           /* �l�o�͊m�F */
       } while (code.kind == ',');                  /* , �Ȃ�p�����[�^������ */
       chk_EofLine();
       break;
  }
}

void sysFncExec(TknKind kd) /* �g���֐����s */
{
  double d;
  string s;

  switch (kd) {
  case Toint:
       code = nextCode();
       stk.push((int)get_expression('(', ')'));               /* �[���؂�̂� */
       break;
  case Input:
       nextCode(); nextCode(); code = nextCode();       /* input ( ) �X�L�b�v */
       getline(cin, s);                                        /* 1�s�ǂݍ��� */
       stk.push(atof(s.c_str()));                       /* �����ɕϊ����Ċi�[ */
       break;
  case Print: case Println:
       do {
         code = nextCode();
         if (code.kind == String) {                             /* ������o�� */
           cout << code.text; code = nextCode();
         } else {
           d = get_expression();                  /* �֐�����exit�̉\������ */
           if (!exit_Flg) cout << d;                              /* ���l�o�� */
         }
       } while (code.kind == ',');                  /* , �Ȃ�p�����[�^������ */
       if (kd == Println) cout << endl;                    /* println�Ȃ���s */
       break;
  }
}

// �P���ϐ��܂��͔z��v�f�̃A�h���X��Ԃ�
int get_memAdrs(const CodeSet& cd)
{
  int adr=0, index, len;
  double d;

  adr = get_topAdrs(cd);
  len = tableP(cd)->aryLen;
  code = nextCode();
  if (len == 0) return adr;                                     /* ��z��ϐ� */

  d = get_expression('[', ']');
  if ((int)d != d) err_exit("�Y���͒[���̂Ȃ����l�Ŏw�肵�Ă��������B");
  if (syntaxChk_mode) return adr;                           /* �\���`�F�b�N�� */

  index = (int)d;
  if (index <0 || len <= index)
    err_exit(index, " �͓Y���͈͊O�ł��i�Y���͈�:0-", len-1, "�j");
  return adr + index;                                             /* �Y�����Z */
}

// �ϐ��̐擪(�z��̂Ƃ������̐擪)�A�h���X��Ԃ�
int get_topAdrs(const CodeSet& cd)
{
  switch (cd.kind) {
  case Gvar: return tableP(cd)->adrs;
  case Lvar: return tableP(cd)->adrs + baseReg;
  default: err_exit("�ϐ������K�v�ł�: ", kind_to_s(cd));
  }
  return 0; // �����ɂ͂��Ȃ�
}

int endline_of_If(int line) /* if���̑Ή�end�ʒu */
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

void chk_EofLine() /* �R�[�h�m�F */
{
  if (code.kind != EofLine) err_exit("�s���ȋL�q�ł�: ", kind_to_s(code));
}

TknKind lookCode(int line) /* line�s�̐擪�R�[�h */
{
  return (TknKind)(unsigned char)intercode[line][0];
}

CodeSet chk_nextCode(const CodeSet& cd, int kind2) /* �m�F�t�R�[�h�擾 */
{
  if (cd.kind != kind2) {
    if (kind2   == EofLine) err_exit("�s���ȋL�q�ł�: ", kind_to_s(cd));
    if (cd.kind == EofLine) err_exit(kind_to_s(kind2), " ���K�v�ł��B");
    err_exit(kind_to_s(kind2) + " �� " + kind_to_s(cd) + " �̑O�ɕK�v�ł��B");
  }
  return nextCode();
}

CodeSet firstCode(int line) /* �擪�R�[�h�擾 */
{
  code_ptr = intercode[line];                 /* ��͗p�|�C���^���s�擪�ɐݒ� */
  return nextCode();
}

CodeSet nextCode() /* �R�[�h�擾 */
{
  TknKind kd;
  short int jmpAdrs, tblNbr;

  if (*code_ptr == '\0') return CodeSet(EofLine);
  kd = (TknKind)*UCHAR_P(code_ptr++);
  switch (kd) {
  case Func:
  case While: case For: case If: case Elif: case Else:
       jmpAdrs = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, -1, jmpAdrs);                           /* ���Ԓn */
  case String:
       tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, strLITERAL[tblNbr].c_str());  /* �����񃊃e�����ʒu */
  case IntNum: case DblNum:
       tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, nbrLITERAL[tblNbr]);              /* ���l���e�����l */
  case Fcall: case Gvar: case Lvar:
       tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
       return CodeSet(kd, tblNbr, -1);
  default:                                            /* �t�����̂Ȃ��R�[�h */
       return CodeSet(kd);
  }
}

void chk_dtTyp(const CodeSet& cd) /* �^����m�F */
{
  if (tableP(cd)->dtTyp == NON_T)
    err_exit("����������Ă��Ȃ��ϐ����g�p����܂���: ", kind_to_s(cd));
}

void set_dtTyp(const CodeSet& cd, char typ) /* �^�ݒ� */
{
  int memAdrs = get_topAdrs(cd);
  vector<SymTbl>::iterator p = tableP(cd);

  if (p->dtTyp != NON_T) return;                    /* ���łɌ^�����肵�Ă��� */
  p->dtTyp = typ;
  if (p->aryLen != 0) {                           /* �z��Ȃ���e���[�������� */
    for (int n=0; n < p->aryLen; n++) { Dmem.set(memAdrs+n, 0); }
  }
}

int set_LITERAL(double d) /* ���l���e���� */
{
  for (int n=0; n<(int)nbrLITERAL.size(); n++) {
    if (nbrLITERAL[n] == d) return n;                   /* �����Y���ʒu��Ԃ� */
  }
  nbrLITERAL.push_back(d);                                /* ���l���e�����i�[ */
  return nbrLITERAL.size() - 1;                 /* �i�[���l���e�����̓Y���ʒu */
}

int set_LITERAL(const string& s) /* �����񃊃e���� */
{
  for (int n=0; n<(int)strLITERAL.size(); n++) {
    if (strLITERAL[n] == s) return n;                   /* �����Y���ʒu��Ԃ� */
  }
  strLITERAL.push_back(s);                              /* �����񃊃e�����i�[ */
  return strLITERAL.size() - 1;               /* �i�[�����񃊃e�����̓Y���ʒu */
}

