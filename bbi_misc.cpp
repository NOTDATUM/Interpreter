/**********************************************************/
/*    filename:bbi_misc.cpp �G�֐�                        */
/**********************************************************/
#include "bbi.h"
#include "bbi_prot.h"

string dbl_to_s(double d) /* ���l�������� */
{
  ostringstream ostr;                               /* �o�͗p�X�g���[�����m�� */
  ostr << d;                                      /* �o�̓X�g���[���ɏ������� */
  return ostr.str();                                    /* �o�b�t�@���e��Ԃ� */
}

string err_msg(const string& a, const string& b) /* �G���[��񐶐� */
{
  if (a == "") return b + " ���K�v�ł��B";
  if (b == "") return a + " ���s���ł��B";
  return b + " �� " + a + " �̑O�ɕK�v�ł��B";
}

// �֐��錾�Ŏ��̃f�t�H���g�������w��
//void err_exit(Tobj a="\1", Tobj b="\1", Tobj c="\1", Tobj d="\1")
void err_exit(Tobj a, Tobj b, Tobj c, Tobj d) /* �G���[�\�� */
{
  Tobj ob[5];
  ob[1] = a; ob[2] = b; ob[3] = c; ob[4] = d;
  cerr << "line:" << get_lineNo() << " ERROR ";

  for (int i=1; i<=4 && ob[i].s!="\1"; i++) {
    if (ob[i].type == 'd') cout << ob[i].d;  // ���l���
    if (ob[i].type == 's') cout << ob[i].s;  // ��������
  }
  cout << endl;
  exit(1);
}
