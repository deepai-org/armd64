extern int isalnum(int);
extern int isalpha(int);
extern int isdigit(int);
extern int islower(int);
extern int isspace(int);
extern int isupper(int);
extern int isxdigit(int);
extern int tolower(int);
extern int toupper(int);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  int (*volatile fn_isalnum)(int) = isalnum;
  int (*volatile fn_isalpha)(int) = isalpha;
  int (*volatile fn_isdigit)(int) = isdigit;
  int (*volatile fn_islower)(int) = islower;
  int (*volatile fn_isspace)(int) = isspace;
  int (*volatile fn_isupper)(int) = isupper;
  int (*volatile fn_isxdigit)(int) = isxdigit;
  int (*volatile fn_tolower)(int) = tolower;
  int (*volatile fn_toupper)(int) = toupper;

  if (fn_isalnum('Z') == 0 || fn_isalnum('?') != 0)
    return 1;
  if (fn_isalpha('m') == 0 || fn_isalpha('7') != 0)
    return 2;
  if (fn_isdigit('8') == 0 || fn_isdigit('x') != 0)
    return 3;
  if (fn_islower('q') == 0 || fn_islower('Q') != 0)
    return 4;
  if (fn_isspace('\n') == 0 || fn_isspace('n') != 0)
    return 5;
  if (fn_isupper('Q') == 0 || fn_isupper('q') != 0)
    return 6;
  if (fn_isxdigit('f') == 0 || fn_isxdigit('G') != 0)
    return 7;
  if (fn_tolower('A') != 'a' || fn_tolower('7') != '7')
    return 8;
  if (fn_toupper('z') != 'Z' || fn_toupper('7') != '7')
    return 9;

  return 1516;
}
