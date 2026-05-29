extern int atoi(const char *);
extern long strtol(const char *, char **, int);
extern unsigned long strtoul(const char *, char **, int);
extern long long strtoll(const char *, char **, int);
extern unsigned long long strtoull(const char *, char **, int);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  char *end = 0;
  const char dec[] = " -42xyz";
  const char hex[] = "0x2a!";
  const char oct[] = "0755/";
  const char neg[] = "-99tail";
  const char wide_neg[] = "-1234567890;";
  const char wide_pos[] = "1234567890123z";
  const char none[] = "xyz";

  if (atoi(dec) != -42)
    return 1;

  if (strtol(hex, &end, 0) != 42 || end != hex + 4)
    return 2;

  end = 0;
  if (strtoul(oct, &end, 0) != 493UL || end != oct + 4)
    return 3;

  end = 0;
  if (strtoul(neg, &end, 10) != (unsigned long) -99L || end != neg + 3)
    return 4;

  end = 0;
  if (strtoll(wide_neg, &end, 10) != -1234567890LL ||
      end != wide_neg + 11)
    return 5;

  end = 0;
  if (strtoull(wide_pos, &end, 10) != 1234567890123ULL ||
      end != wide_pos + 13)
    return 6;

  end = 0;
  if (strtol(none, &end, 10) != 0 || end != none)
    return 7;

  return 1508;
}
